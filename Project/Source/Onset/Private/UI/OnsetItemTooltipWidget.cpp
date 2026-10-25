// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/OnsetItemTooltipWidget.h"

#include "Combat/OnsetEquipmentLibrary.h"
#include "CommonTextBlock.h"
#include "Components/ProgressBar.h"
#include "Data/OnsetItemLibrary.h"

void UOnsetItemTooltipWidget::SetItemEntry(const FOnsetInventoryEntry& Entry)
{
	const FOnsetItemDefinition* BaseDef = UOnsetItemLibrary::GetItemDefinition(Entry.Category, Entry.RowName);

	if (ItemNameText)
	{
		const FText Name = BaseDef && !BaseDef->DisplayName.IsEmpty() ? BaseDef->DisplayName : FText::FromName(Entry.RowName);
		ItemNameText->SetText(Name);
	}

	if (RarityText)
	{
		const UEnum* RarityEnum = StaticEnum<EOnsetItemRarity>();
		const FText RarityStr = (BaseDef && RarityEnum) ? RarityEnum->GetDisplayValueAsText(BaseDef->Rarity) : FText::GetEmpty();
		RarityText->SetText(RarityStr);
	}

	if (CategoryText)
	{
		const UEnum* CatEnum = StaticEnum<EOnsetItemCategory>();
		const FText CatStr = CatEnum ? CatEnum->GetDisplayValueAsText(Entry.Category) : FText::GetEmpty();
		CategoryText->SetText(CatStr);
	}

	if (DescriptionText)
	{
		DescriptionText->SetText(BaseDef ? BaseDef->Description : FText::GetEmpty());
	}

	if (Entry.Category == EOnsetItemCategory::Equipment)
	{
		const FOnsetEquipmentDefinition* EquipDef = static_cast<const FOnsetEquipmentDefinition*>(BaseDef);

		if (GradeBadgeText)
		{
			GradeBadgeText->SetText(UOnsetEquipmentLibrary::GetGradeDisplayText(Entry.InstanceData.PrimaryStatGrade));
			GradeBadgeText->SetColorAndOpacity(FSlateColor(UOnsetEquipmentLibrary::GetGradeColor(Entry.InstanceData.PrimaryStatGrade)));
			GradeBadgeText->SetVisibility(ESlateVisibility::Visible);
		}

		const float DurabilityPercent = Entry.InstanceData.GetDurabilityPercent();
		if (DurabilityBar)
		{
			DurabilityBar->SetPercent(DurabilityPercent);
			DurabilityBar->SetVisibility(ESlateVisibility::Visible);
		}

		if (DurabilityText)
		{
			DurabilityText->SetText(FText::Format(
				NSLOCTEXT("OnsetTooltip", "DurabilityFmt", "Durability: {0} / {1} ({2}%)"),
				FText::AsNumber(FMath::RoundToInt(Entry.InstanceData.CurrentDurability)),
				FText::AsNumber(FMath::RoundToInt(Entry.InstanceData.MaxDurability)),
				FText::AsNumber(FMath::RoundToInt(DurabilityPercent * 100.0f))
			));
			DurabilityText->SetVisibility(ESlateVisibility::Visible);
		}

		if (MalfunctioningBanner)
		{
			MalfunctioningBanner->SetVisibility(Entry.InstanceData.IsMalfunctioning() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}

		if (PrimaryStatText)
		{
			if (EquipDef && EquipDef->Slot == EOnsetEquipmentSlot::Weapon)
			{
				const float EffectiveDmg = UOnsetEquipmentLibrary::CalculateEffectiveWeaponDamage(EquipDef, Entry.InstanceData);
				PrimaryStatText->SetText(FText::Format(
					NSLOCTEXT("OnsetTooltip", "WeaponDamageFmt", "Weapon Damage: {0}"),
					FText::AsNumber(FMath::RoundToInt(EffectiveDmg))
				));
			}
			else if (EquipDef)
			{
				const float EffectiveDef = UOnsetEquipmentLibrary::CalculateEffectiveDefense(EquipDef, Entry.InstanceData);
				PrimaryStatText->SetText(FText::Format(
					NSLOCTEXT("OnsetTooltip", "DefenseFmt", "Defense Bonus: +{0}"),
					FText::AsNumber(FMath::RoundToInt(EffectiveDef))
				));
			}
			PrimaryStatText->SetVisibility(ESlateVisibility::Visible);
		}

		if (SecondaryStatsText && EquipDef)
		{
			TArray<FString> StatLines;
			auto AppendBonus = [&](const TCHAR* StatName, float BaseVal)
			{
				if (BaseVal > 0.0f)
				{
					const float Effective = UOnsetEquipmentLibrary::CalculateEffectiveStatBonus(BaseVal, Entry.InstanceData);
					StatLines.Add(FString::Printf(TEXT("+%d %s"), FMath::RoundToInt(Effective), StatName));
				}
			};

			AppendBonus(TEXT("STR"), EquipDef->StrengthBonus);
			AppendBonus(TEXT("INT"), EquipDef->IntellectBonus);
			AppendBonus(TEXT("VIT"), EquipDef->VitalityBonus);
			AppendBonus(TEXT("AGI"), EquipDef->AgilityBonus);
			AppendBonus(TEXT("LUK"), EquipDef->LuckBonus);

			SecondaryStatsText->SetText(FText::FromString(FString::Join(StatLines, TEXT(" | "))));
			SecondaryStatsText->SetVisibility(StatLines.Num() > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}

		if (InfusionLevelText)
		{
			InfusionLevelText->SetText(FText::Format(
				NSLOCTEXT("OnsetTooltip", "InfusionLevelFmt", "Infusion Rank {0}"),
				FText::AsNumber(Entry.InstanceData.InfusionLevel)
			));
			InfusionLevelText->SetVisibility(ESlateVisibility::Visible);
		}

		if (InfusionProgressBar)
		{
			InfusionProgressBar->SetPercent(Entry.InstanceData.InfusionProgress);
			InfusionProgressBar->SetVisibility(ESlateVisibility::Visible);
		}
	}
	else
	{
		if (GradeBadgeText) GradeBadgeText->SetVisibility(ESlateVisibility::Collapsed);
		if (DurabilityBar) DurabilityBar->SetVisibility(ESlateVisibility::Collapsed);
		if (DurabilityText) DurabilityText->SetVisibility(ESlateVisibility::Collapsed);
		if (MalfunctioningBanner) MalfunctioningBanner->SetVisibility(ESlateVisibility::Collapsed);
		if (PrimaryStatText) PrimaryStatText->SetVisibility(ESlateVisibility::Collapsed);
		if (SecondaryStatsText) SecondaryStatsText->SetVisibility(ESlateVisibility::Collapsed);
		if (InfusionLevelText) InfusionLevelText->SetVisibility(ESlateVisibility::Collapsed);
		if (InfusionProgressBar) InfusionProgressBar->SetVisibility(ESlateVisibility::Collapsed);
	}

	BP_OnTooltipDataUpdated();
}

void UOnsetItemTooltipWidget::SetEquippedEntry(EOnsetEquipmentSlot EquipSlot, FName RowName, const FOnsetEquipmentInstanceData& InstanceData)
{
	FOnsetInventoryEntry SyntheticEntry;
	SyntheticEntry.Category = EOnsetItemCategory::Equipment;
	SyntheticEntry.RowName = RowName;
	SyntheticEntry.Count = 1;
	SyntheticEntry.InstanceData = InstanceData;
	SetItemEntry(SyntheticEntry);
}
