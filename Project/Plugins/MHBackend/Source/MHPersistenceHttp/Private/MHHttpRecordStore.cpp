#include "MHHttpRecordStore.h"

#include "MHHttpClient.h"
#include "Containers/Queue.h"
#include "Misc/Base64.h"

namespace
{
    bool ValidConfig(const FMHHttpRecordConfig& Config)
    {
        if (!FMath::IsFinite(Config.TimeoutSeconds) || Config.TimeoutSeconds <= 0.0f) return false;
        const FString& Url = Config.BaseUrl;
        if (Url.Contains(TEXT("?")) || Url.Contains(TEXT("#")) || Url.Contains(TEXT("@")) || Url.Contains(TEXT("\\"))) return false;
        for (TCHAR C : Url) { if (C <= 32 || C == 127) return false; }
        const bool bHttps = Url.StartsWith(TEXT("https://"), ESearchCase::CaseSensitive);
        if (!bHttps && !Url.StartsWith(TEXT("http://"), ESearchCase::CaseSensitive)) return false;
        FString Authority = Url.Mid(bHttps ? 8 : 7);
        int32 Slash;
        if (Authority.FindChar(TEXT('/'), Slash)) Authority.LeftInline(Slash);
        if (Authority.IsEmpty()) return false;
        if (bHttps) return true;
        FString Host = Authority, Port;
        if (Authority.Split(TEXT(":"), &Host, &Port) && (Port.IsEmpty() || !Port.IsNumeric())) return false;
        return Host.Equals(TEXT("localhost"), ESearchCase::IgnoreCase) || Host == TEXT("127.0.0.1");
    }

    FString EncodeKey(const FString& Value)
    {
        const FTCHARToUTF8 Utf8(*Value);
        FString Encoded = FBase64::Encode(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length());
        Encoded.ReplaceInline(TEXT("+"), TEXT("-"));
        Encoded.ReplaceInline(TEXT("/"), TEXT("_"));
        Encoded.ReplaceInline(TEXT("="), TEXT(""));
        return Encoded;
    }

    template <typename T> TMHResult<T> Failure(EMHErrorCode Code)
    {
        return TMHResult<T>::Failure({Code, TEXT("HTTP record operation failed")});
    }
}

namespace MH::Private
{
    struct FHttpRecordState : TSharedFromThis<FHttpRecordState, ESPMode::ThreadSafe>
    {
        FMHHttpRecordConfig Config;
        FMHHttpClient Client;
        TQueue<TUniqueFunction<void()>> Jobs;
        TMap<FGuid, FMHRequestHandle> Pending;
        FMHRequestHandle Active;
        FGuid ActiveId;
        bool bBusy = false;
        bool bShutdown = false;

        void StartNext()
        {
            check(IsInGameThread());
            TUniqueFunction<void()> Job;
            if (!bBusy && !bShutdown && Jobs.Dequeue(Job)) { bBusy = true; Job(); }
        }
        void Retire(const FGuid& Id)
        {
            Pending.Remove(Id);
            Active = {};
            ActiveId.Invalidate();
            bBusy = false;
            const TWeakPtr<FHttpRecordState, ESPMode::ThreadSafe> Weak = AsShared();
            DispatchCompletion([Weak]() { if (const auto Self = Weak.Pin()) Self->StartNext(); });
        }
    };

