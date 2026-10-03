#include "MHSteamIdentityProvider.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "MHSteamClient.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformTime.h"

namespace
{
    class FFakeSteam final : public IMHSteamClient
    {
    public:
        FString User = TEXT("76561198000000001");
        bool bAvailable = true, bRespond = true, bSuccess = true;
        uint32 Next = 0;
        int32 CancelCalls = 0;
        TArray<FMHSteamEvent> Events;
        bool IsAvailable() const override { return bAvailable; }
        FString Account() const override { return User; }
        uint32 Begin(const FString&) override
        {
            ++Next;
            if (bRespond) { Events.Add({Next, bSuccess, {0, 255, 42}}); }
            return Next;
        }
        void Cancel(uint32) override { ++CancelCalls; }
        bool Poll(FMHSteamEvent& Event) override
        {
            if (Events.IsEmpty()) { return false; }
            Event = MoveTemp(Events[0]); Events.RemoveAt(0); return true;
        }
    };
    class FWaitSteam final : public IAutomationLatentCommand
    {
        TFunction<bool()> Check;
        FAutomationTestBase* Test;
        double Deadline = FPlatformTime::Seconds() + 40;
    public:
        FWaitSteam(TFunction<bool()> InCheck, FAutomationTestBase* InTest) : Check(MoveTemp(InCheck)), Test(InTest) {}
        bool Update() override
        {
            if (Check()) { return true; }
            if (FPlatformTime::Seconds() < Deadline) { return false; }
            Test->AddError(TEXT("Steam test completion timeout")); return true;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMHSteamLifecycleTest, "MH.Online.SteamLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMHSteamLifecycleTest::RunTest(const FString& Parameters)
{
    struct FObserved
    {
        int32 Calls = 0;
        TArray<TSharedPtr<FMHSteamIdentityProvider>> Providers;
        TArray<TSharedPtr<FFakeSteam>> Clients;
    };
    auto Observed = MakeShared<FObserved>();
    auto Create = [Observed]()
    {
        auto Client = MakeShared<FFakeSteam>();
        TSharedPtr<FMHSteamIdentityProvider> Provider(new FMHSteamIdentityProvider(Client));
        Observed->Clients.Add(Client); Observed->Providers.Add(Provider);
        return Provider;
    };
    auto Expect = [this, Observed](EMHErrorCode Code)
    {
        return [this, Observed, Code](TMHResult<FMHAuthTicketRef> Result)
        {
            ++Observed->Calls;
            TestTrue(TEXT("Deferred game-thread error"), IsInGameThread() && Result.GetError() && Result.GetError()->Code == Code);
        };
    };
    FMHAuthTicketRequest Request{0, TEXT("webapi:mh-test"), 0.01f};
    Request.TimeoutSeconds = 10;
    auto Success = Create();
    Success->AcquireAuthTicket(Request, [this, Observed, Success](TMHResult<FMHAuthTicketRef> Result)
    {
        ++Observed->Calls;
        TestTrue(TEXT("Steam account and binary credential"), Result.GetValue()
            && (*Result.GetValue())->GetAccount().Provider == TEXT("Steam")
            && (*Result.GetValue())->GetBytes() == TArray<uint8>({0, 255, 42}));
        Success->Shutdown();
        TestTrue(TEXT("Shutdown releases successful lease"), Result.GetValue() && (*Result.GetValue())->IsReleased());
    });
    auto Cancel = Create();
    TestTrue(TEXT("Pending cancellation wins"), Cancel->AcquireAuthTicket(Request, Expect(EMHErrorCode::Cancelled)).Cancel());
    auto Failure = Create(); Observed->Clients.Last()->bSuccess = false;
    Failure->AcquireAuthTicket(Request, Expect(EMHErrorCode::Unavailable));
    auto Timeout = Create(); Observed->Clients.Last()->bRespond = false;
    Request.TimeoutSeconds = 0.01f;
    Timeout->AcquireAuthTicket(Request, Expect(EMHErrorCode::Timeout));
    Request.TimeoutSeconds = 10;
    auto Logout = Create();
    Logout->AcquireAuthTicket(Request, Expect(EMHErrorCode::Unauthorized));
    Observed->Clients.Last()->User.Empty();
    auto Closed = Create();
    Closed->AcquireAuthTicket(Request, Expect(EMHErrorCode::Cancelled)); Closed->Shutdown(); Closed->Shutdown();
    Closed->AcquireAuthTicket(Request, Expect(EMHErrorCode::Unavailable));
    auto Invalid = Create();
    auto Bad = Request; Bad.Audience = TEXT("webapi:");
    Invalid->AcquireAuthTicket(Bad, Expect(EMHErrorCode::InvalidArgument));
    auto Missing = Create(); Observed->Clients.Last()->bAvailable = false;
    Missing->AcquireAuthTicket(Request, Expect(EMHErrorCode::NotConfigured));
    auto SignedOut = Create(); Observed->Clients.Last()->User.Empty();
    SignedOut->AcquireAuthTicket(Request, Expect(EMHErrorCode::Unauthorized));
    auto Destroyed = Create();
    Destroyed->AcquireAuthTicket(Request, Expect(EMHErrorCode::Cancelled));
    Observed->Providers.Pop(); Destroyed.Reset();
    TestEqual(TEXT("Never inline"), Observed->Calls, 0);
    ADD_LATENT_AUTOMATION_COMMAND(FWaitSteam([this, Observed]()
    {
        if (Observed->Calls < 11) { return false; }
        TestEqual(TEXT("One result per request"), Observed->Calls, 11);
        for (const auto& Provider : Observed->Providers) { Provider->Shutdown(); }
        int32 CancelCalls = 0;
        for (const auto& Client : Observed->Clients) { CancelCalls += Client->CancelCalls; }
        TestEqual(TEXT("All seven native handles revoked exactly once"), CancelCalls, 7);
        return true;
    }, this));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMHSteamLiveTest, "MH.Online.SteamLive",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)
bool FMHSteamLiveTest::RunTest(const FString& Parameters)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("MHSteamLiveTest")))
    { AddWarning(TEXT("Steam live test not run: requires -MHSteamLiveTest and a signed-in Steam client")); return true; }
    auto Provider = MakeShared<FMHSteamIdentityProvider>();
    auto Done = MakeShared<bool>(false);
    Provider->GetLocalAccount(0, [this, Provider, Done](TMHResult<FMHAccountId> Account)
    {
        if (!Account.GetValue())
        {
            AddError(TEXT("Live Steam local account unavailable"));
            Provider->Shutdown(); *Done = true; return;
        }
        auto Count = MakeShared<int32>(0);
        for (const FString& Audience : {FString(TEXT("webapi:mh-framework-test")), FString(TEXT("session:")) + Account.GetValue()->Subject})
        {
            Provider->AcquireAuthTicket({0, Audience, 25},
                [this, Provider, Done, Count](TMHResult<FMHAuthTicketRef> Result)
            {
                TestTrue(TEXT("Live Steam ticket acquisition"), Result.GetValue() && !(*Result.GetValue())->GetBytes().IsEmpty());
                if (Result.GetValue())
                {
                    (*Result.GetValue())->Release();
                    TestTrue(TEXT("Live ticket lease released"), (*Result.GetValue())->IsReleased());
                }
                else if (Result.GetError()) { AddError(FString::Printf(TEXT("Steam acquisition error code %d"), int32(Result.GetError()->Code))); }
                if (++*Count == 2) { Provider->Shutdown(); *Done = true; }
            });
        }
    });
    ADD_LATENT_AUTOMATION_COMMAND(FWaitSteam([Provider, Done]() { return *Done; }, this));
    return true;
}
#endif
