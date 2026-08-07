// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/PlayerHealthBarWidget.h"

#include "AbilitySystemComponent.h"
#include "CommonTextBlock.h"
#include "GAS/OnsetAttributeSet.h"

void UPlayerHealthBarWidget::BindToASC(UAbilitySystemComponent* InASC)
{
	if (!InASC)
	{
		return;
	}

	if (BoundASC)
	{
		BoundASC->GetGameplayAttributeValueChangeDelegate(UOnsetAttributeSet::GetHealthAttribute()).RemoveAll(this);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UOnsetAttributeSet::GetMaxHealthAttribute()).RemoveAll(this);
	}

	BoundASC = InASC;
	BoundASC->GetGameplayAttributeValueChangeDelegate(UOnsetAttributeSet::GetHealthAttribute()).AddUObject(this, &UPlayerHealthBarWidget::HandleAttributeChanged);
	BoundASC->GetGameplayAttributeValueChangeDelegate(UOnsetAttributeSet::GetMaxHealthAttribute()).AddUObject(this, &UPlayerHealthBarWidget::HandleAttributeChanged);

	RefreshHealth();
}

void UPlayerHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshHealth();
}

void UPlayerHealthBarWidget::NativeDestruct()
{
	if (BoundASC)
	{
		BoundASC->GetGameplayAttributeValueChangeDelegate(UOnsetAttributeSet::GetHealthAttribute()).RemoveAll(this);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UOnsetAttributeSet::GetMaxHealthAttribute()).RemoveAll(this);
		BoundASC = nullptr;
	}

	Super::NativeDestruct();
}

void UPlayerHealthBarWidget::HandleAttributeChanged(const FOnAttributeChangeData& Data)
{
	RefreshHealth();
}

void UPlayerHealthBarWidget::RefreshHealth()
{
	if (!BoundASC)
	{
		return;
	}

	const float MaxHealth = BoundASC->GetNumericAttribute(UOnsetAttributeSet::GetMaxHealthAttribute());
	const float Health = BoundASC->GetNumericAttribute(UOnsetAttributeSet::GetHealthAttribute());
	const float NewPercent = MaxHealth > 0.0f ? FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f) : 0.0f;

	if (HealthPercent != NewPercent)
	{
		HealthPercent = NewPercent;
		OnHealthPercentChanged(HealthPercent);
	}

	if (HealthText)
	{
		HealthText->SetText(FText::Format(FText::FromString(TEXT("{0} / {1}")), FText::AsNumber(FMath::RoundToInt(Health)), FText::AsNumber(FMath::RoundToInt(MaxHealth))));
	}
}
