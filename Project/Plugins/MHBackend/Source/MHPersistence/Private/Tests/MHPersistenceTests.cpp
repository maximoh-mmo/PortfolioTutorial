#include "MHMemoryStore.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "HAL/PlatformTime.h"

namespace
{
    class FWaitForPersistence final : public IAutomationLatentCommand
    {
        TFunction<bool()> Check;
        FAutomationTestBase* Test;
        double Deadline = FPlatformTime::Seconds() + 10.0;
    public:
        FWaitForPersistence(TFunction<bool()> InCheck, FAutomationTestBase* InTest)
            : Check(MoveTemp(InCheck)), Test(InTest) {}
        bool Update() override
        {
            if (Check()) { return true; }
            if (FPlatformTime::Seconds() > Deadline) { Test->AddError(TEXT("Persistence callbacks timed out")); return true; }
            return false;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMHPersistenceTest, "MH.Persistence.MemoryContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMHPersistenceTest::RunTest(const FString& Parameters)
{
    struct FObserved
    {
        int32 Calls = 0;
        bool bCorrect = true;
        void Record(bool bResult) { ++Calls; bCorrect &= bResult && IsInGameThread(); }
    };
    const auto Observed = MakeShared<FObserved>();
    const auto Store = MakeShared<FMHMemoryStore>();
    auto Success = [Observed](TMHResult<FMHUnit> Result) { Observed->Record(Result.IsSuccess()); };
    auto Missing = [Observed](TMHResult<TArray<uint8>> Result)
    {
        Observed->Record(Result.GetError() && Result.GetError()->Code == EMHErrorCode::NotFound);
    };
    auto Cancelled = [Observed](TMHResult<FMHUnit> Result)
    {
        Observed->Record(Result.GetError() && Result.GetError()->Code == EMHErrorCode::Cancelled);
    };
    Store->Load({TEXT("players"), TEXT("one")}, Missing);
    Store->Put({TEXT("players"), TEXT("one")}, {0, 255, 42}, Success);
    Store->Load({TEXT("players"), TEXT("one")}, [Observed](TMHResult<TArray<uint8>> Result)
    {
        Observed->Record(Result.GetValue() && *Result.GetValue() == TArray<uint8>({0, 255, 42}));
    });
    Store->Load({TEXT("other"), TEXT("one")}, Missing);
    Store->Load({TEXT("players"), TEXT("One")}, Missing);
    Store->Put({TEXT("players"), TEXT("one")}, {}, Success);
    Store->Load({TEXT("players"), TEXT("one")}, [Observed](TMHResult<TArray<uint8>> Result)
    {
        Observed->Record(Result.GetValue() && Result.GetValue()->IsEmpty());
    });
    Store->Delete({TEXT("players"), TEXT("one")}, Success);
    Store->Delete({TEXT("players"), TEXT("one")}, Success);
    Store->Load({TEXT("players"), TEXT("one")}, Missing);
    const auto Handle = Store->Put({TEXT("players"), TEXT("cancelled")}, {1}, Cancelled);
    TestTrue(TEXT("Cancellation accepted before queued mutation"), Handle.Cancel());
    Store->Load({TEXT("players"), TEXT("cancelled")}, Missing);
    Store->Put({TEXT(""), TEXT("bad")}, {1}, [Observed](TMHResult<FMHUnit> Result)
    {
        Observed->Record(Result.GetError() && Result.GetError()->Code == EMHErrorCode::InvalidArgument);
    });
    {
        FMHMemoryStore Closing;
        Closing.Put({TEXT("records"), TEXT("one")}, {1}, Cancelled);
        Closing.Shutdown();
        Closing.Shutdown();
        Closing.Load({TEXT("records"), TEXT("one")}, [Observed](TMHResult<TArray<uint8>> Result)
        {
            Observed->Record(Result.GetError() && Result.GetError()->Code == EMHErrorCode::Unavailable);
        });
    }
    {
        FMHMemoryStore Destroyed;
        Destroyed.Put({TEXT("records"), TEXT("one")}, {1}, Cancelled);
    }
    TestEqual(TEXT("No inline completion"), Observed->Calls, 0);
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForPersistence([this, Store, Observed]()
    {
        if (Observed->Calls < 16) { return false; }
        TestEqual(TEXT("Exactly one callback per operation"), Observed->Calls, 16);
        TestTrue(TEXT("CRUD, key isolation, errors and cancellation semantics"), Observed->bCorrect);
        Store->Shutdown();
        return true;
    }, this));
    return true;
}
#endif
