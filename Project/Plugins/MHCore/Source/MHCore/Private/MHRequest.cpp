#include "MHRequest.h"
#include "Containers/Ticker.h"

void MH::Private::DispatchCompletion(TUniqueFunction<void()>&& Callback)
{
    FTSTicker::GetCoreTicker().AddTicker(TEXT("MHCompletion"), 0.0f,
        [Callback = MoveTemp(Callback)](float) mutable
        {
            check(IsInGameThread());
            Callback();
            return false;
        });
}
