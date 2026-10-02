#include "MHHttpRecordStore.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Containers/Ticker.h"


namespace
{
    class FWaitForHttpRecords final : public IAutomationLatentCommand
    {
        TFunction<bool()> Check;
        FAutomationTestBase* Test;
        double Deadline = FPlatformTime::Seconds() + 45.0;
    public:
        FWaitForHttpRecords(TFunction<bool()> InCheck, FAutomationTestBase* InTest) : Check(MoveTemp(InCheck)), Test(InTest) {}
        bool Update() override
        {
            if (Check()) { return true; }
            if (FPlatformTime::Seconds() > Deadline) { Test->AddError(TEXT("HttpRecords test timed out")); return true; }
            return false;
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMHHttpRecordsTest, "MH.Persistence.HttpRecordsContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMHHttpRecordsTest::RunTest(const FString& Parameters)
{
    FString BaseUrl;
    if (!FParse::Value(FCommandLine::Get(), TEXT("MHRecordTestBaseUrl="), BaseUrl))
    {
        AddInfo(TEXT("Not run: requires -MHRecordTestBaseUrl and the local record fixture"));
        return true;
    }
    struct FObserved
    {
        int32 Calls = 0;
        TArray<FString> Failures;
        bool bReopened = false;
        TSharedPtr<FMHHttpRecordStore> Reopened;
        void Record(bool bOk, const TCHAR* Context)
        {
            ++Calls;
            if (!bOk || !IsInGameThread()) { Failures.Add(Context); }
        }
    };
    const auto Observed = MakeShared<FObserved>();
    FMHHttpRecordConfig Config;
    Config.BaseUrl = BaseUrl + TEXT("/");
    Config.Headers.Add(TEXT("Authorization"), TEXT("Bearer mh-local-test-only"));
    const FString Namespace = TEXT("mh-test-") + FGuid::NewGuid().ToString();
    const auto Store = MakeShared<FMHHttpRecordStore>(Config);
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
        FMHHttpRecordStore Invalid({});
        Invalid.Load(Key, [Observed](TMHResult<TArray<uint8>> Result)
        {
            Observed->Record(Result.GetError() && Result.GetError()->Code == EMHErrorCode::NotConfigured, TEXT("Invalid configuration"));
        });
        FMHHttpRecordStore Closing(Config);
        Closing.Put(Key, {1}, Cancelled);
        Closing.Shutdown();
        Closing.Shutdown();
        Closing.Load(Key, [Observed](TMHResult<TArray<uint8>> Result)
        {
            Observed->Record(Result.GetError() && Result.GetError()->Code == EMHErrorCode::Unavailable, TEXT("Closed store"));
        });
        FMHHttpRecordStore Destroyed(Config);
        Destroyed.Put(Key, {1}, Cancelled);
    }
    FMHHttpRecordConfig BadConfig = Config;
    BadConfig.Headers.Add(TEXT("Authorization"), TEXT("Bearer wrong"));
    const auto BadPath = MakeShared<FMHHttpRecordStore>(BadConfig);
    BadPath->Load(Key, [Observed](TMHResult<TArray<uint8>> Result)
    {
        Observed->Record(Result.GetError() && Result.GetError()->Code == EMHErrorCode::Unauthorized, TEXT("Authentication failure without fallback"));
    });
    TestEqual(TEXT("No inline completion"), Observed->Calls, 0);
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForHttpRecords([this, Store, BadPath, Config, Namespace, Observed]()
    {
        if (Observed->Calls < 18 || Store->HasPendingIO() || BadPath->HasPendingIO()) { return false; }
        if (!Observed->bReopened)
        {
            Store->Shutdown();
            Observed->bReopened = true;
            Observed->Reopened = MakeShared<FMHHttpRecordStore>(Config);
            Observed->Reopened->Load({Namespace, TEXT("durable")}, [Observed](TMHResult<TArray<uint8>> Result)
            {
                Observed->Record(Result.GetValue() && *Result.GetValue() == TArray<uint8>({7, 8, 9}), TEXT("Records survive reopening the client"));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMHHttpRecordFailuresTest, "MH.Persistence.HttpRecordsFailures",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMHHttpRecordFailuresTest::RunTest(const FString& Parameters)
{
    FString BaseUrl;
    if (!FParse::Value(FCommandLine::Get(), TEXT("MHRecordTestBaseUrl="), BaseUrl))
    {
        AddInfo(TEXT("Not run: requires -MHRecordTestBaseUrl and the local record fixture"));
        return true;
    }
    struct FObserved { int32 Calls = 0; bool bCorrect = true; };
    const auto Observed = MakeShared<FObserved>();
    TArray<TSharedPtr<FMHHttpRecordStore>> Stores;
    auto MakeStore = [&Stores, BaseUrl](const TCHAR* Suffix, float Timeout)
    {
        FMHHttpRecordConfig Config;
        Config.BaseUrl = BaseUrl + Suffix;
        Config.TimeoutSeconds = Timeout;
        Config.Headers.Add(TEXT("Authorization"), TEXT("Bearer mh-local-test-only"));
        auto Store = MakeShared<FMHHttpRecordStore>(Config);
        Stores.Add(Store);
        return Store;
    };
    auto Expect = [Observed](EMHErrorCode Code)
    {
        return [Observed, Code](TMHResult<TArray<uint8>> Result)
        {
            ++Observed->Calls;
            Observed->bCorrect &= IsInGameThread() && Result.GetError() && Result.GetError()->Code == Code;
        };
    };
    const FMHRecordKey Key{TEXT("fault"), TEXT("key")};
    MakeStore(TEXT("/unexpected"), 5)->Load(Key, Expect(EMHErrorCode::Internal));
    MakeStore(TEXT("/unavailable"), 5)->Load(Key, Expect(EMHErrorCode::Unavailable));
    AddExpectedMessagePlain(TEXT("HTTP request timed out after 0.20 seconds URL=") +
        BaseUrl + TEXT("/slow/v1/records/ZmF1bHQ/a2V5"), ELogVerbosity::Warning);
    MakeStore(TEXT("/slow"), 0.2f)->Load(Key, Expect(EMHErrorCode::Timeout));
    const auto Cancelling = MakeStore(TEXT("/slow"), 5);
    const auto Handle = Cancelling->Load(Key, Expect(EMHErrorCode::Cancelled));
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Handle](float)
    {
        Handle.Cancel();
        return false;
    }), 0.1f);
    FMHHttpRecordConfig Unsafe;
    Unsafe.BaseUrl = TEXT("http://example.com");
    const auto Invalid = MakeShared<FMHHttpRecordStore>(Unsafe);
    Stores.Add(Invalid);
    Invalid->Load(Key, Expect(EMHErrorCode::NotConfigured));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForHttpRecords([this, Observed, Stores]()
    {
        if (Observed->Calls < 5) return false;
        for (const auto& Store : Stores) { if (Store->HasPendingIO()) return false; }
        TestEqual(TEXT("One completion per error/cancelled request"), Observed->Calls, 5);
        TestTrue(TEXT("Expected errors on game thread"), Observed->bCorrect);
        for (const auto& Store : Stores) Store->Shutdown();
        return true;
    }, this));
    return true;
}
#endif
