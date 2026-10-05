#include "UI/MainMenuScreen.h"

#include "CommonButtonBase.h"
#include "Engine/GameInstance.h"
#include "Player/OnsetPlayerController.h"
#include "Subsystem/OnsetUISubsystem.h"

void UMainMenuScreen::NativeOnActivated()
{
	Super::NativeOnActivated();

	AOnsetPlayerController* PlayerController = GetOwningPlayer<AOnsetPlayerController>();
	if (BoundPlayerController.Get() != PlayerController)
	{
		if (BoundPlayerController.IsValid())
		{
			BoundPlayerController->OnAccountDataChanged.RemoveDynamic(this, &UMainMenuScreen::HandleAccountDataChanged);
		}

		BoundPlayerController = PlayerController;
		if (BoundPlayerController.IsValid())
		{
			BoundPlayerController->OnAccountDataChanged.AddDynamic(this, &UMainMenuScreen::HandleAccountDataChanged);
		}
	}

	RefreshConnectAvailability();
}

void UMainMenuScreen::NativeDestruct()
{
	if (BoundPlayerController.IsValid())
	{
		BoundPlayerController->OnAccountDataChanged.RemoveDynamic(this, &UMainMenuScreen::HandleAccountDataChanged);
		BoundPlayerController = nullptr;
	}

	Super::NativeDestruct();
}

bool UMainMenuScreen::CanConnectToServer() const
{
	const AOnsetPlayerController* PlayerController = GetOwningPlayer<AOnsetPlayerController>();
	return PlayerController && PlayerController->IsAccountDataReady();
}

void UMainMenuScreen::RefreshConnectAvailability()
{
	// Keep the Blueprint's existing WBP_ButtonBase variable and graph pin type.
	// A native BindWidget property with the same name shadows that variable.
	if (UCommonButtonBase* Button = Cast<UCommonButtonBase>(GetWidgetFromName(TEXT("ConnectButton"))))
	{
		Button->SetIsEnabled(CanConnectToServer());
	}
}

void UMainMenuScreen::HandleAccountDataChanged()
{
	RefreshConnectAvailability();
}

void UMainMenuScreen::ConnectToServer() const
{
	auto* GameInstance = GetGameInstance();
	auto* PlayerController = GetOwningPlayer<AOnsetPlayerController>();
	if (!GameInstance || !PlayerController) return;
	if (!PlayerController->IsAccountDataReady())
	{
		UE_LOG(LogTemp, Verbose, TEXT("ConnectToServer: waiting for server-side Steam packet authentication."));
		return;
	}
	
	auto* UI = GameInstance->GetSubsystem<UOnsetUISubsystem>();
	if (!UI || !PlayerController || !CharacterSelectScreenClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ConnectToServer: UI=%d PC=%d CharSelectClass=%d"),
			!!UI, !!PlayerController, !!CharacterSelectScreenClass);
		return;
	}                                                      
                                                                                                                     
	if (auto* Screen = Cast<UCharacterSelectScreen>(UI->PushScreen(EOnsetUILayer::Game, CharacterSelectScreenClass)))                         
	{                                         
		Screen->SetPlayerController(PlayerController);
		Screen->SetAccountData(PlayerController->GetCachedAccountData());                                                     
	}                 
}
