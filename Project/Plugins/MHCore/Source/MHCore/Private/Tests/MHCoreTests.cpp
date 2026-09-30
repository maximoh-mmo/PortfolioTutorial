#include "MHRequest.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Async/Async.h"
#include "HAL/PlatformTime.h"
#include "Misc/AutomationTest.h"
#include <atomic>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMHResultTest, "MH.Core.Result",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMHResultTest::RunTest(const FString& Parameters)
{
    const auto Success = TMHResult<int32>::Success(42);
    TestTrue(TEXT("Success has value only"), Success.GetValue() && !Success.GetError());
    TestEqual(TEXT("Payload preserved"), *Success.GetValue(), 42);
    const auto Failure = TMHResult<int32>::Failure({EMHErrorCode::InvalidArgument, TEXT("Invalid input")});
    TestTrue(TEXT("Failure has error only"), Failure.GetError() && !Failure.GetValue());
    TestTrue(TEXT("Error code preserved"), Failure.GetError()->Code == EMHErrorCode::InvalidArgument);
    const auto MoveOnly = TMHResult<TUniquePtr<int32>>::Success(MakeUnique<int32>(7));
    TestEqual(TEXT("Move-only payload supported"), **MoveOnly.GetValue(), 7);
    TestTrue(TEXT("Void-like success"), TMHResult<FMHUnit>::Success({}).IsSuccess());
    FMHRequestHandle Empty;
    TestFalse(TEXT("Empty handle is invalid"), Empty.IsValid());
    TestFalse(TEXT("Empty handle is not pending"), Empty.IsPending());
    TestFalse(TEXT("Empty handle cannot cancel"), Empty.Cancel());
    return true;
}

namespace
{
    class FMHWaitForCallbacks final : public IAutomationLatentCommand
    {
        TFunction<bool()> Check;
        FAutomationTestBase* Test;
        double Deadline = FPlatformTime::Seconds() + 10.0;
    public:
        FMHWaitForCallbacks(TFunction<bool()> InCheck, FAutomationTestBase* InTest)
            : Check(MoveTemp(InCheck)), Test(InTest) {}
        bool Update() override
        {
            if (Check()) { return true; }
            if (FPlatformTime::Seconds() > Deadline)
            {
                Test->AddError(TEXT("Timed out waiting for MH completion callbacks"));
                return true;
            }
            return false;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMHRequestTest, "MH.Core.RequestLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMHRequestTest::RunTest(const FString& Parameters)
{
    struct FObserved
    {
        int32 Calls = 0;
        int32 OwnerCalls = 0;
        bool bGameThread = true;
        bool bResultsCorrect = true;
        std::atomic<int32> RaceWinners{0};
    };
    const auto Observed = MakeShared<FObserved, ESPMode::ThreadSafe>();
    auto Observe = [Observed](bool bCorrect)
    {
        ++Observed->Calls;
        Observed->bGameThread &= IsInGameThread();
        Observed->bResultsCorrect &= bCorrect;
    };

    TMHRequest<int32> Success([Observe](TMHResult<int32> Result)
    {
        Observe(Result.GetValue() && *Result.GetValue() == 42);
    });
    TestTrue(TEXT("First success accepted"), Success.Complete(TMHResult<int32>::Success(42)));
    TestFalse(TEXT("Duplicate rejected"), Success.Complete(TMHResult<int32>::Success(99)));
    TestFalse(TEXT("Cancellation after success rejected"), Success.GetHandle().Cancel());
    TestFalse(TEXT("Resolved request no longer pending"), Success.GetHandle().IsPending());

    TMHRequest<int32> Cancelled([Observe](TMHResult<int32> Result)
    {
        Observe(Result.GetError() && Result.GetError()->Code == EMHErrorCode::Cancelled);
    });
    const auto HandleCopy = Cancelled.GetHandle();
    TestTrue(TEXT("Pending before cancellation"), HandleCopy.IsPending());
    TestTrue(TEXT("Cancellation accepted"), HandleCopy.Cancel());
    TestFalse(TEXT("Repeated cancellation rejected"), Cancelled.GetHandle().Cancel());
    TestFalse(TEXT("Late provider success rejected"), Cancelled.Complete(TMHResult<int32>::Success(1)));

    TMHRequest<int32> Invalid([Observe](TMHResult<int32> Result)
    {
        Observe(Result.GetError() && Result.GetError()->Code == EMHErrorCode::InvalidArgument);
    });
    Invalid.Complete(TMHResult<int32>::Failure({EMHErrorCode::InvalidArgument, TEXT("Invalid input")}));

    TSharedPtr<int32, ESPMode::ThreadSafe> Owner = MakeShared<int32, ESPMode::ThreadSafe>(1);
    TWeakPtr<int32, ESPMode::ThreadSafe> WeakOwner = Owner;
    TMHRequest<int32> Teardown([Observe, Observed, WeakOwner](TMHResult<int32> Result)
    {
        if (WeakOwner.Pin()) { ++Observed->OwnerCalls; }
        Observe(Result.GetError() && Result.GetError()->Code == EMHErrorCode::Cancelled);
    });
    Teardown.GetHandle().Cancel();
    Owner.Reset();

    constexpr int32 Races = 16;
    for (int32 Index = 0; Index < Races; ++Index)
    {
        TMHRequest<int32> Race([Observe](TMHResult<int32> Result)
        {
            Observe((Result.GetValue() && *Result.GetValue() == 8) ||
                (Result.GetError() && Result.GetError()->Code == EMHErrorCode::Cancelled));
        });
        auto Complete = Async(EAsyncExecution::ThreadPool, [Race, Observed]()
        {
            if (Race.Complete(TMHResult<int32>::Success(8))) { ++Observed->RaceWinners; }
        });
        auto Cancel = Async(EAsyncExecution::ThreadPool, [Race, Observed]()
        {
            if (Race.GetHandle().Cancel()) { ++Observed->RaceWinners; }
        });
        Complete.Wait();
        Cancel.Wait();
    }
    TestEqual(TEXT("Callbacks never run inline"), Observed->Calls, 0);
    TestEqual(TEXT("One winner per race"), Observed->RaceWinners.load(), Races);
    ADD_LATENT_AUTOMATION_COMMAND(FMHWaitForCallbacks([this, Observed]()
    {
        if (Observed->Calls < 20) { return false; }
        TestEqual(TEXT("Exactly one callback per request"), Observed->Calls, 20);
        TestTrue(TEXT("All callbacks on game thread"), Observed->bGameThread);
        TestTrue(TEXT("Terminal results preserved"), Observed->bResultsCorrect);
        TestEqual(TEXT("Destroyed owner is not invoked"), Observed->OwnerCalls, 0);
        return true;
    }, this));
    return true;
}
#endif
