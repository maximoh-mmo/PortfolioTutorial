#include "MHHttpClientState.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Async/Async.h"
#include "HAL/PlatformTime.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
    class FWaitForHttp final : public IAutomationLatentCommand
    {
        TFunction<bool()> Check;
        FAutomationTestBase* Test;
        double Deadline = FPlatformTime::Seconds() + 15.0;
    public:
        FWaitForHttp(TFunction<bool()> InCheck, FAutomationTestBase* InTest)
            : Check(MoveTemp(InCheck)), Test(InTest) {}
        bool Update() override
        {
            if (Check()) { return true; }
            if (FPlatformTime::Seconds() > Deadline)
            {
                Test->AddError(TEXT("Timed out waiting for HTTP callbacks"));
                return true;
            }
            return false;
        }
    };

    class FFakeOperation final : public MH::Private::IHttpOperation
    {
    public:
        bool bStart = true;
        int32 Cancels = 0;
        TUniqueFunction<void(TMHResult<FMHHttpResponse>)> Callback;
        bool Start(TUniqueFunction<void(TMHResult<FMHHttpResponse>)>&& Completion) override
        {
            Callback = MoveTemp(Completion);
            return bStart;
        }
        void Cancel() override { ++Cancels; }
        void Respond(int32 Status)
        {
            FMHHttpResponse Response;
            Response.StatusCode = Status;
            Response.Body = {1, 2, 3};
            Response.Headers = {TEXT("X-Test: fixture")};
            Callback(TMHResult<FMHHttpResponse>::Success(MoveTemp(Response)));
        }
    };
    struct FObserved
    {
        int32 Calls = 0;
        bool bCorrect = true;
        void Record(bool bValue)
        {
            ++Calls;
            bCorrect &= bValue && IsInGameThread();
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMHHttpStatusTest, "MH.Backend.Http.StatusMapping",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMHHttpStatusTest::RunTest(const FString& Parameters)
{
    const TPair<int32, EMHErrorCode> Cases[] = {
        {400, EMHErrorCode::InvalidArgument}, {401, EMHErrorCode::Unauthorized},
        {403, EMHErrorCode::Unauthorized}, {404, EMHErrorCode::NotFound},
        {409, EMHErrorCode::Conflict}, {408, EMHErrorCode::Timeout},
        {504, EMHErrorCode::Timeout}, {429, EMHErrorCode::Unavailable},
        {503, EMHErrorCode::Unavailable}, {302, EMHErrorCode::Internal}
    };
    for (const auto& Case : Cases)
    {
        FMHHttpResponse Response;
        Response.StatusCode = Case.Key;
        const auto Error = Response.GetStatusError();
        TestTrue(TEXT("Non-2xx status mapped"), Error.IsSet() && Error->Code == Case.Value);
    }
    FMHHttpResponse Success;
    Success.StatusCode = 204;
    TestFalse(TEXT("2xx has no status error"), Success.GetStatusError().IsSet());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMHHttpLifecycleTest, "MH.Backend.Http.Lifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMHHttpLifecycleTest::RunTest(const FString& Parameters)
{
    using namespace MH::Private;
    const auto Observed = MakeShared<FObserved>();
    const auto Operations = MakeShared<TArray<TSharedRef<FFakeOperation, ESPMode::ThreadSafe>>>();
    const auto State = MakeShared<FHttpClientState, ESPMode::ThreadSafe>([Operations](const FMHHttpRequest& Request)
    {
        const auto Operation = MakeShared<FFakeOperation, ESPMode::ThreadSafe>();
        Operation->bStart = !Request.Url.EndsWith(TEXT("fail-start"));
        Operations->Add(Operation);
        return Operation;
    });
    FMHHttpRequest Request;
    Request.Url = TEXT("http://localhost/fixture");
    auto ExpectError = [Observed](EMHErrorCode Expected)
    {
        return [Observed, Expected](TMHResult<FMHHttpResponse> Result)
        {
            Observed->Record(Result.GetError() && Result.GetError()->Code == Expected);
        };
    };
    FMHHttpRequest Invalid = Request;
    Invalid.TimeoutSeconds = 0;
    State->Send(Invalid, ExpectError(EMHErrorCode::InvalidArgument));
    Invalid = Request;
    Invalid.Url = TEXT("file:///fixture");
    State->Send(Invalid, ExpectError(EMHErrorCode::InvalidArgument));
    Invalid = Request;
    Invalid.Headers.Add(TEXT("X-Test"), TEXT("unsafe\r\nInjected: yes"));
    State->Send(Invalid, ExpectError(EMHErrorCode::InvalidArgument));
    TestEqual(TEXT("Invalid requests never start I/O"), Operations->Num(), 0);

    Request.Url = TEXT("http://localhost/fail-start");
    State->Send(Request, ExpectError(EMHErrorCode::Unavailable));
    Request.Url = TEXT("http://localhost/fixture");
    const auto Successful = State->Send(Request, [Observed](TMHResult<FMHHttpResponse> Result)
    {
        const auto* Response = Result.GetValue();
        Observed->Record(Response && Response->IsSuccessStatus() && Response->Body.Num() == 3 && Response->Headers.Num() == 1);
    });
    Operations->Last()->Respond(200);
    Operations->Last()->Respond(200);
    TestFalse(TEXT("Cancel after completion loses"), Successful.Cancel());
    TestEqual(TEXT("Normal completion does not cancel I/O"), Operations->Last()->Cancels, 0);

    State->Send(Request, [Observed](TMHResult<FMHHttpResponse> Result)
    {
        Observed->Record(Result.GetValue() && Result.GetValue()->StatusCode == 503 && Result.GetValue()->Body.Num() == 3);
    });
    Operations->Last()->Respond(503);

    const auto Cancelled = State->Send(Request, ExpectError(EMHErrorCode::Cancelled));
    const auto CancelledOperation = Operations->Last();
    auto Worker = Async(EAsyncExecution::ThreadPool, [Cancelled]() { return Cancelled.Cancel(); });
    TestTrue(TEXT("Worker cancellation wins"), Worker.Get());
    TestEqual(TEXT("Transport cleanup not inline on worker"), CancelledOperation->Cancels, 0);

    const auto ClosingState = MakeShared<FHttpClientState, ESPMode::ThreadSafe>([Operations](const FMHHttpRequest&)
    {
        const auto Operation = MakeShared<FFakeOperation, ESPMode::ThreadSafe>();
        Operations->Add(Operation);
        return Operation;
    });
    ClosingState->Send(Request, ExpectError(EMHErrorCode::Cancelled));
    const auto ShutdownOperation = Operations->Last();
    ClosingState->Shutdown();
    ClosingState->Shutdown();
    TestEqual(TEXT("Shutdown cancels I/O once"), ShutdownOperation->Cancels, 1);
    ShutdownOperation->Respond(200);
    const int32 BeforeRejected = Operations->Num();
    ClosingState->Send(Request, ExpectError(EMHErrorCode::Unavailable));
    TestEqual(TEXT("No I/O after shutdown"), Operations->Num(), BeforeRejected);
    {
        const auto TemporaryState = MakeShared<FHttpClientState, ESPMode::ThreadSafe>([Operations](const FMHHttpRequest&)
        {
            const auto Operation = MakeShared<FFakeOperation, ESPMode::ThreadSafe>();
            Operations->Add(Operation);
            return Operation;
        });
        TemporaryState->Send(Request, ExpectError(EMHErrorCode::Cancelled));
    }
    TestEqual(TEXT("Destruction cancels I/O"), Operations->Last()->Cancels, 1);
    TestEqual(TEXT("Callbacks never inline"), Observed->Calls, 0);
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForHttp([this, State, Observed, CancelledOperation]()
    {
        if (Observed->Calls < 10) { return false; }
        TestEqual(TEXT("Exactly one terminal callback"), Observed->Calls, 10);
        TestTrue(TEXT("Correct results on game thread"), Observed->bCorrect);
        TestEqual(TEXT("Worker cancellation cleaned up transport"), CancelledOperation->Cancels, 1);
        CancelledOperation->Respond(200); // Late response cannot win the resolved MHCore gate.
        TestEqual(TEXT("Late response does not call completion inline"), Observed->Calls, 10);
        return true;
    }, this));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMHHttpLoopbackTest, "MH.Backend.Http.Loopback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMHHttpLoopbackTest::RunTest(const FString& Parameters)
{
    FString BaseUrl;
    if (!FParse::Value(FCommandLine::Get(), TEXT("MHHttpTestBaseUrl="), BaseUrl))
    {
        AddInfo(TEXT("Loopback fixture not configured; run with -MHHttpTestBaseUrl=http://127.0.0.1:<port>"));
        return true;
    }
    if (!BaseUrl.StartsWith(TEXT("http://127.0.0.1:")))
    {
        AddError(TEXT("HTTP integration fixture must be on loopback"));
        return false;
    }
    // These diagnostics are caused deliberately by /disconnect and /slow.
    AddExpectedMessage(TEXT("request failed, libcurl error: 52"), ELogVerbosity::Warning);
    AddExpectedMessage(TEXT("libcurl info message cache [0-9]+ \\("), ELogVerbosity::Warning,
        EAutomationExpectedMessageFlags::Contains, 0);
    AddExpectedMessagePlain(BaseUrl + TEXT("/disconnect completed with reason"), ELogVerbosity::Warning);
    AddExpectedMessagePlain(TEXT("HTTP request timed out after 0.20 seconds URL=") + BaseUrl + TEXT("/slow"), ELogVerbosity::Warning);
    const auto Client = MakeShared<FMHHttpClient>();
    const auto Observed = MakeShared<FObserved>();
    FMHHttpRequest Request;
    Request.Url = BaseUrl + TEXT("/ok");
    Client->Send(Request, [Observed](TMHResult<FMHHttpResponse> Result)
    {
        Observed->Record(Result.GetValue() && Result.GetValue()->StatusCode == 200 && Result.GetValue()->Body == TArray<uint8>({111, 107}));
    });
    Request.Url = BaseUrl + TEXT("/missing");
    Client->Send(Request, [Observed](TMHResult<FMHHttpResponse> Result)
    {
        Observed->Record(Result.GetValue() && Result.GetValue()->GetStatusError().IsSet() &&
            Result.GetValue()->GetStatusError()->Code == EMHErrorCode::NotFound);
    });
    Request.Url = BaseUrl + TEXT("/echo");
    Request.Verb = TEXT("POST");
    Request.Body = {10, 20, 30};
    Request.Headers.Add(TEXT("X-MH-Fixture"), TEXT("present"));
    Client->Send(Request, [Observed](TMHResult<FMHHttpResponse> Result)
    {
        Observed->Record(Result.GetValue() && Result.GetValue()->StatusCode == 201 && Result.GetValue()->Body == TArray<uint8>({10, 20, 30}));
    });
    Request = FMHHttpRequest{};
    Request.Url = BaseUrl + TEXT("/slow");
    Request.TimeoutSeconds = 0.2f;
    Client->Send(Request, [Observed](TMHResult<FMHHttpResponse> Result)
    {
        Observed->Record(Result.GetError() && Result.GetError()->Code == EMHErrorCode::Timeout);
    });
    Request.TimeoutSeconds = 5.0f;
    const auto Cancelled = Client->Send(Request, [Observed](TMHResult<FMHHttpResponse> Result)
    {
        Observed->Record(Result.GetError() && Result.GetError()->Code == EMHErrorCode::Cancelled);
    });
    Cancelled.Cancel();
    Request.Url = BaseUrl + TEXT("/disconnect");
    Client->Send(Request, [Observed](TMHResult<FMHHttpResponse> Result)
    {
        Observed->Record(Result.GetError() && Result.GetError()->Code == EMHErrorCode::Unavailable);
    });
    ADD_LATENT_AUTOMATION_COMMAND(FWaitForHttp([this, Client, Observed]()
    {
        if (Observed->Calls < 6) { return false; }
        TestEqual(TEXT("All loopback operations complete once"), Observed->Calls, 6);
        TestTrue(TEXT("Loopback responses, timeout, cancellation and disconnection mapped"), Observed->bCorrect);
        Client->Shutdown();
        return true;
    }, this));
    return true;
}
#endif
