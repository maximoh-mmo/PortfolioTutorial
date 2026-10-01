#include "MHHttpClient.h"
#include "MHHttpClientState.h"

namespace
{
    bool IsToken(const FString& Text)
    {
        if (Text.IsEmpty()) { return false; }
        for (TCHAR C : Text)
        {
            const bool bAlphaNumeric = (C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') || (C >= '0' && C <= '9');
            if (!bAlphaNumeric && !FString(TEXT("!#$%&'*+-.^_`|~")).Contains(FString::Chr(C))) { return false; }
        }
        return true;
    }

    bool IsValid(const FMHHttpRequest& Request)
    {
        if (!FMath::IsFinite(Request.TimeoutSeconds) || Request.TimeoutSeconds <= 0 || !IsToken(Request.Verb)) { return false; }
        const int32 SchemeLength = Request.Url.StartsWith(TEXT("https://"), ESearchCase::IgnoreCase) ? 8 :
            (Request.Url.StartsWith(TEXT("http://"), ESearchCase::IgnoreCase) ? 7 : 0);
        if (!SchemeLength || Request.Url.Len() <= SchemeLength) { return false; }
        const FString Remainder = Request.Url.Mid(SchemeLength);
        int32 AuthorityEnd = 0;
        while (AuthorityEnd < Remainder.Len() && Remainder[AuthorityEnd] != '/' &&
            Remainder[AuthorityEnd] != '?' && Remainder[AuthorityEnd] != '#') { ++AuthorityEnd; }
        if (AuthorityEnd == 0 || Remainder.Left(AuthorityEnd).Contains(TEXT("@"))) { return false; }
        for (TCHAR C : Request.Url) { if (C <= 32 || C == 127) { return false; } }
        for (const auto& Header : Request.Headers)
        {
            if (!IsToken(Header.Key)) { return false; }
            for (TCHAR C : Header.Value) { if ((C < 32 && C != '\t') || C == 127) { return false; } }
        }
        return true;
    }
}

TOptional<FMHError> FMHHttpResponse::GetStatusError() const
{
    if (IsSuccessStatus()) { return {}; }
    EMHErrorCode Code = EMHErrorCode::Internal;
    if (StatusCode == 401 || StatusCode == 403) { Code = EMHErrorCode::Unauthorized; }
    else if (StatusCode == 404) { Code = EMHErrorCode::NotFound; }
    else if (StatusCode == 409) { Code = EMHErrorCode::Conflict; }
    else if (StatusCode == 408 || StatusCode == 504) { Code = EMHErrorCode::Timeout; }
    else if (StatusCode == 429 || StatusCode >= 500) { Code = EMHErrorCode::Unavailable; }
    else if (StatusCode >= 400) { Code = EMHErrorCode::InvalidArgument; }
    return FMHError{Code, FString::Printf(TEXT("HTTP response status %d"), StatusCode)};
}

MH::Private::FHttpClientState::~FHttpClientState() { Shutdown(); }

FMHRequestHandle MH::Private::FHttpClientState::Send(const FMHHttpRequest& Request,
    TUniqueFunction<void(TMHResult<FMHHttpResponse>)>&& Completion)
{
    check(IsInGameThread());
    const FGuid Id = FGuid::NewGuid();
    const TWeakPtr<FHttpClientState, ESPMode::ThreadSafe> WeakSelf = AsShared();
    TMHRequest<FMHHttpResponse> Source(MoveTemp(Completion), [WeakSelf, Id]()
    {
        if (const auto Self = WeakSelf.Pin()) { Self->CancelOperation(Id); }
    });
    if (bShutdown || !IsValid(Request))
    {
        Source.Complete(TMHResult<FMHHttpResponse>::Failure({bShutdown ? EMHErrorCode::Unavailable : EMHErrorCode::InvalidArgument,
            bShutdown ? TEXT("HTTP client is shut down") : TEXT("Invalid HTTP request")}));
        return Source.GetHandle();
    }
    const auto Operation = Factory(Request);
    Active.Add(Id, FActive{Source.GetHandle(), Operation});
    const bool bStarted = Operation->Start([WeakSelf, Source, Id](TMHResult<FMHHttpResponse> Result) mutable
    {
        check(IsInGameThread());
        if (const auto Self = WeakSelf.Pin()) { Self->Active.Remove(Id); }
        Source.Complete(MoveTemp(Result));
    });
    if (!bStarted)
    {
        Active.Remove(Id);
        Source.Complete(TMHResult<FMHHttpResponse>::Failure({EMHErrorCode::Unavailable, TEXT("HTTP request could not start")}));
    }
    return Source.GetHandle();
}

void MH::Private::FHttpClientState::CancelOperation(FGuid Id)
{
    check(IsInGameThread());
    if (const FActive* Entry = Active.Find(Id))
    {
        const auto Operation = Entry->Operation;
        Active.Remove(Id);
        Operation->Cancel();
    }
}

void MH::Private::FHttpClientState::Shutdown()
{
    check(IsInGameThread());
    if (bShutdown) { return; }
    bShutdown = true;
    auto Outstanding = MoveTemp(Active);
    Active.Empty();
    for (const auto& Pair : Outstanding)
    {
        Pair.Value.Handle.Cancel();
        Pair.Value.Operation->Cancel();
    }
}

FMHHttpClient::FMHHttpClient()
    : State(MakeShared<MH::Private::FHttpClientState, ESPMode::ThreadSafe>(MH::Private::MakeUnrealHttpOperation))
{
    check(IsInGameThread());
}
FMHHttpClient::~FMHHttpClient() { Shutdown(); }
FMHRequestHandle FMHHttpClient::Send(const FMHHttpRequest& Request,
    TUniqueFunction<void(TMHResult<FMHHttpResponse>)>&& Completion)
{
    return State->Send(Request, MoveTemp(Completion));
}
void FMHHttpClient::Shutdown() { State->Shutdown(); }
