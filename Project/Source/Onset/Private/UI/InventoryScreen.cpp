// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/InventoryScreen.h"

#include "Blueprint/UserWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Core/OnsetBaseCharacter.h"
#include "Engine/GameInstance.h"
#include "Inventory/UOnsetInventoryComponent.h"
#include "Subsystem/OnsetUISubsystem.h"
#include "UI/InventorySlotWidget.h"

namespace
{
	/** Resolves the bag cell count from the owning pawn's inventory component. */
	int32 GetMaxBagSlots(UOnsetInventoryComponent* Inventory)
	{
		return Inventory ? FMath::Max(1, Inventory->MaxInventorySlots) : 80;
	}
}

void UInventoryScreen::CloseInventory()
{
	if (UOnsetUISubsystem* UI = GetGameInstance()->GetSubsystem<UOnsetUISubsystem>())
	{
		UI->PopScreen(EOnsetUILayer::Game);
	}
}

void UInventoryScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	BindToInventory(GetInventoryComponent());
	RebuildSlots();
}

void UInventoryScreen::NativeOnDeactivated()
{
	BindToInventory(nullptr);
	Super::NativeOnDeactivated();
}

void UInventoryScreen::NativeDestruct()
{
	BindToInventory(nullptr);
	Super::NativeDestruct();
}

UOnsetInventoryComponent* UInventoryScreen::GetInventoryComponent() const
{
	if (const APlayerController* PC = GetOwningPlayer())
	{
		if (const APawn* Pawn = PC->GetPawn())
		{
			if (const AOnsetBaseCharacter* Character = Cast<AOnsetBaseCharacter>(Pawn))
			{
				return Character->InventoryComponent;
			}
		}
	}
	return nullptr;
}

void UInventoryScreen::BindToInventory(UOnsetInventoryComponent* InInventory)
{
	if (BoundInventory == InInventory)
	{
		return;
	}

	if (BoundInventory)
	{
		BoundInventory->OnInventoryChanged.RemoveAll(this);
	}

	BoundInventory = InInventory;

	if (BoundInventory)
	{
		BoundInventory->OnInventoryChanged.AddUObject(this, &UInventoryScreen::HandleInventoryChanged);
	}
}

void UInventoryScreen::RebuildSlots()
{
	// Rebuild the bag grid only when the row layout or component changed.
	const int32 MaxSlots = GetMaxBagSlots(BoundInventory);
	const int32 Columns = FMath::Max(1, FMath::CeilToInt(static_cast<float>(MaxSlots) / static_cast<float>(GridRows)));
	const int32 DesiredCount = GridRows * Columns;

	if (!BagGrid || BagSlots.Num() == DesiredCount)
	{
		RefreshBag();
		RefreshEquipment();
		return;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	if (!OwningPlayer || !InventorySlotWidgetClass)
	{
		return;
	}

	BagGrid->ClearChildren();
	BagSlots.Empty();

	for (int32 Index = 0; Index < DesiredCount; ++Index)
	{
		UInventorySlotWidget* SlotWidget = CreateWidget<UInventorySlotWidget>(OwningPlayer, InventorySlotWidgetClass);
		if (!SlotWidget)
		{
			continue;
		}

		const int32 Row = Index / Columns;
		const int32 Col = Index % Columns;
		BagGrid->AddChildToUniformGrid(SlotWidget, Row, Col);
		SlotWidget->SetSlotIndex(Index);
		SlotWidget->OnSlotClicked.AddDynamic(this, &UInventoryScreen::HandleSlotClicked);
		BagSlots.Add(SlotWidget);
	}

	RefreshBag();
	RefreshEquipment();
}

void UInventoryScreen::RefreshBag()
{
	const TArray<FOnsetInventoryEntry>& Items = BoundInventory ? BoundInventory->GetItems() : TArray<FOnsetInventoryEntry>();

	for (int32 Index = 0; Index < BagSlots.Num(); ++Index)
	{
		UInventorySlotWidget* SlotWidget = BagSlots[Index];
		if (!SlotWidget)
		{
			continue;
		}

		if (Index < Items.Num())
		{
			SlotWidget->SetItem(Items[Index]);
		}
		else
		{
			SlotWidget->SetEmpty();
		}
	}
}

void UInventoryScreen::RefreshEquipment()
{
	if (!BoundInventory)
	{
		return;
	}

	for (uint8 SlotValue = 0; SlotValue < static_cast<uint8>(EOnsetEquipmentSlot::Trinket) + 1; ++SlotValue)
	{
		const EOnsetEquipmentSlot EquipSlot = static_cast<EOnsetEquipmentSlot>(SlotValue);
		UInventorySlotWidget* SlotWidget = GetEquipmentSlotWidget(EquipSlot);
		if (!SlotWidget)
		{
			continue;
		}

		const FName RowName = BoundInventory->GetEquippedRow(EquipSlot);
		if (RowName.IsNone())
		{
			SlotWidget->SetEquipmentEmpty(EquipSlot);
		}
		else
		{
			SlotWidget->SetEquipment(EquipSlot, RowName);
		}
	}
}

void UInventoryScreen::HandleInventoryChanged()
{
	RefreshBag();
	RefreshEquipment();
}

void UInventoryScreen::HandleSlotClicked(int32 SlotIndex)
{
	// Reserved for future interactions (equip/use/compare/drop). The bag UI
	// skeleton only renders + refreshes the grid for now.
	UE_LOG(LogTemp, Log, TEXT("InventoryScreen: bag slot %d clicked (interaction not yet wired)"), SlotIndex);
}

UInventorySlotWidget* UInventoryScreen::GetEquipmentSlotWidget(EOnsetEquipmentSlot EquipSlot) const
{
	switch (EquipSlot)
	{
		case EOnsetEquipmentSlot::Weapon:	return WeaponSlot;
		case EOnsetEquipmentSlot::Shield:	return ShieldSlot;
		case EOnsetEquipmentSlot::Head:		return HeadSlot;
		case EOnsetEquipmentSlot::Chest:	return ChestSlot;
		case EOnsetEquipmentSlot::Hands:	return HandsSlot;
		case EOnsetEquipmentSlot::Legs:		return LegsSlot;
		case EOnsetEquipmentSlot::Feet:		return FeetSlot;
		case EOnsetEquipmentSlot::Amulet:	return AmuletSlot;
		case EOnsetEquipmentSlot::Ring1:	return Ring1Slot;
		case EOnsetEquipmentSlot::Ring2:	return Ring2Slot;
		case EOnsetEquipmentSlot::Trinket:	return TrinketSlot;
		default:							return nullptr;
	}
}