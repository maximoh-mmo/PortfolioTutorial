#include "MHHttpClientState.h"

#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

namespace MH::Private
{
    class FUnrealHttpOperation final : public IHttpOperation,
        public TSharedFromThis<FUnrealHttpOperation, ESPMode::ThreadSafe>
    {
        FHttpRequestRef Request;
        TUniqueFunction<void(TMHResult<FMHHttpResponse>)> Completion;

    public:
        explicit FUnrealHttpOperation(const FMHHttpRequest& Input) : Request(FHttpModule::Get().CreateRequest())
        {
            Request->SetURL(Input.Url);
            Request->SetVerb(Input.Verb);
            Request->SetContent(Input.Body);
            Request->SetTimeout(Input.TimeoutSeconds);
            Request->SetDelegateThreadPolicy(EHttpRequestDelegateThreadPolicy::CompleteOnGameThread);
            for (const auto& Header : Input.Headers) { Request->SetHeader(Header.Key, Header.Value); }
            // Curl requires a content type for non-form binary payloads, including empty POSTs.
            if (Request->GetHeader(TEXT("Content-Type")).IsEmpty())
            {
                Request->SetHeader(TEXT("Content-Type"), TEXT("application/octet-stream"));
            }
        }

        bool Start(TUniqueFunction<void(TMHResult<FMHHttpResponse>)>&& InCompletion) override
        {
            Completion = MoveTemp(InCompletion);
            const TWeakPtr<FUnrealHttpOperation, ESPMode::ThreadSafe> WeakSelf = AsShared();
            Request->OnProcessRequestComplete().BindLambda([WeakSelf](FHttpRequestPtr HttpRequest, FHttpResponsePtr Response, bool bSucceeded)
            {
                const auto Self = WeakSelf.Pin();
                if (!Self || !Self->Completion) { return; }
                auto Callback = MoveTemp(Self->Completion);
                if (bSucceeded && Response && Response->GetResponseCode() >= 100)
                {
                    FMHHttpResponse Output;
                    Output.StatusCode = Response->GetResponseCode();
                    Output.Headers = Response->GetAllHeaders();
                    Output.Body = Response->GetContent();
                    Callback(TMHResult<FMHHttpResponse>::Success(MoveTemp(Output)));
                    return;
                }
                const auto Reason = HttpRequest->GetFailureReason();
                const auto Code = Reason == EHttpFailureReason::TimedOut ? EMHErrorCode::Timeout :
                    (Reason == EHttpFailureReason::Cancelled ? EMHErrorCode::Cancelled : EMHErrorCode::Unavailable);
                Callback(TMHResult<FMHHttpResponse>::Failure({Code, TEXT("HTTP transport failed")}));
            });
            if (!Request->ProcessRequest())
            {
                Request->OnProcessRequestComplete().Unbind();
                Completion = nullptr;
                return false;
            }
            return true;
        }

        void Cancel() override
        {
            Request->OnProcessRequestComplete().Unbind();
            Completion = nullptr;
            Request->CancelRequest();
        }
    };

    TSharedRef<IHttpOperation, ESPMode::ThreadSafe> MakeUnrealHttpOperation(const FMHHttpRequest& Request)
    {
        return MakeShared<FUnrealHttpOperation, ESPMode::ThreadSafe>(Request);
    }
}
