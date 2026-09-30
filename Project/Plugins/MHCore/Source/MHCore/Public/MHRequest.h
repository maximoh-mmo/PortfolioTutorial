#pragma once

#include "MHResult.h"
#include "HAL/CriticalSection.h"
#include "Misc/ScopeLock.h"
#include "Templates/Function.h"
#include "Templates/SharedPointer.h"

namespace MH::Private
{
    /** Always queues, including when called from the game thread. Requires an active core ticker. */
    MHCORE_API void DispatchCompletion(TUniqueFunction<void()>&& Callback);

    class IRequestState
    {
    public:
        virtual ~IRequestState() = default;
        virtual bool Cancel() = 0;
        virtual bool IsPending() const = 0;
    };

    template <typename T>
    class TRequestState final : public IRequestState
    {
        mutable FCriticalSection Mutex;
        bool bPending = true;
        TUniqueFunction<void(TMHResult<T>)> Completion;

    public:
        explicit TRequestState(TUniqueFunction<void(TMHResult<T>)>&& InCompletion)
            : Completion(MoveTemp(InCompletion)) {}

        bool Complete(TMHResult<T> Result)
        {
            TUniqueFunction<void(TMHResult<T>)> Callback;
            {
                FScopeLock Lock(&Mutex);
                if (!bPending) { return false; }
                bPending = false;
                Callback = MoveTemp(Completion);
            }
            DispatchCompletion([Callback = MoveTemp(Callback), Result = MoveTemp(Result)]() mutable
            {
                if (Callback) { Callback(MoveTemp(Result)); }
            });
            return true;
        }

        bool Cancel() override
        {
            return Complete(TMHResult<T>::Failure({EMHErrorCode::Cancelled, TEXT("Request cancelled")}));
        }

        bool IsPending() const override
        {
            FScopeLock Lock(&Mutex);
            return bPending;
        }
    };
}

/** Copyable observer/cancellation token. Destruction does not cancel; owners cancel explicitly. */
class FMHRequestHandle
{
    TSharedPtr<MH::Private::IRequestState, ESPMode::ThreadSafe> State;
    template <typename T> friend class TMHRequest;
    explicit FMHRequestHandle(TSharedPtr<MH::Private::IRequestState, ESPMode::ThreadSafe> InState)
        : State(MoveTemp(InState)) {}

public:
    FMHRequestHandle() = default;
    bool IsValid() const { return State.IsValid(); }
    /** True only if cancellation wins the terminal-state race. Does not abort remote writes. */
    bool Cancel() const { return State && State->Cancel(); }
    /** False once a terminal result is selected, possibly before callback delivery. */
    bool IsPending() const { return State && State->IsPending(); }
};

/** Provider-side completion source. Keep a copy alive until completion or cancellation. */
template <typename T>
class TMHRequest
{
    TSharedRef<MH::Private::TRequestState<T>, ESPMode::ThreadSafe> State;

public:
    explicit TMHRequest(TUniqueFunction<void(TMHResult<T>)>&& Completion)
        : State(MakeShared<MH::Private::TRequestState<T>, ESPMode::ThreadSafe>(MoveTemp(Completion))) {}

    FMHRequestHandle GetHandle() const { return FMHRequestHandle(State); }
    /** Thread-safe: first completion or cancellation wins, later attempts return false. */
    bool Complete(TMHResult<T> Result) const { return State->Complete(MoveTemp(Result)); }
};
