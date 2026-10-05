#pragma once

#include "CoreMinimal.h"

#if WITH_SERVER_CODE || WITH_EDITOR
#include "MHSessionTokenService.h"

namespace Onset::SessionTokens
{
    struct FClaims
    {
        FString Platform;
        FString PlatformID;
        int32 SlotIndex = -1;
    };

    // -1 is the existing unselected-slot sentinel; playable slots are 0..2.
    TMHResult<FString> Issue(const FMHSessionTokenConfig& Config, const FString& Platform,
        const FString& PlatformID, int32 SlotIndex, int64 NowUnix);
    TMHResult<FClaims> Validate(const FMHSessionTokenConfig& Config, const FString& Token, int64 NowUnix);
}
#endif
