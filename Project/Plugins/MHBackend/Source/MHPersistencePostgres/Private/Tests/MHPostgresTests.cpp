#include "MHPostgresStore.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#if WITH_MH_POSTGRES
#include "libpq-fe.h"
#endif

namespace
{
    class FWaitForPostgres final : public IAutomationLatentCommand
    {
        TFunction<bool()> Check;
        FAutomationTestBase* Test;
        double Deadline = FPlatformTime::Seconds() + 45.0;
    public:
        FWaitForPostgres(TFunction<bool()> InCheck, FAutomationTestBase* InTest) : Check(MoveTemp(InCheck)), Test(InTest) {}
        bool Update() override
        {
            if (Check()) { return true; }
            if (FPlatformTime::Seconds() > Deadline) { Test->AddError(TEXT("Postgres test timed out")); return true; }
            return false;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMHPostgresTest, "MH.Persistence.PostgresContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMHPostgresTest::RunTest(const FString& Parameters)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("MHPostgresTest")))
    {
        AddInfo(TEXT("Not run: requires -MHPostgresTest and disposable local fixture"));
        return true;
    }
#if !WITH_MH_POSTGRES
    AddError(TEXT("Build with MH_POSTGRES_ROOT to run PostgreSQL integration tests"));
    return false;
#endif
    struct FObserved
    {
        int32 Calls = 0;
        TArray<FString> Failures;
        bool bReopened = false;
        TSharedPtr<FMHPostgresStore> Reopened;
        void Record(bool bOk, const TCHAR* Context)
        {
            ++Calls;
            if (!bOk || !IsInGameThread()) { Failures.Add(Context); }
        }
    };
    const auto Observed = MakeShared<FObserved>();
    FMHPostgresConfig Config;
    Config.ConnectionString = TEXT("host=127.0.0.1 port=55432 dbname=mh_test user=mh_test password=mh_local_test_only sslmode=disable");
    const FString Namespace = TEXT("mh-test-") + FGuid::NewGuid().ToString();
    const auto Store = MakeShared<FMHPostgresStore>(Config);
    const FMHRecordKey Key{Namespace, TEXT("\u00e9\u4e16\u754c quoted'; DROP TABLE mh_records_v1;--")};
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
    Store->Load({Namespace.ToUpper(), Key.Id}, Missing);
    Store->Load({Namespace + TEXT("other"), Key.Id}, Missing);
    Store->Put(Key, {}, Success);
    Store->Load(Key, [Observed](TMHResult<TArray<uint8>> Result)
    {
        Observed->Record(Result.GetValue() && Result.GetValue()->IsEmpty(), TEXT("Empty BLOB"));
    });
    Store->Delete(Key, Success);
    Store->Delete(Key, Success);
    Store->Load(Key, Missing);
    Store->Put({Namespace, TEXT("durable")}, {7, 8, 9}, Success);
    const auto CancelHandle = Store->Put({Namespace, TEXT("cancelled")}, {1}, Cancelled);
    TestTrue(TEXT("Cancel queued write"), CancelHandle.Cancel());
    Store->Load({Namespace, TEXT("cancelled")}, Missing);
    Store->Put({TEXT(""), TEXT("invalid")}, {1}, [Observed](TMHResult<FMHUnit> Result)
    {
        Observed->Record(Result.GetError() && Result.GetError()->Code == EMHErrorCode::InvalidArgument, TEXT("Invalid key"));
    });
    {
        FMHPostgresStore Invalid({});
        Invalid.Load(Key, [Observed](TMHResult<TArray<uint8>> Result)
        {
            Observed->Record(Result.GetError() && Result.GetError()->Code == EMHErrorCode::NotConfigured, TEXT("Invalid configuration"));
        });
        FMHPostgresStore Closing(Config);
        Closing.Put(Key, {1}, Cancelled);
        Closing.Shutdown();
        Closing.Shutdown();
        Closing.Load(Key, [Observed](TMHResult<TArray<uint8>> Result)
        {
            Observed->Record(Result.GetError() && Result.GetError()->Code == EMHErrorCode::Unavailable, TEXT("Closed store"));
        });
        FMHPostgresStore Destroyed(Config);
        Destroyed.Put(Key, {1}, Cancelled);
    }
    FMHPostgresConfig BadConfig = Config;
    BadConfig.ConnectionString += TEXT(" password=deliberately_wrong");
    const auto BadPath = MakeShared<FMHPostgresStore>(BadConfig);
    BadPath->Load(Key, [Observed](TMHResult<TArray<uint8>> Result)
    {
        Observed->Record(Result.GetError() && Result.GetError()->Code == EMHErrorCode::Unavailable, TEXT("Authentication failure without fallback"));
    });
    TestEqual(TEXT("No inline completion"), Observed->Calls, 0);
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForPostgres([this, Store, BadPath, Config, Namespace, Observed]()
    {
        if (Observed->Calls < 18 || Store->HasPendingIO() || BadPath->HasPendingIO()) { return false; }
        if (!Observed->bReopened)
        {
            Store->Shutdown();
            Observed->bReopened = true;
            Observed->Reopened = MakeShared<FMHPostgresStore>(Config);
            Observed->Reopened->Load({Namespace, TEXT("durable")}, [Observed](TMHResult<TArray<uint8>> Result)
            {
                Observed->Record(Result.GetValue() && *Result.GetValue() == TArray<uint8>({7, 8, 9}), TEXT("Reopen durability"));
            });
            return false;
        }
        if (Observed->Calls < 19 || Observed->Reopened->HasPendingIO()) { return false; }
        if (Observed->Calls == 19)
        {
            Observed->Reopened->Delete({Namespace, TEXT("durable")}, [Observed](TMHResult<FMHUnit> Result)
            {
                Observed->Record(Result.IsSuccess(), TEXT("Remove durable fixture record"));
            });
            return false;
        }
        TestEqual(TEXT("Exactly one result per operation"), Observed->Calls, 20);
        for (const FString& Failure : Observed->Failures) { AddError(Failure); }
        Observed->Reopened->Shutdown();
        Observed->Reopened.Reset();
        return true;
    }, this));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMHPostgresTimeoutTest, "MH.Persistence.PostgresTimeout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMHPostgresTimeoutTest::RunTest(const FString& Parameters)
{
    if (!FParse::Param(FCommandLine::Get(), TEXT("MHPostgresTest")))
    {
        AddInfo(TEXT("Not run: requires -MHPostgresTest and disposable local fixture"));
        return true;
    }
#if WITH_MH_POSTGRES
    struct FObserved
    {
        PGconn* Blocker = nullptr;
        bool bComplete = false;
        bool bTimedOut = false;
        ~FObserved() { if (Blocker) PQfinish(Blocker); }
    };
    const auto Observed = MakeShared<FObserved>();
    // Test-only connection holds the provider's schema lock to force a real server timeout.
    Observed->Blocker = PQconnectdb("host=127.0.0.1 port=55432 dbname=mh_test user=mh_test "
        "password=mh_local_test_only sslmode=disable connect_timeout=5 options='-c statement_timeout=1000'");
    if (!Observed->Blocker || PQstatus(Observed->Blocker) != CONNECTION_OK)
    {
        AddError(TEXT("Could not connect fixture lock holder"));
        return false;
    }
    PGresult* Lock = PQexec(Observed->Blocker, "SELECT pg_try_advisory_lock(1296584786,1)");
    const bool bLocked = Lock && PQresultStatus(Lock) == PGRES_TUPLES_OK &&
        PQntuples(Lock) == 1 && FCStringAnsi::Strcmp(PQgetvalue(Lock, 0, 0), "t") == 0;
    if (Lock) PQclear(Lock);
    if (!bLocked) { AddError(TEXT("Fixture schema lock already held")); return false; }
    FMHPostgresConfig Config;
    Config.ConnectionString = TEXT("host=127.0.0.1 port=55432 dbname=mh_test user=mh_test password=mh_local_test_only sslmode=disable");
    Config.StatementTimeoutMilliseconds = 100;
    const auto Store = MakeShared<FMHPostgresStore>(Config);
    Store->Load({TEXT("timeout-test"), TEXT("absent")}, [Observed](TMHResult<TArray<uint8>> Result)
    {
        Observed->bComplete = true;
        Observed->bTimedOut = IsInGameThread() && Result.GetError() && Result.GetError()->Code == EMHErrorCode::Timeout;
    });
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForPostgres([this, Store, Observed]()
    {
        if (!Observed->bComplete || Store->HasPendingIO()) return false;
        TestTrue(TEXT("Server lock/statement timeout maps to Timeout"), Observed->bTimedOut);
        Store->Shutdown();
        PQfinish(Observed->Blocker);
        Observed->Blocker = nullptr;
        return true;
    }, this));
    return true;
#else
    AddError(TEXT("Build with MH_POSTGRES_ROOT to run PostgreSQL integration tests"));
    return false;
#endif
}
#endif
