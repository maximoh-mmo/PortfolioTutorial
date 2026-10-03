#include "IMHIdentityProvider.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "HAL/PlatformTime.h"

namespace
{
    // Controlled provider: native ticket arrival is injected separately from the request.
    class FMockIdentity final : public IMHIdentityProvider
    {
        TArray<TMHRequest<FMHAuthTicketRef>> Pending;
        TArray<TWeakPtr<FMHAuthTicket>> Tickets;
        bool bClosed = false;
    public:
        ~FMockIdentity() override { Shutdown(); }
        FMHRequestHandle GetLocalAccount(int32 Index,
            TUniqueFunction<void(TMHResult<FMHAccountId>)>&& Callback) override
        {
            TMHRequest<FMHAccountId> Source(MoveTemp(Callback));
            Source.Complete(bClosed || Index != 0
                ? TMHResult<FMHAccountId>::Failure({EMHErrorCode::Unavailable, TEXT("No local account")})
                : TMHResult<FMHAccountId>::Success({TEXT("mock"), TEXT("123")}));
            return Source.GetHandle();
        }
        FMHRequestHandle AcquireAuthTicket(const FMHAuthTicketRequest& Request,
            TUniqueFunction<void(TMHResult<FMHAuthTicketRef>)>&& Callback) override
        {
            TMHRequest<FMHAuthTicketRef> Source(MoveTemp(Callback));
            if (bClosed)
            {
                Source.Complete(TMHResult<FMHAuthTicketRef>::Failure({EMHErrorCode::Unavailable, TEXT("Closed")}));
            }
            else { Pending.Add(Source); }
            return Source.GetHandle();
        }
        void Deliver(const FMHAuthTicketRef& Ticket)
        {
            Tickets.Add(Ticket);
            for (const auto& Source : Pending)
            {
                Source.Complete(TMHResult<FMHAuthTicketRef>::Success(Ticket));
            }
            Pending.Empty();
        }
        void Shutdown() override
        {
            bClosed = true;
            for (const auto& Source : Pending) { Source.GetHandle().Cancel(); }
            Pending.Empty();
            for (const auto& Weak : Tickets) { if (auto Ticket = Weak.Pin()) { Ticket->Release(); } }
            Tickets.Empty();
        }
    };

    class FWaitIdentity final : public IAutomationLatentCommand
    {
        TFunction<bool()> Check;
        FAutomationTestBase* Test;
        double Deadline = FPlatformTime::Seconds() + 10;
    public:
        FWaitIdentity(TFunction<bool()> InCheck, FAutomationTestBase* InTest)
            : Check(MoveTemp(InCheck)), Test(InTest) {}
        bool Update() override
        {
            if (Check()) { return true; }
            if (FPlatformTime::Seconds() < Deadline) { return false; }
            Test->AddError(TEXT("Identity callback timeout"));
            return true;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMHIdentityTest, "MH.Online.IdentityContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMHIdentityTest::RunTest(const FString& Parameters)
{
    const FMHAccountId Account{TEXT("mock"), TEXT("123")};
    TestTrue(TEXT("Account is valid"), Account.IsValid());
    TestFalse(TEXT("Empty account is invalid"), FMHAccountId{}.IsValid());
    TestFalse(TEXT("Provider is part of identity"), Account == FMHAccountId{TEXT("other"), TEXT("123")});
    TestFalse(TEXT("Provider is case sensitive"), Account == FMHAccountId{TEXT("Mock"), TEXT("123")});
    struct FObserved { int32 Calls = 0; int32 Releases = 0; };
    auto Observed = MakeShared<FObserved>();
    auto MakeTicket = [Observed, Account]()
    {
        return MakeShared<FMHAuthTicket>(Account, TEXT("test"), TArray<uint8>{0, 255, 42},
            [Observed]() { ++Observed->Releases; });
    };
    {
        auto Ticket = MakeTicket();
        auto Copy = Ticket;
        Ticket->Release();
        Copy->Release();
        TestTrue(TEXT("Shared release clears credential"), Copy->IsReleased() && Copy->GetBytes().IsEmpty());
    }
    TestEqual(TEXT("Explicit release and destruction revoke once"), Observed->Releases, 1);
    { auto Ticket = MakeTicket(); }
    TestEqual(TEXT("Last owner revokes automatically"), Observed->Releases, 2);
    auto Provider = MakeShared<FMockIdentity>();
    Provider->GetLocalAccount(0, [this, Observed, Account](TMHResult<FMHAccountId> Result)
    {
        ++Observed->Calls;
        TestTrue(TEXT("Deferred local account on game thread"), IsInGameThread() && Result.GetValue() && *Result.GetValue() == Account);
    });
    auto Cancelled = [this, Observed](TMHResult<FMHAuthTicketRef> Result)
    {
        ++Observed->Calls;
        TestTrue(TEXT("One cancelled completion"), IsInGameThread() && Result.GetError() && Result.GetError()->Code == EMHErrorCode::Cancelled);
    };
    const auto Handle = Provider->AcquireAuthTicket({}, Cancelled);
    TestTrue(TEXT("Cancel pending ticket"), Handle.Cancel());
    { auto Late = MakeTicket(); Provider->Deliver(Late); }
    TestEqual(TEXT("Late native ticket released"), Observed->Releases, 3);
    auto Successful = MakeShared<FMockIdentity>();
    Successful->AcquireAuthTicket({}, [this, Observed, Successful](TMHResult<FMHAuthTicketRef> Result)
    {
        ++Observed->Calls;
        TestTrue(TEXT("Ticket bytes preserved"), Result.GetValue() && (*Result.GetValue())->GetBytes() == TArray<uint8>({0, 255, 42}));
        Successful->Shutdown();
        TestTrue(TEXT("Shutdown revokes delivered ticket"), Result.GetValue() && (*Result.GetValue())->IsReleased());
    });
    Successful->Deliver(MakeTicket());
    Provider->AcquireAuthTicket({}, Cancelled);
    Provider->Shutdown();
    Provider->Shutdown();
    Provider->AcquireAuthTicket({}, [this, Observed](TMHResult<FMHAuthTicketRef> Result)
    {
        ++Observed->Calls;
        TestTrue(TEXT("Closed provider rejects work"), Result.GetError() && Result.GetError()->Code == EMHErrorCode::Unavailable);
    });
    TestEqual(TEXT("Callbacks never inline"), Observed->Calls, 0);
    ADD_LATENT_AUTOMATION_COMMAND(FWaitIdentity([this, Observed, Provider, Successful]()
    {
        if (Observed->Calls < 5) { return false; }
        TestEqual(TEXT("Exactly once callbacks"), Observed->Calls, 5);
        TestEqual(TEXT("All native leases released once"), Observed->Releases, 4);
        return true;
    }, this));
    return true;
}
#endif
