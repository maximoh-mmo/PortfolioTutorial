#pragma once

#include "CoreMinimal.h"
#include "Game/OnsetGameModeBase.h"
#include "OnsetLoginServerGameMode.generated.h"

UCLASS()
class ONSET_API AOnsetLoginServerGameMode : public AOnsetGameModeBase
{
	GENERATED_BODY()

public:
	AOnsetLoginServerGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;

private:
	void KickPlayer(APlayerController* Player);

	UPROPERTY()
	float KickDelay = 2.0f;
};
