// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/TargetHUDWidget.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Core/OnsetBaseCharacter.h"
#include "GAS/OnsetAttributeSet.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"
#include "Fonts/SlateFontInfo.h"

void UTargetHUDWidget::SetTarget(AOnsetBaseCharacter* InTarget)
{
	if (BoundASC)
	{
		BoundASC->GetGameplayAttributeValueChangeDelegate(UOnsetAttributeSet::GetHealthAttribute()).RemoveAll(this);
		BoundASC = nullptr;
	}

	TrackedTarget = IsValid(InTarget) ? InTarget : nullptr;

	if (TrackedTarget)
	{
		BoundASC = TrackedTarget->AbilitySystemComponent;
		if (BoundASC)
		{
			BoundASC->GetGameplayAttributeValueChangeDelegate(UOnsetAttributeSet::GetHealthAttribute()).AddUObject(this, &UTargetHUDWidget::HandleTargetHealthChanged);
		}

		if (NameText)
		{
			NameText->SetText(FText::FromString(TrackedTarget->GetName()));
		}
	}

	RefreshHealth();
	SetVisibleState(TrackedTarget != nullptr);
}

void UTargetHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree)
	{
		return;
	}

	// Root canvas fills the viewport; the element content lives in a positioned child panel.
	if (UCanvasPanel* RootPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootPanel")))
	{
		WidgetTree->RootWidget = RootPanel;

		UCanvasPanel* ContentPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ContentPanel"));
		if (ContentPanel)
		{
			ContentPanelSlot = RootPanel->AddChildToCanvas(ContentPanel);
			if (ContentPanelSlot)
			{
				ContentPanelSlot->SetAnchors(FAnchors(0.0f, 0.0f));
				ContentPanelSlot->SetAlignment(FVector2D(0.0f, 0.0f));
				ContentPanelSlot->SetAutoSize(true);
			}

			if (!ReticleBorder)
			{
				ReticleBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ReticleBorder"));
				if (ReticleBorder)
				{
					ReticleBorder->SetBrushColor(FLinearColor(1.0f, 0.0f, 0.0f, 1.0f));
					ReticleBorder->SetContentColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
					ReticleBorder->SetPadding(FMargin(24.0f));

					UCanvasPanelSlot* ReticleSlot = ContentPanel->AddChildToCanvas(ReticleBorder);
					if (ReticleSlot)
					{
						ReticleSlot->SetAnchors(FAnchors(0.5f, 0.5f));
						ReticleSlot->SetAlignment(FVector2D(0.5f, 0.5f));
						ReticleSlot->SetAutoSize(true);
					}
				}
			}

			if (!NameText)
			{
				NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NameText"));
				if (NameText)
				{
					NameText->SetJustification(ETextJustify::Center);
					NameText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 12, TEXT("Bold")));

					UCanvasPanelSlot* NameSlot = ContentPanel->AddChildToCanvas(NameText);
					if (NameSlot)
					{
						NameSlot->SetAnchors(FAnchors(0.5f, 0.5f));
						NameSlot->SetAlignment(FVector2D(0.5f, 0.5f));
						NameSlot->SetAutoSize(true);
						NameSlot->SetPosition(FVector2D(0.0f, -28.0f));
					}
				}
			}

			if (!HealthBar)
			{
				HealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthBar"));
				if (HealthBar)
				{
					HealthBar->SetFillColorAndOpacity(FLinearColor(0.6f, 0.0f, 0.0f, 1.0f));

					UCanvasPanelSlot* BarSlot = ContentPanel->AddChildToCanvas(HealthBar);
					if (BarSlot)
					{
						BarSlot->SetAnchors(FAnchors(0.5f, 0.5f));
						BarSlot->SetAlignment(FVector2D(0.5f, 0.5f));
						BarSlot->SetSize(FVector2D(120.0f, 8.0f));
						BarSlot->SetPosition(FVector2D(0.0f, 8.0f));
					}
				}
			}
		}
	}

	SetVisibleState(TrackedTarget != nullptr);
}

void UTargetHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!TrackedTarget || !IsValid(TrackedTarget))
	{
		if (TrackedTarget == nullptr)
		{
			SetVisibleState(false);
		}
		else
		{
			SetTarget(nullptr);
		}
		return;
	}

	if (!TrackedTarget->IsAlive())
	{
		SetTarget(nullptr);
		return;
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		FVector2D ScreenPos;
		if (PC->ProjectWorldLocationToScreen(TrackedTarget->GetActorLocation() + FVector(0.0f, 0.0f, 120.0f), ScreenPos, false))
		{
			if (ContentPanelSlot)
			{
				ContentPanelSlot->SetPosition(ScreenPos);
			}
			SetVisibleState(true);
		}
		else
		{
			SetVisibleState(false);
		}
	}
}

void UTargetHUDWidget::NativeDestruct()
{
	if (BoundASC)
	{
		BoundASC->GetGameplayAttributeValueChangeDelegate(UOnsetAttributeSet::GetHealthAttribute()).RemoveAll(this);
		BoundASC = nullptr;
	}

	Super::NativeDestruct();
}

void UTargetHUDWidget::HandleTargetHealthChanged(const FOnAttributeChangeData& Data)
{
	RefreshHealth();
}

void UTargetHUDWidget::RefreshHealth()
{
	if (!BoundASC || !HealthBar)
	{
		return;
	}

	const float MaxHealth = BoundASC->GetNumericAttribute(UOnsetAttributeSet::GetMaxHealthAttribute());
	const float Health = BoundASC->GetNumericAttribute(UOnsetAttributeSet::GetHealthAttribute());
	HealthBar->SetPercent(MaxHealth > 0.0f ? FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f) : 0.0f);
}

void UTargetHUDWidget::SetVisibleState(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
