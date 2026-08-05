// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/AbilityBarWidget.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameplayEffect.h"
#include "GAS/OnsetGameplayTags.h"
#include "Player/OnsetPlayerController.h"
#include "Styling/CoreStyle.h"
#include "Fonts/SlateFontInfo.h"
#include "TimerManager.h"
#include "Engine/World.h"

void UAbilityBarWidget::BindToPlayer(AOnsetPlayerController* InPlayerController, UAbilitySystemComponent* InASC)
{
	if (BoundASC && BoundASC != InASC)
	{
		for (FDelegateHandle& Handle : CooldownTagHandles)
		{
			if (Handle.IsValid())
			{
				BoundASC->UnregisterGameplayTagEvent(Handle, TAG_Cooldown_BasicAttack, EGameplayTagEventType::AnyCountChange);
				BoundASC->UnregisterGameplayTagEvent(Handle, TAG_Cooldown_AoE, EGameplayTagEventType::AnyCountChange);
				BoundASC->UnregisterGameplayTagEvent(Handle, TAG_Cooldown_Cone, EGameplayTagEventType::AnyCountChange);
				BoundASC->UnregisterGameplayTagEvent(Handle, TAG_Cooldown_Shadowstep, EGameplayTagEventType::AnyCountChange);
				Handle.Reset();
			}
		}
	}

	BoundPlayerController = InPlayerController;
	BoundASC = InASC;

	if (BoundASC)
	{
		const FGameplayTag CooldownTags[] = { TAG_Cooldown_BasicAttack, TAG_Cooldown_AoE, TAG_Cooldown_Cone, TAG_Cooldown_Shadowstep };
		CooldownTagHandles.Reset();
		for (const FGameplayTag& Tag : CooldownTags)
		{
			CooldownTagHandles.Add(
				BoundASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::AnyCountChange)
					.AddUObject(this, &UAbilityBarWidget::HandleCooldownTagChanged));
		}
	}

	NotifyAbilitiesChanged();
}

void UAbilityBarWidget::HandleCooldownTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	// Tag added (cooldown GE applied) — start the lightweight poll. Tag removed — refresh once.
	NotifyAbilitiesChanged();
}

void UAbilityBarWidget::NotifyAbilitiesChanged()
{
	if (RefreshCooldowns())
	{
		if (BoundASC && GetWorld() && !GetWorld()->GetTimerManager().IsTimerActive(CooldownTimerHandle))
		{
			GetWorld()->GetTimerManager().SetTimer(CooldownTimerHandle, this, &UAbilityBarWidget::OnCooldownTick, 0.1f, true);
		}
	}
}

void UAbilityBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree)
	{
		return;
	}

	if (UCanvasPanel* RootPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootPanel")))
	{
		WidgetTree->RootWidget = RootPanel;
		InitCooldownTags();
		BuildSlotLayout(RootPanel);
	}
}

void UAbilityBarWidget::NativeDestruct()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(CooldownTimerHandle);
	}

	if (BoundASC)
	{
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
		BoundASC = nullptr;
	}

	Super::NativeDestruct();
}

void UAbilityBarWidget::OnCooldownTick()
{
	if (!RefreshCooldowns())
	{
		GetWorld()->GetTimerManager().ClearTimer(CooldownTimerHandle);
	}
}

