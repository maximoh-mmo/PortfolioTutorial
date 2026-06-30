#include "Multiplayer/OnsetLobbyGameMode.h"
#include "Engine/World.h"

AOnsetLobbyGameMode::AOnsetLobbyGameMode()
{
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
}

void AOnsetLobbyGameMode::TravelToGame()
{
	FString URL = TEXT("/Game/DemoLevel.DemoLevel?game=/Script/Onset.OnsetGameModeBase");
	GetWorld()->ServerTravel(URL);
}
