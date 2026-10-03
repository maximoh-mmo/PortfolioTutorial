#pragma once

#include "MHRequest.h"

/** Provider-defined, case-sensitive identifier. Never a backend session or proof of ownership. */
struct FMHAccountId
{
    FString Provider;
    FString Subject;

    bool IsValid() const { return !Provider.IsEmpty() && !Subject.IsEmpty(); }
    bool operator==(const FMHAccountId& Other) const
    {
        return Provider.Equals(Other.Provider, ESearchCase::CaseSensitive)
            && Subject.Equals(Other.Subject, ESearchCase::CaseSensitive);
    }
};

/** Shared credential lease. All access and final reference destruction must be on the game thread.
 * Release revokes the native ticket once and clears the retained bytes. Copies made by callers
 * cannot be revoked locally. Never log or persist credentials. Provider shutdown releases leases.
 */
class MHONLINECORE_API FMHAuthTicket final
{
public:
    FMHAuthTicket(FMHAccountId InAccount, FString InAudience, TArray<uint8> InBytes,
        TUniqueFunction<void()>&& InRelease);
    ~FMHAuthTicket();
    FMHAuthTicket(const FMHAuthTicket&) = delete;
    FMHAuthTicket& operator=(const FMHAuthTicket&) = delete;

    const FMHAccountId& GetAccount() const { return Account; }
    const FString& GetAudience() const { return Audience; }
    const TArray<uint8>& GetBytes() const { return Bytes; }
    bool IsReleased() const { return bReleased; }
    void Release();

private:
    FMHAccountId Account;
    FString Audience;
    TArray<uint8> Bytes;
    TUniqueFunction<void()> ReleaseNative;
    bool bReleased = false;
};

using FMHAuthTicketRef = TSharedRef<FMHAuthTicket>;

struct FMHAuthTicketRequest
{
    int32 LocalUserIndex = 0;
    /** Provider-specific verifier identity; providers must reject unsupported audiences. */
    FString Audience;
    float TimeoutSeconds = 30.0f;
};

/** Game-thread API. Callbacks are deferred to the game thread, exactly once, through MHCore.
 * Handles may be cancelled from any thread. Implementations own pending native operations;
 * cancellation/timeout must dispose of late tickets. No interactive login is implied.
 * Shutdown is idempotent, cancels pending requests, releases tickets and rejects new work.
 * Providers must shut down before the native SDK and while the core ticker is still running.
 */
class MHONLINECORE_API IMHIdentityProvider
{
public:
    virtual ~IMHIdentityProvider() = default;
    virtual FMHRequestHandle GetLocalAccount(int32 LocalUserIndex,
        TUniqueFunction<void(TMHResult<FMHAccountId>)>&& Completion) = 0;
    virtual FMHRequestHandle AcquireAuthTicket(const FMHAuthTicketRequest& Request,
        TUniqueFunction<void(TMHResult<FMHAuthTicketRef>)>&& Completion) = 0;
    virtual void Shutdown() = 0;
};
