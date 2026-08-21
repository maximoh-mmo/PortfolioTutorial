// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/InventorySlotWidget.h"

#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Data/OnsetItemLibrary.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "UI/OnsetButtonBase.h"

/** Rarity-tinted accent so item rarity reads at a glance (matches loot overlay). */
static FLinearColor GetInventoryRarityColor(EOnsetItemRarity Rarity)
{
	switch (Rarity)
	{
		case EOnsetItemRarity::Uncommon:	return FLinearColor(0.3f, 1.0f, 0.4f, 1.0f);
		case EOnsetItemRarity::Rare:		return FLinearColor(0.35f, 0.7f, 1.0f, 1.0f);
		case EOnsetItemRarity::Epic:		return FLinearColor(0.75f, 0.35f, 1.0f, 1.0f);
		case EOnsetItemRarity::Legendary:	return FLinearColor(1.0f, 0.6f, 0.2f, 1.0f);
		default:							return FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
}

/** Returns the display name for an equipment slot ("Ring 1", "Head", ...). */
static FText GetEquipmentSlotLabel(EOnsetEquipmentSlot Slot)
{
	switch (Slot)
	{
		case EOnsetEquipmentSlot::Weapon:	return NSLOCTEXT("InventorySlot", "Weapon", "Weapon");
		case EOnsetEquipmentSlot::Shield:	return NSLOCTEXT("InventorySlot", "Shield", "Shield");
		case EOnsetEquipmentSlot::Head:		return NSLOCTEXT("InventorySlot", "Head", "Head");
		case EOnsetEquipmentSlot::Chest:	return NSLOCTEXT("InventorySlot", "Chest", "Chest");
		case EOnsetEquipmentSlot::Hands:	return NSLOCTEXT("InventorySlot", "Hands", "Hands");
		case EOnsetEquipmentSlot::Legs:		return NSLOCTEXT("InventorySlot", "Legs", "Legs");
		case EOnsetEquipmentSlot::Feet:		return NSLOCTEXT("InventorySlot", "Feet", "Feet");
		case EOnsetEquipmentSlot::Amulet:	return NSLOCTEXT("InventorySlot", "Amulet", "Amulet");
		case EOnsetEquipmentSlot::Ring1:	return NSLOCTEXT("InventorySlot", "Ring1", "Ring 1");
		case EOnsetEquipmentSlot::Ring2:	return NSLOCTEXT("InventorySlot", "Ring2", "Ring 2");
		case EOnsetEquipmentSlot::Trinket:	return NSLOCTEXT("InventorySlot", "Trinket", "Trinket");
		default:							return NSLOCTEXT("InventorySlot", "Unknown", "Slot");
	}
}

void UInventorySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyVisualState();
}

void UInventorySlotWidget::SetSlotIndex(int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;
}

void UInventorySlotWidget::SetItem(const FOnsetInventoryEntry& InEntry)
{
	bIsEquipmentCell = false;
	Entry = InEntry;

	const FOnsetItemDefinition* Def = UOnsetItemLibrary::GetItemDefinition(Entry.Category, Entry.RowName);

	if (ItemIcon)
	{
		if (Def && !Def->Icon.IsNull())
		{
			if (UTexture2D* Icon = Def->Icon.LoadSynchronous())
			{
				ItemIcon->SetBrushFromTexture(Icon);
			}
			ItemIcon->SetVisibility(ESlateVisibility::Visible);
			ItemIcon->SetColorAndOpacity(GetInventoryRarityColor(Def ? Def->Rarity : EOnsetItemRarity::Common));
		}
		else
		{
			ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (EmptyIcon)
	{
		EmptyIcon->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (CountLabel)
	{
		CountLabel->SetVisibility((Entry.Count > 1) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		CountLabel->SetText(FText::AsNumber(Entry.Count));
	}

	if (SlotLabel)
	{
		SlotLabel->SetVisibility(ESlateVisibility::Collapsed);
	}

	ApplyVisualState();
}

void UInventorySlotWidget::SetEmpty()
{
	bIsEquipmentCell = false;
	Entry = FOnsetInventoryEntry();

	if (ItemIcon)
	{
		ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (EmptyIcon)
	{
		EmptyIcon->SetVisibility(ESlateVisibility::Visible);
	}
	if (CountLabel)
	{
		CountLabel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SlotLabel)
	{
		SlotLabel->SetVisibility(ESlateVisibility::Collapsed);
	}

	ApplyVisualState();
}

void UInventorySlotWidget::SetEquipment(EOnsetEquipmentSlot InSlot, FName InRowName)
{
	bIsEquipmentCell = true;
	EquipmentSlot = InSlot;
	Entry.Category = EOnsetItemCategory::Equipment;
	Entry.RowName = InRowName;
	Entry.Count = 1;

	const FOnsetItemDefinition* Def = UOnsetItemLibrary::GetItemDefinition(Entry.Category, Entry.RowName);

	if (ItemIcon)
	{
		if (Def && !Def->Icon.IsNull())
		{
			if (UTexture2D* Icon = Def->Icon.LoadSynchronous())
			{
				ItemIcon->SetBrushFromTexture(Icon);
			}
			ItemIcon->SetVisibility(ESlateVisibility::Visible);
			ItemIcon->SetColorAndOpacity(GetInventoryRarityColor(Def ? Def->Rarity : EOnsetItemRarity::Common));
		}
		else
		{
			ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (EmptyIcon)
	{
		EmptyIcon->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (CountLabel)
	{
		CountLabel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (SlotLabel)
	{
		SlotLabel->SetVisibility(ESlateVisibility::Visible);
		SlotLabel->SetText(GetEquipmentSlotLabel(InSlot));
	}

	ApplyVisualState();
}

void UInventorySlotWidget::SetEquipmentEmpty(EOnsetEquipmentSlot InSlot)
{
	bIsEquipmentCell = true;
	EquipmentSlot = InSlot;
	Entry = FOnsetInventoryEntry();

	if (ItemIcon)
	{
		ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (EmptyIcon)
	{
		EmptyIcon->SetVisibility(ESlateVisibility::Visible);
	}
	if (CountLabel)
	{
		CountLabel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SlotLabel)
	{
		SlotLabel->SetVisibility(ESlateVisibility::Visible);
		SlotLabel->SetText(GetEquipmentSlotLabel(InSlot));
	}

	ApplyVisualState();
}

void UInventorySlotWidget::ApplyVisualState()
{
	if (SlotButton)
	{
		SlotButton->OnClicked().RemoveAll(this);
		SlotButton->OnClicked().AddUObject(this, &UInventorySlotWidget::HandleClicked);
	}

	BP_OnSlotDataChanged();
}

void UInventorySlotWidget::HandleClicked()
{
	OnSlotClicked.Broadcast(SlotIndex);
}