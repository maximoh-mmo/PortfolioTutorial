#include "Multiplayer/OnsetLobbyGameMode.h"
#include "UI/OnsetLobbyHUD.h"
#include "Engine/World.h"

AOnsetLobbyGameMode::AOnsetLobbyGameMode()
{
	DefaultPawnClass = nullptr;
	HUDClass = AOnsetLobbyHUD::StaticClass();
}

void AOnsetLobbyGameMode::TravelToGame()
{
	FString URL = TEXT("DemoLevel?game=/Script/Onset.OnsetGameModeBase");
	GetWorld()->ServerTravel(URL);
}