    template <typename T, typename TConvert>
    FMHRequestHandle QueueHttp(const TSharedRef<FHttpRecordState, ESPMode::ThreadSafe>& State,
        FMHRecordKey Key, const TCHAR* Verb, TArray<uint8> Payload,
        TUniqueFunction<void(TMHResult<T>)>&& Completion, TConvert Convert)
    {
        check(IsInGameThread());
        const FGuid Id = FGuid::NewGuid();
        const TWeakPtr<FHttpRecordState, ESPMode::ThreadSafe> Weak = State;
        TMHRequest<T> Source(MoveTemp(Completion), [Weak, Id]()
        {
            if (const auto Self = Weak.Pin()) { if (Self->ActiveId == Id) Self->Active.Cancel(); }
        });
        if (State->bShutdown || !ValidConfig(State->Config) || !MHIsValidRecordKey(Key))
        {
            Source.Complete(Failure<T>(State->bShutdown ? EMHErrorCode::Unavailable :
                (!ValidConfig(State->Config) ? EMHErrorCode::NotConfigured : EMHErrorCode::InvalidArgument)));
            return Source.GetHandle();
        }
        FMHHttpRequest Request;
        Request.Url = State->Config.BaseUrl + TEXT("/v1/records/") + EncodeKey(Key.Namespace) + TEXT("/") + EncodeKey(Key.Id);
        Request.Verb = Verb;
        Request.Body = MoveTemp(Payload);
        Request.Headers = State->Config.Headers;
        Request.Headers.Add(TEXT("Accept"), TEXT("application/octet-stream"));
        Request.Headers.Add(TEXT("Content-Type"), TEXT("application/octet-stream"));
        Request.TimeoutSeconds = State->Config.TimeoutSeconds;
        State->Pending.Add(Id, Source.GetHandle());
        State->Jobs.Enqueue([Weak, Id, Source, Request = MoveTemp(Request), Convert]() mutable
        {
            const auto Self = Weak.Pin();
            if (!Self) return;
            if (!Source.GetHandle().IsPending()) { Self->Retire(Id); return; }
            Self->ActiveId = Id;
            Self->Active = Self->Client.Send(Request, [Weak, Source, Id, Convert](TMHResult<FMHHttpResponse> Result) mutable
            {
                const auto Owner = Weak.Pin();
                if (!Owner) return;
                if (const auto Error = Result.GetError()) Source.Complete(Failure<T>(Error->Code));
                else Source.Complete(Convert(*Result.GetValue()));
                Owner->Retire(Id);
            });
        });
        DispatchCompletion([Weak]() { if (const auto Self = Weak.Pin()) Self->StartNext(); });
        return Source.GetHandle();
    }
}

FMHHttpRecordStore::FMHHttpRecordStore(FMHHttpRecordConfig Config)
    : State(MakeShared<MH::Private::FHttpRecordState, ESPMode::ThreadSafe>())
{
    check(IsInGameThread());
    while (Config.BaseUrl.EndsWith(TEXT("/"))) Config.BaseUrl.LeftChopInline(1);
    State->Config = MoveTemp(Config);
}
FMHHttpRecordStore::~FMHHttpRecordStore() { Shutdown(); }

FMHRequestHandle FMHHttpRecordStore::Load(FMHRecordKey Key, TUniqueFunction<void(TMHResult<TArray<uint8>>)>&& Completion)
{
    return MH::Private::QueueHttp<TArray<uint8>>(State, MoveTemp(Key), TEXT("GET"), {}, MoveTemp(Completion),
        [](FMHHttpResponse& Response)
        {
            if (Response.StatusCode == 200) return TMHResult<TArray<uint8>>::Success(MoveTemp(Response.Body));
            const auto Error = Response.GetStatusError();
            return Failure<TArray<uint8>>(Error.IsSet() ? Error->Code : EMHErrorCode::Internal);
        });
}
FMHRequestHandle FMHHttpRecordStore::Put(FMHRecordKey Key, TArray<uint8> Payload, TUniqueFunction<void(TMHResult<FMHUnit>)>&& Completion)
{
    return MH::Private::QueueHttp<FMHUnit>(State, MoveTemp(Key), TEXT("PUT"), MoveTemp(Payload), MoveTemp(Completion),
        [](FMHHttpResponse& Response)
        {
            if (Response.StatusCode == 204) return TMHResult<FMHUnit>::Success({});
            const auto Error = Response.GetStatusError();
            return Failure<FMHUnit>(Error.IsSet() ? Error->Code : EMHErrorCode::Internal);
        });
}
FMHRequestHandle FMHHttpRecordStore::Delete(FMHRecordKey Key, TUniqueFunction<void(TMHResult<FMHUnit>)>&& Completion)
{
    return MH::Private::QueueHttp<FMHUnit>(State, MoveTemp(Key), TEXT("DELETE"), {}, MoveTemp(Completion),
        [](FMHHttpResponse& Response)
        {
            if (Response.StatusCode == 204 || Response.StatusCode == 404) return TMHResult<FMHUnit>::Success({});
            const auto Error = Response.GetStatusError();
            return Failure<FMHUnit>(Error.IsSet() ? Error->Code : EMHErrorCode::Internal);
        });
}
void FMHHttpRecordStore::Shutdown()
{
    check(IsInGameThread());
    if (State->bShutdown) return;
    State->bShutdown = true;
    for (const auto& Request : State->Pending) Request.Value.Cancel();
    State->Pending.Empty();
    State->Jobs.Empty();
    State->Client.Shutdown();
}
bool FMHHttpRecordStore::HasPendingIO() const
{
    check(IsInGameThread());
    return State->bBusy || !State->Jobs.IsEmpty();
}
