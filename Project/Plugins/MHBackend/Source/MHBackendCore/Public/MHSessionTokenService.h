#pragma once

#include "MHResult.h"

struct FMHSessionTokenConfig
{
    FString Secret;
    int32 LifetimeSeconds = 300;
};

/** Context is an opaque game-defined claim. MH does not interpret character slots. */
struct FMHSessionClaims
{
    FString Subject;
    FString Provider;
    FString Context;
    int64 ExpiryUnix = 0;
};

/** Local cryptographic operations; no network I/O. Keep this service on trusted servers. */
class MHBACKENDCORE_API FMHSessionTokenService
{
    FMHSessionTokenConfig Config;

public:
    explicit FMHSessionTokenService(FMHSessionTokenConfig InConfig);
    /** Reads [MH.Auth] Secret and LifetimeSeconds; no fallback development secret. */
    static TMHResult<FMHSessionTokenConfig> LoadConfig(const FString& IniFilename);
    TMHResult<FString> Issue(const FString& Subject, const FString& Provider,
        const FString& Context, int64 NowUnix) const;
    TMHResult<FMHSessionClaims> Validate(const FString& Token, int64 NowUnix) const;
};
