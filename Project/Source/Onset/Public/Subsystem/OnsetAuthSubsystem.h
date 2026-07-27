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

UCLASS()
class ONSET_API UOnsetAuthSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	EOnsetAuthMode GetAuthMode() const { return AuthMode; }

	void HandlePostLogin(APlayerController* NewPlayer);
	void HandleLogout(AController* Exiting);

	void ValidateAuthTicket(APlayerController* NewPlayer, const FString& AuthTicket);

private:
	EOnsetAuthMode AuthMode = EOnsetAuthMode::Direct;
};
