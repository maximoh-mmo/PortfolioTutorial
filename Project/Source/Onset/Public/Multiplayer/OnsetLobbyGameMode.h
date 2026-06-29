#pragma once

#include "CoreMinimal.h"
#include "Multiplayer/OnsetGameModeBase.h"
#include "OnsetLobbyGameMode.generated.h"

/** GameMode for the character-select lobby. No pawn, no HUD — just the selection widget. */
UCLASS()
class ONSET_API AOnsetLobbyGameMode : public AOnsetGameModeBase
{
	GENERATED_BODY()

public:
	AOnsetLobbyGameMode();

	UFUNCTION(Exec, Category = "Game")
	void TravelToGame();
};
