#include "Data/OnsetSessionTokens.h"

#if WITH_SERVER_CODE || WITH_EDITOR
#include "String/LexFromString.h"

namespace Onset::SessionTokens
{
    TMHResult<FString> Issue(const FMHSessionTokenConfig& Config, const FString& Platform,
        const FString& PlatformID, int32 SlotIndex, int64 NowUnix)
    {
        if (SlotIndex < -1 || SlotIndex > 2)
            return TMHResult<FString>::Failure({EMHErrorCode::InvalidArgument, TEXT("Invalid character slot")});
        return FMHSessionTokenService(Config).Issue(PlatformID, Platform, FString::FromInt(SlotIndex), NowUnix);
    }

    TMHResult<FClaims> Validate(const FMHSessionTokenConfig& Config, const FString& Token, int64 NowUnix)
    {
        const auto Result = FMHSessionTokenService(Config).Validate(Token, NowUnix);
        if (const auto* Error = Result.GetError()) return TMHResult<FClaims>::Failure(*Error);
        const auto& Claims = *Result.GetValue();
        int32 SlotIndex = -1;
        if (!LexTryParseString(SlotIndex, *Claims.Context) || SlotIndex < -1 || SlotIndex > 2 ||
            FString::FromInt(SlotIndex) != Claims.Context)
            return TMHResult<FClaims>::Failure({EMHErrorCode::Unauthorized, TEXT("Invalid character slot claim")});
        return TMHResult<FClaims>::Success({Claims.Provider, Claims.Subject, SlotIndex});
    }
}
#endif
