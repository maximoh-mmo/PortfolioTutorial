// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/PlayerXPBarWidget.h"

#include "CommonTextBlock.h"
#include "Player/OnsetPlayerCharacter.h"

void UPlayerXPBarWidget::BindToPlayerCharacter(AOnsetPlayerCharacter* InPawn)
{
	if (!InPawn)
	{
		return;
	}

	if (BoundPawn)
	{
		BoundPawn->OnProgressionChanged.RemoveDynamic(this, &UPlayerXPBarWidget::HandleProgressionChanged);
	}

	BoundPawn = InPawn;
	BoundPawn->OnProgressionChanged.AddDynamic(this, &UPlayerXPBarWidget::HandleProgressionChanged);

	RefreshProgression();
}

void UPlayerXPBarWidget::NativeDestruct()
{
	if (BoundPawn)
	{
		BoundPawn->OnProgressionChanged.RemoveDynamic(this, &UPlayerXPBarWidget::HandleProgressionChanged);
		BoundPawn = nullptr;
	}

	Super::NativeDestruct();
}

void UPlayerXPBarWidget::HandleProgressionChanged(int32 NewLevel, int32 NewExperience)
{
	RefreshProgression();
}

void UPlayerXPBarWidget::RefreshProgression()
{
	if (!BoundPawn)
	{
		return;
	}

	CurrentLevel = BoundPawn->Level;
	StatPointsAvailable = BoundPawn->UnspentStatPoints;

	XPToNextLevel = BoundPawn->GetXPRequiredForNextLevel();
	const float NewPercent = BoundPawn->GetXPProgressPercent();

	if (XPPercent != NewPercent)
	{
		XPPercent = NewPercent;
		OnXPPercentChanged(XPPercent);
	}

	if (LevelText)
	{
		LevelText->SetText(FText::Format(FText::FromString(TEXT("Lv {0}")), FText::AsNumber(CurrentLevel)));
	}

	if (XPText)
	{
		XPText->SetText(FText::Format(
			FText::FromString(TEXT("{0} / {1}")),
			FText::AsNumber(BoundPawn->Experience),
			FText::AsNumber(XPToNextLevel)));
	}
}