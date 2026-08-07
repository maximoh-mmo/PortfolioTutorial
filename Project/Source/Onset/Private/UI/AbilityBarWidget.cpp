// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/AbilityBarWidget.h"

#include "AbilitySystemComponent.h"
#include "Combat/OnsetGameplayAbility.h"
#include "GAS/OnsetGameplayTags.h"
#include "GameplayAbilitySpec.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "UI/AbilitySlotWidget.h"

namespace AbilityBarInputIDs
{
	// Slots are bound to GAS input IDs 1-4, matching IA_Ability1-4 triggers.
	constexpr int32 FirstInputID = 1;
	constexpr int32 SlotCount = 4;
}

UAbilityBarWidget::UAbilityBarWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UAbilityBarWidget::BindToPlayer(AOnsetPlayerController* InPlayerController, UAbilitySystemComponent* InASC)
{
	if (BoundASC && BoundASC != InASC)
	{
		UnregisterCooldownEvents();
	}

	BoundPlayerController = InPlayerController;
	BoundASC = InASC;

	BuildSlots();
	RebuildSlots();
}

void UAbilityBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuildSlots();
	RebuildSlots();
}

void UAbilityBarWidget::NativeDestruct()
{
	UnregisterCooldownEvents();

	if (BoundASC)
	{
		BoundASC = nullptr;
	}

	Super::NativeDestruct();
}

void UAbilityBarWidget::BuildSlots()
{
	if (Slots.Num() > 0)
	{
		return;
	}

	if (!SlotContainer)
	{
		return;
	}

	Slots.SetNum(AbilityBarInputIDs::SlotCount);

	APlayerController* OwningPC = GetOwningPlayer();

	for (int32 i = 0; i < AbilityBarInputIDs::SlotCount; ++i)
	{
		UAbilitySlotWidget* SlotWidget = nullptr;
		if (AbilitySlotWidgetClass)
		{
			SlotWidget = CreateWidget<UAbilitySlotWidget>(OwningPC, AbilitySlotWidgetClass);
		}
		if (!SlotWidget)
		{
			continue;
		}

		const int32 InputID = AbilityBarInputIDs::FirstInputID + i;
		SlotWidget->SetSlotInfo(i, FText::FromString(FString::Printf(TEXT("%d"), InputID)));
		SlotWidget->OnSlotClicked.AddDynamic(this, &UAbilityBarWidget::HandleSlotClicked);

		if (UHorizontalBoxSlot* BoxSlot = SlotContainer->AddChildToHorizontalBox(SlotWidget))
		{
			BoxSlot->SetPadding(FMargin(4.0f));
		}

		Slots[i].InputID = InputID;
		Slots[i].Widget = SlotWidget;
	}
}

void UAbilityBarWidget::RebuildSlots()
{
	if (!BoundASC)
	{
		return;
	}

	UnregisterCooldownEvents();

	for (FSlotEntry& Entry : Slots)
	{
		if (!Entry.Widget)
		{
			continue;
		}

		Entry.CooldownTag = FGameplayTag();
		Entry.Widget->SetLocked(true);

		if (FGameplayAbilitySpec* Spec = BoundASC->FindAbilitySpecFromInputID(Entry.InputID))
		{
			if (UOnsetGameplayAbility* AbilityCDO = Cast<UOnsetGameplayAbility>(Spec->Ability))
			{
				Entry.Widget->SetAbility(AbilityCDO->AbilityIcon, AbilityCDO->GetPrimaryCooldownTag());
				Entry.CooldownTag = AbilityCDO->GetPrimaryCooldownTag();
			}
		}

		if (Entry.CooldownTag.IsValid())
		{
			CooldownTagHandles.Add(
				BoundASC->RegisterGameplayTagEvent(Entry.CooldownTag, EGameplayTagEventType::AnyCountChange)
					.AddUObject(this, &UAbilityBarWidget::HandleCooldownTagChanged));
		}
	}

	// Restore any cooldown already active before this widget (re)built.
	for (const FSlotEntry& Entry : Slots)
	{
		if (Entry.Widget && Entry.CooldownTag.IsValid() && BoundASC->HasMatchingGameplayTag(Entry.CooldownTag))
		{
			SyncCooldownState(Entry.CooldownTag, 1);
		}
	}
}

void UAbilityBarWidget::UnregisterCooldownEvents()
{
	if (!BoundASC)
	{
		return;
	}

	for (const FDelegateHandle& Handle : CooldownTagHandles)
	{
		if (Handle.IsValid())
		{
			BoundASC->UnregisterGameplayTagEvent(Handle, TAG_Cooldown_BasicAttack, EGameplayTagEventType::AnyCountChange);
			BoundASC->UnregisterGameplayTagEvent(Handle, TAG_Cooldown_AoE, EGameplayTagEventType::AnyCountChange);
			BoundASC->UnregisterGameplayTagEvent(Handle, TAG_Cooldown_Cone, EGameplayTagEventType::AnyCountChange);
			BoundASC->UnregisterGameplayTagEvent(Handle, TAG_Cooldown_Shadowstep, EGameplayTagEventType::AnyCountChange);
		}
	}
	CooldownTagHandles.Reset();
}

void UAbilityBarWidget::HandleCooldownTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	SyncCooldownState(Tag, NewCount);
}

void UAbilityBarWidget::SyncCooldownState(const FGameplayTag Tag, int32 NewCount)
{
	for (const FSlotEntry& Entry : Slots)
	{
		if (Entry.Widget && Entry.CooldownTag == Tag)
		{
			if (NewCount > 0)
			{
				Entry.Widget->StartCooldown(GetCooldownDuration(Tag));
			}
			else
			{
				Entry.Widget->EndCooldown();
			}
			break;
		}
	}
}

float UAbilityBarWidget::GetCooldownDuration(const FGameplayTag Tag) const
{
	if (!BoundASC)
	{
		return 0.0f;
	}

	const TArray<FActiveGameplayEffectHandle> ActiveHandles =
		BoundASC->GetActiveEffectsWithAllTags(FGameplayTagContainer(Tag));
	for (const FActiveGameplayEffectHandle& ActiveHandle : ActiveHandles)
	{
		const FActiveGameplayEffect* ActiveEffect = BoundASC->GetActiveGameplayEffects().GetActiveGameplayEffect(ActiveHandle);
		if (ActiveEffect)
		{
			const float Duration = ActiveEffect->GetDuration();
			if (Duration > 0.0f)
			{
				return Duration;
			}
		}
	}
	return 0.0f;
}

void UAbilityBarWidget::HandleSlotClicked(int32 SlotIndex)
{
	if (Slots.IsValidIndex(SlotIndex) && Slots[SlotIndex].InputID != INDEX_NONE && BoundASC)
	{
		BoundASC->AbilityLocalInputPressed(Slots[SlotIndex].InputID);
		BoundASC->AbilityLocalInputReleased(Slots[SlotIndex].InputID);
	}
}
