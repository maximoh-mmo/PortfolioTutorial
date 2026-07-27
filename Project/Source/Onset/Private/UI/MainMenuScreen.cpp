#include "UI/MainMenuScreen.h"

#include "Engine/GameInstance.h"
#include "Player/OnsetPlayerController.h"
#include "Subsystem/OnsetUISubsystem.h"

void UMainMenuScreen::ConnectToServer() const
{
	auto* GameInstance = GetGameInstance();
	auto* PlayerController = GetOwningPlayer<AOnsetPlayerController>();
	if (!GameInstance || !PlayerController) return;
	
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
