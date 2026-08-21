#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OnsetAuthSubsystem.generated.h"

class AOnsetPlayerController;
class AOnsetPlayerState;

DECLARE_LOG_CATEGORY_EXTERN(LogOnsetAuth, Log, All);

/**
 * How clients authenticate with a server.
 * - Direct: the client sends a Steam auth ticket over the wire; the server validates
 *   it against Steam and extracts the platform ID inline. Default for PIE/LAN.
 * - Token: the client presents an HMAC-signed session token in the travel URL
 *   (?Token=...) issued by the Login Server; no Steam call is needed on the Game Server.
 */
UENUM()
enum class EOnsetAuthMode : uint8
{
	Direct UMETA(DisplayName="Direct"),
	Token  UMETA(DisplayName="Token")
};

/**
 * Payload carried by a session token (base64 of "Platform|PlatformID|Expiry|Signature").
 * Signature is HMAC-SHA256(AuthTokenSecret, "Platform|PlatformID|Expiry") so the server
 * can verify authenticity without storing token state.
 */
struct FOnsetSessionToken
{
	/** Account identity this token authorizes (e.g. SteamID). */
	FString PlatformID;
	/** Identity source ("Steam", or a dev prefix in Direct/no-Steam mode). */
	FString Platform;
	/** Unix timestamp after which the token must be rejected. */
	int64 ExpiryUnix = 0;
	/** Hex-encoded HMAC-SHA256 over "Platform|PlatformID|Expiry". */
	FString Signature;

	bool IsValid() const { return !PlatformID.IsEmpty() && !Platform.IsEmpty() && ExpiryUnix > 0; }
};

/**
 * Server-only world subsystem owning all authentication (see [Account_System.md]).
 * Extracted from AOnsetGameModeBase in A5c so PostLogin/Logout stay thin:
 * the GameMode delegates to HandlePostLogin/HandleLogout here.
 *
 * Config ([Onset.Auth] in DefaultEngine.ini):
 * - AuthMode        Direct | Token (default Direct)
 * - AuthTokenSecret shared HMAC secret (Token mode; inject per-server, never ship to clients)
 * - AuthTokenLifetimeSeconds token validity window (default 300)
 */
UCLASS()
class ONSET_API UOnsetAuthSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	/** The auth mode resolved from config at Initialize (Direct when unset). */
	EOnsetAuthMode GetAuthMode() const { return AuthMode; }

	/** Token mode gate: validates ?Token= from the login options. Returns empty on failure
	 *  (caller rejects the connection); on success fills OutPlatform/OutPlatformID and
	 *  caches identity for HandlePostLogin keyed by address. */
	FString PreLoginTokenAuth(const FString& Options, const FString& Address, FString& OutPlatform, FString& OutPlatformID);
	/** Direct mode gate: resolves platform identity from the net connection (no Steam call here). */
	void PreLoginDirect(const FString& Options, const FString& Address);
	/** Runs Steam ticket validation (Direct) or consumes cached token identity (Token),
	 *  then loads/creates the account. Called from GameMode PostLogin. */
	void HandlePostLogin(APlayerController* NewPlayer);
	/** Persists + cleans up the leaving player (threat tables, engagement). */
	void HandleLogout(AController* Exiting);

	/** Direct mode: validates a client-sent Steam ticket via BeginAuthSession; rejects the
	 *  connection on failure and starts the auth-timeout handshake on success. */
	void ValidateAuthTicket(APlayerController* NewPlayer, const FString& AuthTicket);

	/** Builds + signs a session token for the given identity (Token mode issue path). */
	FString GenerateToken(const FString& Platform, const FString& PlatformID, int32 SlotIndex = -1);
	/** Verifies signature + expiry; fills the out params from the payload on success. */
	bool ValidateToken(const FString& TokenStr, FString& OutPlatform, FString& OutPlatformID, int32& OutSlotIndex);

	/** Identity resolved at PreLogin, consumed by HandlePostLogin once the PC exists. */
	struct FPendingTokenAuth
	{
		FString Platform;
		FString PlatformID;
		int32 SlotIndex = -1;
	};

private:
	/** Pure-software HMAC-SHA256 (no platform dependency; see TODO/DONE A5c notes). */
	static TArray<uint8> HmacSha256(const TArray<uint8>& Key, const TArray<uint8>& Data);

	EOnsetAuthMode AuthMode = EOnsetAuthMode::Direct;
	FString AuthTokenSecret;
	int32 AuthTokenLifetimeSeconds = 300;

	/** Keyed by connection address (port stripped); drained in HandlePostLogin. */
	TMap<FString, FPendingTokenAuth> PendingTokenAuthMap;

	/** Dev-only: assigns stable test identities to local multi-client sessions. */
	TMap<FString, int32> PendingDevClientIndexMap;
};
