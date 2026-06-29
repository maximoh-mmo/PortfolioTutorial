#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "OnsetMenuGameMode.generated.h"

UCLASS()
class ONSET_API AOnsetMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AOnsetMenuGameMode();

	virtual void BeginPlay() override;
};
