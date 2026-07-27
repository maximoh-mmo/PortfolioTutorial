#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OnsetAuthSubsystem.generated.h"

class AOnsetPlayerController;
class AOnsetPlayerState;

DECLARE_LOG_CATEGORY_EXTERN(LogOnsetAuth, Log, All);

UENUM()
enum class EOnsetAuthMode : uint8
{
	Direct UMETA(DisplayName="Direct"),
	Token  UMETA(DisplayName="Token")
};

struct FOnsetSessionToken
{
	FString PlatformID;
	FString Platform;
	int64 ExpiryUnix = 0;
	FString Signature;

	bool IsValid() const { return !PlatformID.IsEmpty() && !Platform.IsEmpty() && ExpiryUnix > 0; }
};

UCLASS()
class ONSET_API UOnsetAuthSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	EOnsetAuthMode GetAuthMode() const { return AuthMode; }

	FString PreLoginTokenAuth(const FString& Options, const FString& Address, FString& OutPlatform, FString& OutPlatformID);
	void HandlePostLogin(APlayerController* NewPlayer);
	void HandleLogout(AController* Exiting);

	void ValidateAuthTicket(APlayerController* NewPlayer, const FString& AuthTicket);

	FString GenerateToken(const FString& Platform, const FString& PlatformID, int32 SlotIndex = -1);
	bool ValidateToken(const FString& TokenStr, FString& OutPlatform, FString& OutPlatformID, int32& OutSlotIndex);

	struct FPendingTokenAuth
	{
		FString Platform;
		FString PlatformID;
		int32 SlotIndex = -1;
	};

private:
	static TArray<uint8> HmacSha256(const TArray<uint8>& Key, const TArray<uint8>& Data);

	EOnsetAuthMode AuthMode = EOnsetAuthMode::Direct;
	FString AuthTokenSecret;
	int32 AuthTokenLifetimeSeconds = 300;

	TMap<FString, FPendingTokenAuth> PendingTokenAuthMap;
};
