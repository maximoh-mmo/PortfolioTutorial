// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CharacterSelectScreen.h"
#include "Player/OnsetPlayerController.h"

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
		FString DefaultName = FString::Printf(TEXT("Hero_%d"), SlotIndex+1);
		CachedPlayerController->Server_CreateCharacter(SlotIndex, DefaultName);
	}
}

void UCharacterSelectScreen::EnterWorld() const
{
	if (SelectedSlot < 0 || !CachedPlayerController) return;
	CachedPlayerController->Server_SelectCharacter(SelectedSlot);	
}
