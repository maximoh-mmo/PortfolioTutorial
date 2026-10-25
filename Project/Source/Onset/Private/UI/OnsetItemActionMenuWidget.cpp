// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/OnsetItemActionMenuWidget.h"

#include "CommonTextBlock.h"
#include "Data/OnsetItemLibrary.h"
#include "Inventory/UOnsetInventoryComponent.h"
#include "UI/OnsetButtonBase.h"

void UOnsetItemActionMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (EquipButton) EquipButton->OnClicked().AddUObject(this, &UOnsetItemActionMenuWidget::HandleEquipClicked);
	if (UnequipButton) UnequipButton->OnClicked().AddUObject(this, &UOnsetItemActionMenuWidget::HandleUnequipClicked);
	if (SalvageButton) SalvageButton->OnClicked().AddUObject(this, &UOnsetItemActionMenuWidget::HandleSalvageClicked);
	if (InfuseButton) InfuseButton->OnClicked().AddUObject(this, &UOnsetItemActionMenuWidget::HandleInfuseClicked);
	if (RepairButton) RepairButton->OnClicked().AddUObject(this, &UOnsetItemActionMenuWidget::HandleRepairClicked);
	if (CloseButton) CloseButton->OnClicked().AddUObject(this, &UOnsetItemActionMenuWidget::HandleCloseClicked);
}

void UOnsetItemActionMenuWidget::NativeDestruct()
{
	if (EquipButton) EquipButton->OnClicked().RemoveAll(this);
	if (UnequipButton) UnequipButton->OnClicked().RemoveAll(this);
	if (SalvageButton) SalvageButton->OnClicked().RemoveAll(this);
	if (InfuseButton) InfuseButton->OnClicked().RemoveAll(this);
	if (RepairButton) RepairButton->OnClicked().RemoveAll(this);
	if (CloseButton) CloseButton->OnClicked().RemoveAll(this);

	Super::NativeDestruct();
}

void UOnsetItemActionMenuWidget::SetupForBagItem(UOnsetInventoryComponent* InInventory, int32 InBagSlotIndex)
{
	Inventory = InInventory;
	BagSlotIndex = InBagSlotIndex;
	bIsEquipped = false;

	if (EquipButton) EquipButton->SetVisibility(ESlateVisibility::Visible);
	if (UnequipButton) UnequipButton->SetVisibility(ESlateVisibility::Collapsed);
	if (SalvageButton) SalvageButton->SetVisibility(ESlateVisibility::Visible);
	if (InfuseButton) InfuseButton->SetVisibility(ESlateVisibility::Visible);
	if (RepairButton) RepairButton->SetVisibility(ESlateVisibility::Visible);

	if (ActionPromptText && Inventory.IsValid())
	{
		FOnsetInventoryEntry Entry;
		if (Inventory->GetItemAtSlot(BagSlotIndex, Entry))
		{
			const FOnsetItemDefinition* Def = UOnsetItemLibrary::GetItemDefinition(Entry.Category, Entry.RowName);
			ActionPromptText->SetText(Def && !Def->DisplayName.IsEmpty() ? Def->DisplayName : FText::FromName(Entry.RowName));
		}
	}

	SetVisibility(ESlateVisibility::Visible);
}

void UOnsetItemActionMenuWidget::SetupForEquippedItem(UOnsetInventoryComponent* InInventory, EOnsetEquipmentSlot InEquipSlot)
{
	Inventory = InInventory;
	EquipSlot = InEquipSlot;
	bIsEquipped = true;
	BagSlotIndex = INDEX_NONE;

	if (EquipButton) EquipButton->SetVisibility(ESlateVisibility::Collapsed);
	if (UnequipButton) UnequipButton->SetVisibility(ESlateVisibility::Visible);
	if (SalvageButton) SalvageButton->SetVisibility(ESlateVisibility::Collapsed);
	if (InfuseButton) InfuseButton->SetVisibility(ESlateVisibility::Collapsed);
	if (RepairButton) RepairButton->SetVisibility(ESlateVisibility::Visible);

	if (ActionPromptText && Inventory.IsValid())
	{
		const FName RowName = Inventory->GetEquippedRow(EquipSlot);
		const FOnsetItemDefinition* Def = UOnsetItemLibrary::GetItemDefinition(EOnsetItemCategory::Equipment, RowName);
		ActionPromptText->SetText(Def && !Def->DisplayName.IsEmpty() ? Def->DisplayName : FText::FromName(RowName));
	}

	SetVisibility(ESlateVisibility::Visible);
}

void UOnsetItemActionMenuWidget::CloseActionMenu()
{
	SetVisibility(ESlateVisibility::Collapsed);
	BagSlotIndex = INDEX_NONE;
}

void UOnsetItemActionMenuWidget::HandleEquipClicked()
{
	if (Inventory.IsValid() && BagSlotIndex != INDEX_NONE)
	{
		Inventory->EquipFromInventorySlot(BagSlotIndex);
		OnActionExecuted.Broadcast();
	}
	CloseActionMenu();
}

void UOnsetItemActionMenuWidget::HandleUnequipClicked()
{
	if (Inventory.IsValid() && bIsEquipped)
	{
		Inventory->UnequipSlot(EquipSlot);
		OnActionExecuted.Broadcast();
	}
	CloseActionMenu();
}

void UOnsetItemActionMenuWidget::HandleSalvageClicked()
{
	if (Inventory.IsValid() && BagSlotIndex != INDEX_NONE)
	{
		TArray<FOnsetInventoryEntry> Yield;
		Inventory->SalvageItem(BagSlotIndex, Yield);
		OnActionExecuted.Broadcast();
	}
	CloseActionMenu();
}

void UOnsetItemActionMenuWidget::HandleInfuseClicked()
{
	if (Inventory.IsValid() && BagSlotIndex != INDEX_NONE)
	{
		FOnsetInventoryEntry TargetEntry;
		if (Inventory->GetItemAtSlot(BagSlotIndex, TargetEntry))
		{
			// Find duplicate catalyst in bag
			const TArray<FOnsetInventoryEntry>& Items = Inventory->GetItems();
			int32 CatalystIndex = INDEX_NONE;
			for (int32 Index = 0; Index < Items.Num(); ++Index)
			{
				if (Index != BagSlotIndex && Items[Index].Category == EOnsetItemCategory::Equipment && Items[Index].RowName == TargetEntry.RowName)
				{
					CatalystIndex = Index;
					break;
				}
			}

			if (CatalystIndex != INDEX_NONE)
			{
				bool bGradeUp = false;
				bool bCrit = false;
				Inventory->InfuseItem(BagSlotIndex, CatalystIndex, bGradeUp, bCrit);
				OnActionExecuted.Broadcast();
			}
		}
	}
	CloseActionMenu();
}

void UOnsetItemActionMenuWidget::HandleRepairClicked()
{
	if (Inventory.IsValid())
	{
		if (bIsEquipped)
		{
			Inventory->RepairEquippedItem(EquipSlot);
		}
		else if (BagSlotIndex != INDEX_NONE)
		{
			Inventory->RepairBagItem(BagSlotIndex);
		}
		OnActionExecuted.Broadcast();
	}
	CloseActionMenu();
}

void UOnsetItemActionMenuWidget::HandleCloseClicked()
{
	CloseActionMenu();
}
