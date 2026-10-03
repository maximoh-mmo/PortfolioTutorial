#include "MHSteamIdentityProvider.h"
#include "MHSteamClient.h"
#include "Containers/Ticker.h"
#include "HAL/PlatformTime.h"

struct FMHSteamIdentityProvider::FState : TSharedFromThis<FState>
{
    struct FPending
    {
        TMHRequest<FMHAuthTicketRef> Source;
        FMHAccountId Account;
        FString Audience;
        double Deadline;
    };
    TSharedRef<IMHSteamClient> Client;
    TMap<uint32, FPending> Pending;
    TArray<TWeakPtr<FMHAuthTicket>> Tickets;
    FTSTicker::FDelegateHandle Ticker;
    bool bClosed = false;
    explicit FState(TSharedRef<IMHSteamClient> InClient) : Client(InClient) {}

    void Cancel(uint32 Handle)
    {
        if (Pending.Remove(Handle)) { Client->Cancel(Handle); }
    }
    void Tick()
    {
        const FString Account = Client->Account();
        for (auto It = Pending.CreateIterator(); It; ++It)
        {
            auto& Job = It.Value();
            const bool bLoggedOut = Account != Job.Account.Subject;
            const bool bTimedOut = FPlatformTime::Seconds() >= Job.Deadline;
            if (!Job.Source.GetHandle().IsPending() || bLoggedOut || bTimedOut)
            {
                Client->Cancel(It.Key());
                Job.Source.Complete(TMHResult<FMHAuthTicketRef>::Failure({
                    bLoggedOut ? EMHErrorCode::Unauthorized : EMHErrorCode::Timeout,
                    TEXT("Steam ticket request ended")}));
                It.RemoveCurrent();
            }
        }
        for (int32 I = Tickets.Num() - 1; I >= 0; --I)
        {
            auto Ticket = Tickets[I].Pin();
            if (Ticket && Ticket->GetAccount().Subject != Account) { Ticket->Release(); }
            if (!Ticket || Ticket->IsReleased()) { Tickets.RemoveAtSwap(I); }
        }
        FMHSteamEvent Event;
        while (Client->Poll(Event))
        {
            if (auto* Job = Pending.Find(Event.Handle))
            {
                auto Source = Job->Source;
                if (Event.bSuccess && !Event.Bytes.IsEmpty())
                {
                    auto Ticket = MakeShared<FMHAuthTicket>(Job->Account, Job->Audience, MoveTemp(Event.Bytes),
                        [Native = Client, Handle = Event.Handle]() { Native->Cancel(Handle); });
                    Tickets.Add(Ticket);
                    Source.Complete(TMHResult<FMHAuthTicketRef>::Success(Ticket));
                }
                else
                {
                    Client->Cancel(Event.Handle);
                    Source.Complete(TMHResult<FMHAuthTicketRef>::Failure({EMHErrorCode::Unavailable, TEXT("Steam ticket failed")}));
                }
                Pending.Remove(Event.Handle);
            }
            // Unknown/late callbacks belong to cancelled requests or other SDK consumers.
            if (!Event.Bytes.IsEmpty()) { FMemory::Memzero(Event.Bytes.GetData(), Event.Bytes.Num()); Event.Bytes.Empty(); }
        }
    }
};

FMHSteamIdentityProvider::FMHSteamIdentityProvider() : FMHSteamIdentityProvider(MHCreateSteamClient()) {}
FMHSteamIdentityProvider::FMHSteamIdentityProvider(TSharedRef<IMHSteamClient> Client)
    : State(MakeShared<FState>(Client))
{
    check(IsInGameThread());
    TWeakPtr<FState> Weak = State;
    State->Ticker = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak](float)
    {
        if (auto Pinned = Weak.Pin()) { Pinned->Tick(); return !Pinned->bClosed; }
        return false;
    }));
}
FMHSteamIdentityProvider::~FMHSteamIdentityProvider() { Shutdown(); }

