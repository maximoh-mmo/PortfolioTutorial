#include "Multiplayer/OnsetLobbyGameMode.h"
#include "Engine/World.h"

AOnsetLobbyGameMode::AOnsetLobbyGameMode()
{
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
}

void AOnsetLobbyGameMode::TravelToGame()
{
	FString URL = TEXT("DemoLevel?game=/Game/OnsetGameMode.OnsetGameMode_C");
	GetWorld()->ServerTravel(URL);
}
