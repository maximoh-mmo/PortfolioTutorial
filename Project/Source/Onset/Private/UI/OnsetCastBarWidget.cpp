// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/OnsetCastBarWidget.h"

#include "CommonTextBlock.h"
#include "Components/ProgressBar.h"

void UOnsetCastBarWidget::StartCast(const FText& AbilityName, float Duration, bool bIsChannel)
{
	bIsCasting = true;
	bIsChanneling = bIsChannel;
	TotalDuration = FMath::Max(0.05f, Duration);
	ElapsedTime = 0.0f;

	if (AbilityNameText)
	{
		AbilityNameText->SetText(AbilityName);
	}

	if (CastProgressBar)
	{
		CastProgressBar->SetPercent(bIsChanneling ? 1.0f : 0.0f);
	}

	SetVisibility(ESlateVisibility::Visible);
}

void UOnsetCastBarWidget::InterruptCast()
{
	bIsCasting = false;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UOnsetCastBarWidget::FinishCast()
{
	bIsCasting = false;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UOnsetCastBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bIsCasting)
	{
		return;
	}

	ElapsedTime += InDeltaTime;
	const float Alpha = FMath::Clamp(ElapsedTime / TotalDuration, 0.0f, 1.0f);

	if (CastProgressBar)
	{
		CastProgressBar->SetPercent(bIsChanneling ? (1.0f - Alpha) : Alpha);
	}

	if (ElapsedTime >= TotalDuration)
	{
		FinishCast();
	}
}