FMHRequestHandle FMHSteamIdentityProvider::GetLocalAccount(int32 Index,
    TUniqueFunction<void(TMHResult<FMHAccountId>)>&& Completion)
{
    check(IsInGameThread());
    TMHRequest<FMHAccountId> Source(MoveTemp(Completion));
    EMHErrorCode Error = EMHErrorCode::Unauthorized;
    if (State->bClosed) { Error = EMHErrorCode::Unavailable; }
    else if (Index != 0) { Error = EMHErrorCode::InvalidArgument; }
    else if (!State->Client->IsAvailable()) { Error = EMHErrorCode::NotConfigured; }
    else if (const auto Account = State->Client->Account(); !Account.IsEmpty())
    {
        Source.Complete(TMHResult<FMHAccountId>::Success({TEXT("Steam"), Account}));
        return Source.GetHandle();
    }
    Source.Complete(TMHResult<FMHAccountId>::Failure({Error, TEXT("Steam local account unavailable")}));
    return Source.GetHandle();
}

FMHRequestHandle FMHSteamIdentityProvider::AcquireAuthTicket(const FMHAuthTicketRequest& Request,
    TUniqueFunction<void(TMHResult<FMHAuthTicketRef>)>&& Completion)
{
    check(IsInGameThread());
    const bool bWeb = Request.Audience.StartsWith(TEXT("webapi:"), ESearchCase::CaseSensitive);
    const bool bSession = Request.Audience.StartsWith(TEXT("session:"), ESearchCase::CaseSensitive);
    const FString Identity = Request.Audience.Mid(bWeb ? 7 : 8);
    bool bValid = (bWeb || bSession) && !Identity.IsEmpty() && Identity.Len() <= 128;
    for (TCHAR C : Identity)
    {
        bValid &= bSession ? (C >= '0' && C <= '9')
            : ((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') || (C >= '0' && C <= '9') || C == '_' || C == '-' || C == '.');
    }
    if (bSession)
    {
        const uint64 Target = FCString::Strtoui64(*Identity, nullptr, 10);
        bValid &= Target != 0 && FString::Printf(TEXT("%llu"), Target) == Identity;
    }
    const auto Fail = [&Completion](EMHErrorCode Code)
    {
        TMHRequest<FMHAuthTicketRef> Source(MoveTemp(Completion));
        Source.Complete(TMHResult<FMHAuthTicketRef>::Failure({Code, TEXT("Steam ticket unavailable or invalid request")}));
        return Source.GetHandle();
    };
    if (State->bClosed) { return Fail(EMHErrorCode::Unavailable); }
    if (!bValid || Request.LocalUserIndex != 0 || !FMath::IsFinite(Request.TimeoutSeconds) || Request.TimeoutSeconds <= 0)
    { return Fail(EMHErrorCode::InvalidArgument); }
    if (!State->Client->IsAvailable()) { return Fail(EMHErrorCode::NotConfigured); }
    const FString Account = State->Client->Account();
    if (Account.IsEmpty()) { return Fail(EMHErrorCode::Unauthorized); }
    const uint32 Handle = State->Client->Begin(Request.Audience);
    if (Handle == 0) { return Fail(EMHErrorCode::Unavailable); }
    TWeakPtr<FState> Weak = State;
    TMHRequest<FMHAuthTicketRef> Source(MoveTemp(Completion), [Weak, Handle]()
    { if (auto Pinned = Weak.Pin()) { Pinned->Cancel(Handle); } });
    State->Pending.Add(Handle, {Source, {TEXT("Steam"), Account}, Request.Audience,
        FPlatformTime::Seconds() + Request.TimeoutSeconds});
    return Source.GetHandle();
}

void FMHSteamIdentityProvider::Shutdown()
{
    check(IsInGameThread());
    if (State->bClosed) { return; }
    State->bClosed = true;
    FTSTicker::GetCoreTicker().RemoveTicker(State->Ticker);
    for (const auto& Pair : State->Pending) { State->Client->Cancel(Pair.Key); Pair.Value.Source.GetHandle().Cancel(); }
    State->Pending.Empty();
    for (const auto& Weak : State->Tickets) { if (auto Ticket = Weak.Pin()) { Ticket->Release(); } }
    State->Tickets.Empty();
}
