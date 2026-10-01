#pragma once

#include "MHHttpClient.h"

namespace MH::Private
{
    /** Internal injection seam. Start/completion/cancel are game-thread operations. */
    class IHttpOperation
    {
    public:
        virtual ~IHttpOperation() = default;
        virtual bool Start(TUniqueFunction<void(TMHResult<FMHHttpResponse>)>&& Completion) = 0;
        virtual void Cancel() = 0;
    };

    using FHttpOperationFactory = TFunction<TSharedRef<IHttpOperation, ESPMode::ThreadSafe>(const FMHHttpRequest&)>;
    TSharedRef<IHttpOperation, ESPMode::ThreadSafe> MakeUnrealHttpOperation(const FMHHttpRequest& Request);

    class FHttpClientState : public TSharedFromThis<FHttpClientState, ESPMode::ThreadSafe>
    {
        struct FActive
        {
            FMHRequestHandle Handle;
            TSharedRef<IHttpOperation, ESPMode::ThreadSafe> Operation;
        };
        FHttpOperationFactory Factory;
        TMap<FGuid, FActive> Active;
        bool bShutdown = false;
        void CancelOperation(FGuid Id);

    public:
        explicit FHttpClientState(FHttpOperationFactory InFactory) : Factory(MoveTemp(InFactory)) {}
        ~FHttpClientState();
        FMHRequestHandle Send(const FMHHttpRequest& Request,
            TUniqueFunction<void(TMHResult<FMHHttpResponse>)>&& Completion);
        void Shutdown();
    };
}