bool UAbilityBarWidget::RefreshCooldowns()
{
	bool bAnyActive = false;

	for (FAbilitySlot& Entry : Slots)
	{
		if (!BoundASC || !Entry.CooldownTag.IsValid())
		{
			continue;
		}

		float TimeRemaining = 0.0f;
		float Duration = 0.0f;

		const TArray<FActiveGameplayEffectHandle> ActiveHandles =
			BoundASC->GetActiveEffectsWithAllTags(FGameplayTagContainer(Entry.CooldownTag));
		for (const FActiveGameplayEffectHandle& ActiveHandle : ActiveHandles)
		{
			const FActiveGameplayEffect* ActiveEffect = BoundASC->GetActiveGameplayEffects().GetActiveGameplayEffect(ActiveHandle);
			if (!ActiveEffect)
			{
				continue;
			}

			Duration = ActiveEffect->GetDuration();
			if (Duration > 0.0f)
			{
				const float WorldTime = GetWorld()->GetTimeSeconds();
				const float Remaining = ActiveEffect->GetTimeRemaining(WorldTime);
				TimeRemaining = FMath::Max(TimeRemaining, Remaining);
			}
		}

		if (TimeRemaining > 0.0f)
		{
			bAnyActive = true;
		}

		if (Entry.CooldownBar)
		{
			Entry.CooldownBar->SetPercent(Duration > 0.0f ? FMath::Clamp(TimeRemaining / Duration, 0.0f, 1.0f) : 0.0f);
			Entry.CooldownBar->SetVisibility(TimeRemaining > 0.0f ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}

		if (Entry.CooldownText)
		{
			if (TimeRemaining > 0.0f)
			{
				Entry.CooldownText->SetText(FText::AsNumber(FMath::CeilToInt(TimeRemaining)));
				Entry.CooldownText->SetVisibility(ESlateVisibility::Visible);
			}
			else
			{
				Entry.CooldownText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}

	return bAnyActive;
}

void UAbilityBarWidget::HandleSlot0Clicked()
{
	if (BoundPlayerController)
	{
		BoundPlayerController->StartAutoAttack();
	}
}

void UAbilityBarWidget::HandleSlot1Clicked()
{
	if (BoundASC)
	{
		BoundASC->AbilityLocalInputPressed(1);
		BoundASC->AbilityLocalInputReleased(1);
	}
}

void UAbilityBarWidget::HandleSlot2Clicked()
{
	if (BoundASC)
	{
		BoundASC->AbilityLocalInputPressed(2);
		BoundASC->AbilityLocalInputReleased(2);
	}
}

void UAbilityBarWidget::HandleSlot3Clicked()
{
	// Shadowstep is a passive — no manual trigger.
}

void UAbilityBarWidget::InitCooldownTags()
{
	if (Slots.Num() != 4)
	{
		Slots.SetNum(4);
	}

	Slots[0].CooldownTag = TAG_Cooldown_BasicAttack;
	Slots[0].InputID = 0;
	Slots[1].CooldownTag = TAG_Cooldown_AoE;
	Slots[1].InputID = 1;
	Slots[2].CooldownTag = TAG_Cooldown_Cone;
	Slots[2].InputID = 2;
	Slots[3].CooldownTag = TAG_Cooldown_Shadowstep;
	Slots[3].InputID = -1;
}

void UAbilityBarWidget::BuildSlotLayout(UCanvasPanel* RootPanel)
{
	const int32 SlotCount = 4;

	for (int32 i = 0; i < SlotCount; ++i)
	{
		FAbilitySlot& Entry = Slots[i];

		Entry.Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *FString::Printf(TEXT("SlotButton_%d"), i));
		Entry.CooldownBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), *FString::Printf(TEXT("CooldownBar_%d"), i));
		Entry.KeyLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("KeyLabel_%d"), i));
		Entry.CooldownText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("CooldownText_%d"), i));

		if (Entry.KeyLabel)
		{
			Entry.KeyLabel->SetText(FText::FromString(GetSlotKeyLabel(i)));
			Entry.KeyLabel->SetJustification(ETextJustify::Center);
			Entry.KeyLabel->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 10));
		}

		if (Entry.CooldownText)
		{
			Entry.CooldownText->SetJustification(ETextJustify::Center);
			Entry.CooldownText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 12, TEXT("Bold")));
			Entry.CooldownText->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (Entry.CooldownBar)
		{
			Entry.CooldownBar->SetFillColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));
			Entry.CooldownBar->SetPercent(0.0f);
			Entry.CooldownBar->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (Entry.Button)
		{
			switch (i)
			{
			case 0:
				Entry.Button->OnClicked.AddDynamic(this, &UAbilityBarWidget::HandleSlot0Clicked);
				break;
			case 1:
				Entry.Button->OnClicked.AddDynamic(this, &UAbilityBarWidget::HandleSlot1Clicked);
				break;
			case 2:
				Entry.Button->OnClicked.AddDynamic(this, &UAbilityBarWidget::HandleSlot2Clicked);
				break;
			case 3:
				Entry.Button->OnClicked.AddDynamic(this, &UAbilityBarWidget::HandleSlot3Clicked);
				break;
			default:
				break;
			}

			UCanvasPanelSlot* ButtonSlot = RootPanel->AddChildToCanvas(Entry.Button);
			if (ButtonSlot)
			{
				ButtonSlot->SetAnchors(FAnchors(0.5f, 1.0f));
				ButtonSlot->SetAlignment(FVector2D(0.5f, 1.0f));
				ButtonSlot->SetSize(FVector2D(56.0f, 56.0f));
				ButtonSlot->SetPosition(FVector2D((i - (SlotCount - 1) * 0.5f) * 64.0f, -30.0f));
			}
		}

		if (Entry.CooldownBar)
		{
			UCanvasPanelSlot* BarSlot = RootPanel->AddChildToCanvas(Entry.CooldownBar);
			if (BarSlot)
			{
				BarSlot->SetAnchors(FAnchors(0.5f, 1.0f));
				BarSlot->SetAlignment(FVector2D(0.5f, 1.0f));
				BarSlot->SetSize(FVector2D(56.0f, 56.0f));
				BarSlot->SetPosition(FVector2D((i - (SlotCount - 1) * 0.5f) * 64.0f, -30.0f));
			}
		}

		if (Entry.KeyLabel)
		{
			UCanvasPanelSlot* KeySlot = RootPanel->AddChildToCanvas(Entry.KeyLabel);
			if (KeySlot)
			{
				KeySlot->SetAnchors(FAnchors(0.5f, 1.0f));
				KeySlot->SetAlignment(FVector2D(0.5f, 1.0f));
				KeySlot->SetAutoSize(true);
				KeySlot->SetPosition(FVector2D((i - (SlotCount - 1) * 0.5f) * 64.0f, 30.0f));
			}
		}

		if (Entry.CooldownText)
		{
			UCanvasPanelSlot* TextSlot = RootPanel->AddChildToCanvas(Entry.CooldownText);
			if (TextSlot)
			{
				TextSlot->SetAnchors(FAnchors(0.5f, 1.0f));
				TextSlot->SetAlignment(FVector2D(0.5f, 1.0f));
				TextSlot->SetAutoSize(true);
				TextSlot->SetPosition(FVector2D((i - (SlotCount - 1) * 0.5f) * 64.0f, -30.0f));
			}
		}
	}
}

const TCHAR* UAbilityBarWidget::GetSlotKeyLabel(int32 SlotIndex)
{
	switch (SlotIndex)
	{
	case 0: return TEXT("1");
	case 1: return TEXT("2");
	case 2: return TEXT("3");
	case 3: return TEXT("4");
	default: return TEXT("");
	}
}
