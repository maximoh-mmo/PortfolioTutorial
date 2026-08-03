#include "UI/CharacterCreationScreen.h"
#include "Player/OnsetPlayerController.h"
#include "Subsystem/OnsetUISubsystem.h"

void UCharacterCreationScreen::SetPlayerController(AOnsetPlayerController* PlayerController)
{
	CachedPlayerController = PlayerController;
}

void UCharacterCreationScreen::SetSlotIndex(int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;
}

void UCharacterCreationScreen::CreateCharacter(const FString& CharacterName, EOnsetCharacterClass CharacterClass, int32 AppearancePresetIndex)
{
	if (!CachedPlayerController || SlotIndex < 0) return;
	if (UOnsetUISubsystem* UI = GetGameInstance()->GetSubsystem<UOnsetUISubsystem>())
	{
		UI->ShowLoadingScreen();
	}
	CachedPlayerController->Server_CreateCharacter(SlotIndex, CharacterName, CharacterClass, AppearancePresetIndex);
}
