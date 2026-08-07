// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/TargetHUDWidget.h"

#include "AbilitySystemComponent.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Core/OnsetBaseCharacter.h"
#include "GAS/OnsetAttributeSet.h"
#include "GameFramework/PlayerController.h"

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
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
			{
				// The designer placed this slot center-anchored; re-anchor to
				// top-left so SetPosition is absolute from the canvas origin
				// (keeping alignment 0.5 so the widget centers on the point).
				if (CanvasSlot->GetAnchors() != FAnchors(0.0f, 0.0f))
				{
					CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f));
				}

				// Convert viewport pixels to the HUD's layout units.
				const float GeometryScale = MyGeometry.Scale > 0.0f ? MyGeometry.Scale : 1.0f;
				ScreenPos /= GeometryScale;

				static bool bLoggedSlot = false;
				if (!bLoggedSlot)
				{
					bLoggedSlot = true;
					const FAnchors Anchors = CanvasSlot->GetAnchors();
					UE_LOG(LogTemp, Warning, TEXT("[HUDDiag] TargetHUD slot: Anchors=(%s,%s,%s,%s) Alignment=%s SlotPos=%s Size=%s LayoutPos=%s"),
						*FString::SanitizeFloat(Anchors.Minimum.X), *FString::SanitizeFloat(Anchors.Minimum.Y),
						*FString::SanitizeFloat(Anchors.Maximum.X), *FString::SanitizeFloat(Anchors.Maximum.Y),
						*CanvasSlot->GetAlignment().ToString(), *CanvasSlot->GetPosition().ToString(), *CanvasSlot->GetSize().ToString(),
						*ScreenPos.ToString());
				}
				CanvasSlot->SetPosition(ScreenPos);
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
	if (!BoundASC)
	{
		return;
	}

	const float MaxHealth = BoundASC->GetNumericAttribute(UOnsetAttributeSet::GetMaxHealthAttribute());
	const float Health = BoundASC->GetNumericAttribute(UOnsetAttributeSet::GetHealthAttribute());
	const float NewPercent = MaxHealth > 0.0f ? FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f) : 0.0f;

	if (TargetHealthPercent != NewPercent)
	{
		TargetHealthPercent = NewPercent;
		OnTargetHealthPercentChanged(TargetHealthPercent);
	}
}

void UTargetHUDWidget::SetVisibleState(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
