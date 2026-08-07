// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/AbilitySlotWidget.h"

#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "UI/OnsetButtonBase.h"

void UAbilitySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyVisualState();
}

void UAbilitySlotWidget::SetSlotInfo(int32 InSlotIndex, const FText& InKeyLabel)
{
	SlotIndex = InSlotIndex;
	if (KeyLabel)
	{
		KeyLabel->SetText(InKeyLabel);
	}
}

void UAbilitySlotWidget::SetAbility(TSoftObjectPtr<UTexture2D> InIcon, FGameplayTag InCooldownTag)
{
	CooldownTag = InCooldownTag;

	if (AbilityIcon)
	{
		if (UTexture2D* Icon = InIcon.LoadSynchronous())
		{
			AbilityIcon->SetBrushFromTexture(Icon);
			ShowAbilityIcon();
		}
		else
		{
			ShowEmptyIcon();
		}
	}
}

void UAbilitySlotWidget::SetLocked(bool bLocked)
{
	bLocked ? ShowEmptyIcon() : ShowAbilityIcon();

	if (bLocked)
	{
		EndCooldown();
	}
}

void UAbilitySlotWidget::StartCooldown(float InDuration)
{
	if (bCooldownActive)
	{
		return;
	}
	bCooldownActive = true;
	OnCooldownStarted(InDuration);
}

void UAbilitySlotWidget::EndCooldown()
{
	if (!bCooldownActive)
	{
		return;
	}
	bCooldownActive = false;
	OnCooldownEnded();
}

void UAbilitySlotWidget::ApplyVisualState()
{
	if (SlotButton)
	{
		SlotButton->OnClicked().RemoveAll(this);
		SlotButton->OnClicked().AddUObject(this, &UAbilitySlotWidget::HandleClicked);
	}
}

void UAbilitySlotWidget::ShowAbilityIcon()
{
	if (AbilityIcon)
	{
		AbilityIcon->SetVisibility(ESlateVisibility::Visible);
	}
	if (EmptyIcon)
	{
		EmptyIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UAbilitySlotWidget::ShowEmptyIcon()
{
	if (AbilityIcon)
	{
		AbilityIcon->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (EmptyIcon)
	{
		EmptyIcon->SetVisibility(ESlateVisibility::Visible);
	}
}

void UAbilitySlotWidget::HandleClicked()
{
	OnSlotClicked.Broadcast(SlotIndex);
}
