#include "UI/CharacterCreationScreen.h"
#include "Player/OnsetPlayerController.h"

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
	CachedPlayerController->Server_CreateCharacter(SlotIndex, CharacterName, CharacterClass, AppearancePresetIndex);
}
