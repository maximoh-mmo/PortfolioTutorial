#pragma once
#include "CoreMinimal.h"

struct FMHSteamEvent
{
    uint32 Handle = 0;
    bool bSuccess = false;
    TArray<uint8> Bytes;
};

/** Internal SDK seam. All methods on game thread; SDK callbacks queue events only. */
class IMHSteamClient
{
public:
    virtual ~IMHSteamClient() = default;
    virtual bool IsAvailable() const = 0;
    virtual FString Account() const = 0;
    virtual uint32 Begin(const FString& Audience) = 0;
    virtual void Cancel(uint32 Handle) = 0;
    virtual bool Poll(FMHSteamEvent& Event) = 0;
};
TSharedRef<IMHSteamClient> MHCreateSteamClient();
