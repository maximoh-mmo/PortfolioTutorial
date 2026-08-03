// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/CharacterSlot.h"

#include "CommonButtonBase.h"
#include "Components/TextBlock.h"

void UCharacterSlot::SetSlotIndex(int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;
	Refresh();
}

void UCharacterSlot::SetSlotData(const FOnsetCharacterSlotData& InSlotData)
{
	SlotData = InSlotData;
	Refresh();
}

FText UCharacterSlot::GetDisplayName() const
{
	return IsOccupied() ? FText::FromString(SlotData.CharacterName) : EmptySlotLabel;
}

FText UCharacterSlot::GetDisplayLevel() const
{
	if (!IsOccupied())
	{
		return FText::GetEmpty();
	}

	return FText::Format(NSLOCTEXT("CharacterSlot", "LevelFormat", "Level {0}"), FText::AsNumber(SlotData.Level));
}

void UCharacterSlot::HandleSlotClicked()
{
	OnSlotActivated.Broadcast(SlotIndex);
}

void UCharacterSlot::HandleDeleteClicked()
{
	OnDeleteRequested.Broadcast(SlotIndex);
}

void UCharacterSlot::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (InteractionButton)
	{
		InteractionButton->OnClicked().AddUObject(this, &UCharacterSlot::HandleSlotClicked);
	}

	if (DeleteButton)
	{
		DeleteButton->OnClicked().AddUObject(this, &UCharacterSlot::HandleDeleteClicked);
	}

	Refresh();
}

void UCharacterSlot::Refresh()
{
	const bool bOccupied = IsOccupied();

	if (NameLabel)
	{
		NameLabel->SetText(GetDisplayName());
	}

	if (LevelLabel)
	{
		LevelLabel->SetText(GetDisplayLevel());
		LevelLabel->SetVisibility(bOccupied ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}

	if (DeleteButton)
	{
		DeleteButton->SetVisibility(bOccupied ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}

	BP_OnSlotDataChanged();
}
