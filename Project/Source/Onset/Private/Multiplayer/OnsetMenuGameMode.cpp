#include "Multiplayer/OnsetMenuGameMode.h"
#include "UI/OnsetMenuHUD.h"

AOnsetMenuGameMode::AOnsetMenuGameMode()
{
	DefaultPawnClass = nullptr;
	HUDClass = AOnsetMenuHUD::StaticClass();
}
