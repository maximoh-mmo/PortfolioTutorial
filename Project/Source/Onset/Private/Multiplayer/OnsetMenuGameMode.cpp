#include "Multiplayer/OnsetMenuGameMode.h"
#include "UI/MainMenuWidget.h"
#include "Blueprint/UserWidget.h"

AOnsetMenuGameMode::AOnsetMenuGameMode()
{
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
}

void AOnsetMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeUIOnly());

		UMainMenuWidget* Widget = CreateWidget<UMainMenuWidget>(PC);
		if (Widget)
		{
			Widget->AddToViewport();
		}
	}
}
