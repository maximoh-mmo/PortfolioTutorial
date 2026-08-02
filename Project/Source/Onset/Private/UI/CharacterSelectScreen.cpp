// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CharacterSelectScreen.h"
#include "UI/CharacterCreationScreen.h"
#include "Player/OnsetPlayerController.h"
#include "Subsystem/OnsetUISubsystem.h"

void UCharacterSelectScreen::SetAccountData(const FOnsetAccountData& InAccountData)
{
	CachedAccountData = InAccountData;
	BP_OnAccountDataReady(CachedAccountData);
}

void UCharacterSelectScreen::SetPlayerController(AOnsetPlayerController* PlayerController)
{
	CachedPlayerController = PlayerController;
}

void UCharacterSelectScreen::SelectSlot(int32 SlotIndex)
{
	if (!CachedPlayerController) return;
	if (SlotIndex < 0 || SlotIndex >= CachedAccountData.Slots.Num()) return;
	
	if (CachedAccountData.Slots[SlotIndex].bOccupied)
	{
		SelectedSlot = SlotIndex;
		CachedPlayerController->Server_SelectCharacter(SelectedSlot);
	}
	else
	{
		OpenCreateCharacter(SlotIndex);
	}
}

void UCharacterSelectScreen::EnterWorld() const
{
	if (SelectedSlot < 0 || !CachedPlayerController) return;
	CachedPlayerController->Server_SelectCharacter(SelectedSlot);	
}

void UCharacterSelectScreen::DeleteCharacter(int32 SlotIndex)
{
	if (!CachedPlayerController) return;
	CachedPlayerController->Server_DeleteCharacter(SlotIndex);
}

void UCharacterSelectScreen::OpenCreateCharacter(int32 SlotIndex)
{
	if (!CachedPlayerController) return;

	UOnsetUISubsystem* UI = GetGameInstance()->GetSubsystem<UOnsetUISubsystem>();
	if (!UI || !CharacterCreationScreenClass) return;

	UCharacterCreationScreen* CreationScreen = Cast<UCharacterCreationScreen>(
		UI->PushScreen(EOnsetUILayer::Menu, CharacterCreationScreenClass));
	if (CreationScreen)
	{
		CreationScreen->SetPlayerController(CachedPlayerController);
		CreationScreen->SetSlotIndex(SlotIndex);
	}
}

void UCharacterSelectScreen::RefreshAccountData()
{
	if (!CachedPlayerController) return;
	SetAccountData(CachedPlayerController->GetCachedAccountData());
}

void UCharacterSelectScreen::CreateCharacter(int32 SlotIndex, const FString& CharacterName, EOnsetCharacterClass CharacterClass, int32 AppearancePresetIndex)
{
	if (!CachedPlayerController) return;
	CachedPlayerController->Server_CreateCharacter(SlotIndex, CharacterName, CharacterClass, AppearancePresetIndex);
}
