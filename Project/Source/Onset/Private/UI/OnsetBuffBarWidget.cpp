// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/OnsetBuffBarWidget.h"

#include "AbilitySystemComponent.h"
#include "CommonTextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "GAS/OnsetGameplayTags.h"

namespace
{
	struct FOnsetBuffTagDef
	{
		FGameplayTag Tag;
		bool bIsDebuff;
	};
}

void UOnsetBuffIconWidget::SetupForTag(FGameplayTag Tag, bool bIsDebuff, float Duration)
{
	TrackedTag = Tag;
	bIsNegative = bIsDebuff;

	if (DurationText)
	{
		if (Duration > 0.0f)
		{
			DurationText->SetText(FText::AsNumber(FMath::CeilToInt(Duration)));
			DurationText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			DurationText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (IconImage)
	{
		// Tint red for debuffs, cyan/gold for buffs
		const FLinearColor Tint = bIsDebuff ? FLinearColor(0.95f, 0.25f, 0.25f) : FLinearColor(0.2f, 0.85f, 0.95f);
		IconImage->SetColorAndOpacity(Tint);
	}
}

void UOnsetBuffBarWidget::BindToASC(UAbilitySystemComponent* InASC)
{
	BoundASC = InASC;
	RefreshActiveTags();
}

void UOnsetBuffBarWidget::NativeDestruct()
{
	BoundASC = nullptr;
	ActiveIconWidgets.Reset();
	Super::NativeDestruct();
}

void UOnsetBuffBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshActiveTags();
}

void UOnsetBuffBarWidget::RefreshActiveTags()
{
	if (!BoundASC.IsValid() || !BuffContainer)
	{
		return;
	}

	UAbilitySystemComponent* ASC = BoundASC.Get();

	// List of known tactical status tags to track
	const FOnsetBuffTagDef TrackedTags[] = {
		{ TAG_State_Stunned, true },
		{ TAG_State_Frozen, true },
		{ TAG_State_Staggered, true },
		{ TAG_State_Invulnerable, false },
		{ FGameplayTag::RequestGameplayTag(FName(TEXT("Debuff.Burn")), false), true },
		{ FGameplayTag::RequestGameplayTag(FName(TEXT("Debuff.Slow")), false), true },
		{ FGameplayTag::RequestGameplayTag(FName(TEXT("Buff.Haste")), false), false }
	};

	for (const FOnsetBuffTagDef& Def : TrackedTags)
	{
		if (!Def.Tag.IsValid())
		{
			continue;
		}

		const bool bHasTag = ASC->HasMatchingGameplayTag(Def.Tag);
		TObjectPtr<UOnsetBuffIconWidget>* Existing = ActiveIconWidgets.Find(Def.Tag);

		if (bHasTag)
		{
			if (!Existing && BuffIconWidgetClass)
			{
				UOnsetBuffIconWidget* NewIcon = CreateWidget<UOnsetBuffIconWidget>(this, BuffIconWidgetClass);
				if (NewIcon)
				{
					NewIcon->SetupForTag(Def.Tag, Def.bIsDebuff);
					BuffContainer->AddChildToHorizontalBox(NewIcon);
					ActiveIconWidgets.Add(Def.Tag, NewIcon);
				}
			}
		}
		else if (Existing)
		{
			if (*Existing)
			{
				BuffContainer->RemoveChild(*Existing);
			}
			ActiveIconWidgets.Remove(Def.Tag);
		}
	}
}
