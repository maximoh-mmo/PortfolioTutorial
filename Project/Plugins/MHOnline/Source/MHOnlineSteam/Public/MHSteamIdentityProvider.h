#pragma once
#include "IMHIdentityProvider.h"

/** Client adapter. Requires an initialized Steam online subsystem and its callback pump.
 * Audience: webapi:<ASCII verifier identity> or session:<decimal target Steam ID>.
 * Native implementation currently supports Win64 clients; other targets fail NotConfigured.
 */
class MHONLINESTEAM_API FMHSteamIdentityProvider final : public IMHIdentityProvider
{
public:
    FMHSteamIdentityProvider();
    ~FMHSteamIdentityProvider() override;
    FMHRequestHandle GetLocalAccount(int32 LocalUserIndex,
        TUniqueFunction<void(TMHResult<FMHAccountId>)>&& Completion) override;
    FMHRequestHandle AcquireAuthTicket(const FMHAuthTicketRequest& Request,
        TUniqueFunction<void(TMHResult<FMHAuthTicketRef>)>&& Completion) override;
    void Shutdown() override;
private:
    friend class FMHSteamLifecycleTest;
    explicit FMHSteamIdentityProvider(TSharedRef<class IMHSteamClient> Client);
    struct FState;
    TSharedPtr<FState> State;
};
