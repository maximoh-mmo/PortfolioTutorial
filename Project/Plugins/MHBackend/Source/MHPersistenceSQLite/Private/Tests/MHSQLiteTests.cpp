#include "MHSQLiteStore.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

namespace
{
    class FWaitForSQLite final : public IAutomationLatentCommand
    {
        TFunction<bool()> Check;
        FAutomationTestBase* Test;
        double Deadline = FPlatformTime::Seconds() + 15.0;
    public:
        FWaitForSQLite(TFunction<bool()> InCheck, FAutomationTestBase* InTest) : Check(MoveTemp(InCheck)), Test(InTest) {}
        bool Update() override
        {
            if (Check()) { return true; }
            if (FPlatformTime::Seconds() > Deadline) { Test->AddError(TEXT("SQLite test timed out")); return true; }
            return false;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMHSQLiteTest, "MH.Persistence.SQLiteContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMHSQLiteTest::RunTest(const FString& Parameters)
{
    struct FObserved
    {
        int32 Calls = 0;
        TArray<FString> Failures;
        bool bReopened = false;
        TSharedPtr<FMHSQLiteStore> Reopened;
        void Record(bool bOk, const TCHAR* Context)
        {
            ++Calls;
            if (!bOk || !IsInGameThread()) { Failures.Add(Context); }
        }
    };
    const auto Observed = MakeShared<FObserved>();
    const FString Filename = FPaths::ProjectSavedDir() / (TEXT("MHSQLite-") + FGuid::NewGuid().ToString() + TEXT(".db"));
    const auto Store = MakeShared<FMHSQLiteStore>(Filename);
    const FMHRecordKey Key{TEXT("records"), TEXT("quoted'; DROP TABLE mh_records_v1;--")};
    auto Success = [Observed](TMHResult<FMHUnit> Result) { Observed->Record(Result.IsSuccess(), TEXT("Write/delete success")); };
    auto Missing = [Observed](TMHResult<TArray<uint8>> Result)
    {
        Observed->Record(Result.GetError() && Result.GetError()->Code == EMHErrorCode::NotFound, TEXT("Missing record"));
    };
    auto Cancelled = [Observed](TMHResult<FMHUnit> Result)
    {
        Observed->Record(Result.GetError() && Result.GetError()->Code == EMHErrorCode::Cancelled, TEXT("Cancellation"));
    };
    Store->Put(Key, {0, 255, 42}, Success);
    Store->Load(Key, [Observed](TMHResult<TArray<uint8>> Result)
    {
        Observed->Record(Result.GetValue() && *Result.GetValue() == TArray<uint8>({0, 255, 42}), TEXT("Binary round trip"));
    });
    Store->Load({TEXT("Records"), Key.Id}, Missing);
    Store->Load({TEXT("other"), Key.Id}, Missing);
    Store->Put(Key, {}, Success);
    Store->Load(Key, [Observed](TMHResult<TArray<uint8>> Result)
    {
        Observed->Record(Result.GetValue() && Result.GetValue()->IsEmpty(), TEXT("Empty BLOB"));
    });
    Store->Delete(Key, Success);
    Store->Delete(Key, Success);
    Store->Load(Key, Missing);
    Store->Put({TEXT("records"), TEXT("durable")}, {7, 8, 9}, Success);
    const auto CancelHandle = Store->Put({TEXT("records"), TEXT("cancelled")}, {1}, Cancelled);
    TestTrue(TEXT("Cancel queued write"), CancelHandle.Cancel());
    Store->Load({TEXT("records"), TEXT("cancelled")}, Missing);
    Store->Put({TEXT(""), TEXT("invalid")}, {1}, [Observed](TMHResult<FMHUnit> Result)
    {
        Observed->Record(Result.GetError() && Result.GetError()->Code == EMHErrorCode::InvalidArgument, TEXT("Invalid key"));
    });
    {
        FMHSQLiteStore Invalid(TEXT(""));
        Invalid.Load(Key, [Observed](TMHResult<TArray<uint8>> Result)
        {
            Observed->Record(Result.GetError() && Result.GetError()->Code == EMHErrorCode::NotConfigured, TEXT("Invalid filename"));
        });
        FMHSQLiteStore Closing(Filename);
        Closing.Put(Key, {1}, Cancelled);
        Closing.Shutdown();
        Closing.Shutdown();
        Closing.Load(Key, [Observed](TMHResult<TArray<uint8>> Result)
        {
            Observed->Record(Result.GetError() && Result.GetError()->Code == EMHErrorCode::Unavailable, TEXT("Closed store"));
        });
        FMHSQLiteStore Destroyed(Filename);
        Destroyed.Put(Key, {1}, Cancelled);
    }
    const auto BadPath = MakeShared<FMHSQLiteStore>(FPaths::ProjectSavedDir() / FGuid::NewGuid().ToString() / TEXT("missing.db"));
    BadPath->Load(Key, [Observed](TMHResult<TArray<uint8>> Result)
    {
        Observed->Record(Result.GetError() && Result.GetError()->Code == EMHErrorCode::Unavailable, TEXT("No in-memory fallback on I/O error"));
    });
    TestEqual(TEXT("No inline completion"), Observed->Calls, 0);
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForSQLite([this, Store, BadPath, Filename, Observed]()
    {
        if (Observed->Calls < 18 || Store->HasPendingIO() || BadPath->HasPendingIO()) { return false; }
        if (!Observed->bReopened)
        {
            Store->Shutdown();
            Observed->bReopened = true;
            Observed->Reopened = MakeShared<FMHSQLiteStore>(Filename);
            Observed->Reopened->Load({TEXT("records"), TEXT("durable")}, [Observed](TMHResult<TArray<uint8>> Result)
            {
                Observed->Record(Result.GetValue() && *Result.GetValue() == TArray<uint8>({7, 8, 9}), TEXT("Reopen durability"));
            });
            return false;
        }
        if (Observed->Calls < 19 || Observed->Reopened->HasPendingIO()) { return false; }
        TestEqual(TEXT("Exactly one result per operation"), Observed->Calls, 19);
        for (const FString& Failure : Observed->Failures) { AddError(Failure); }
        Observed->Reopened->Shutdown();
        Observed->Reopened.Reset();
        TestTrue(TEXT("Closed test database removed"), IFileManager::Get().Delete(*Filename));
        return true;
    }, this));
    return true;
}
#endif
