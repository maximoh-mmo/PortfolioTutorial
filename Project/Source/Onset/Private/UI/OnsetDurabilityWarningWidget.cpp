// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/OnsetDurabilityWarningWidget.h"

#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Inventory/UOnsetInventoryComponent.h"

void UOnsetDurabilityWarningWidget::BindToInventory(UOnsetInventoryComponent* InInventory)
{
	if (BoundInventory.IsValid())
	{
		BoundInventory->OnInventoryChanged.RemoveAll(this);
	}

	BoundInventory = InInventory;

	if (BoundInventory.IsValid())
	{
		BoundInventory->OnInventoryChanged.AddUObject(this, &UOnsetDurabilityWarningWidget::HandleInventoryChanged);
		HandleInventoryChanged();
	}
}

void UOnsetDurabilityWarningWidget::NativeDestruct()
{
	if (BoundInventory.IsValid())
	{
		BoundInventory->OnInventoryChanged.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UOnsetDurabilityWarningWidget::HandleInventoryChanged()
{
	if (!BoundInventory.IsValid())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	bool bHasBroken = false;
	bool bHasLow = false;

	const TArray<FOnsetEquippedEntry>& Loadout = BoundInventory->GetEquippedEntries();
	for (const FOnsetEquippedEntry& Entry : Loadout)
	{
		if (Entry.RowName.IsNone())
		{
			continue;
		}

		if (Entry.InstanceData.IsMalfunctioning())
		{
			bHasBroken = true;
			break;
		}
		else if (Entry.InstanceData.GetDurabilityPercent() < 0.20f)
		{
			bHasLow = true;
		}
	}

	if (bHasBroken)
	{
		SetVisibility(ESlateVisibility::Visible);
		if (WarningIcon) WarningIcon->SetColorAndOpacity(FLinearColor(1.0f, 0.15f, 0.15f, 1.0f)); // Flashing Red
		if (StatusText) StatusText->SetText(NSLOCTEXT("OnsetDurability", "MalfunctioningAlert", "GEAR MALFUNCTIONING"));
	}
	else if (bHasLow)
	{
		SetVisibility(ESlateVisibility::Visible);
		if (WarningIcon) WarningIcon->SetColorAndOpacity(FLinearColor(1.0f, 0.75f, 0.1f, 1.0f)); // Warning Amber
		if (StatusText) StatusText->SetText(NSLOCTEXT("OnsetDurability", "LowDurabilityAlert", "LOW DURABILITY"));
	}
	else
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}

	BP_OnDurabilityStatusChanged(bHasBroken, bHasLow);
}
