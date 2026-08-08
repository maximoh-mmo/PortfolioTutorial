// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/TargetHUDWidget.h"

#include "AbilitySystemComponent.h"
#include "Core/OnsetBaseCharacter.h"
#include "GAS/OnsetAttributeSet.h"

void UTargetHUDWidget::SetTarget(AOnsetBaseCharacter* InTarget)
{
	if (BoundASC)
	{
		BoundASC->GetGameplayAttributeValueChangeDelegate(UOnsetAttributeSet::GetHealthAttribute()).RemoveAll(this);
		BoundASC = nullptr;
	}

	// Retire the ground reticle decal on the previous target, then mark the new one.
	if (TrackedTarget)
	{
		TrackedTarget->SetTargetReticle(false);
	}

	TrackedTarget = IsValid(InTarget) ? InTarget : nullptr;

	if (TrackedTarget)
	{
		TrackedTarget->SetTargetReticle(true);
		TargetType = TrackedTarget->TargetType;

		BoundASC = TrackedTarget->AbilitySystemComponent;
		if (BoundASC)
		{
			BoundASC->GetGameplayAttributeValueChangeDelegate(UOnsetAttributeSet::GetHealthAttribute()).AddUObject(this, &UTargetHUDWidget::HandleTargetHealthChanged);
		}

		OnTargetAcquired(TargetType);
	}
	else
	{
		TargetType = ETargetType::Normal;
		OnTargetCleared();
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

	// The widget is static (designer-anchored); it never follows the target. Only
	// clear ourselves when the tracked target dies.
	if (!IsValid(TrackedTarget) || !TrackedTarget->IsAlive())
	{
		if (TrackedTarget)
		{
			SetTarget(nullptr);
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

	if (TrackedTarget)
	{
		TrackedTarget->SetTargetReticle(false);
		TrackedTarget = nullptr;
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
