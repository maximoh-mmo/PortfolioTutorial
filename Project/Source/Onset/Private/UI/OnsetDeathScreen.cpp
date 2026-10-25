// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/OnsetDeathScreen.h"

#include "CommonTextBlock.h"
#include "Player/OnsetPlayerController.h"
#include "UI/OnsetButtonBase.h"

UOnsetDeathScreen::UOnsetDeathScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bUseMenuInputMode = true;
}

void UOnsetDeathScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (RespawnButton)
	{
		RespawnButton->OnClicked().AddUObject(this, &UOnsetDeathScreen::HandleRespawnClicked);
		RespawnButton->SetIsEnabled(false);
	}
}

void UOnsetDeathScreen::NativeDestruct()
{
	if (RespawnButton)
	{
		RespawnButton->OnClicked().RemoveAll(this);
	}

	Super::NativeDestruct();
}

bool UOnsetDeathScreen::NativeOnHandleBackAction()
{
	// Death screen cannot be dismissed via Esc/Back button until respawn
	return true;
}

void UOnsetDeathScreen::SetDeathReport(const FOnsetDeathReport& InReport)
{
	CurrentReport = InReport;
	RemainingSeconds = FMath::Max(0.0f, InReport.RespawnDelay);
	bRespawnReady = (RemainingSeconds <= 0.0f);

	if (KillerNameText)
	{
		const FText FormattedKiller = FText::Format(
			NSLOCTEXT("OnsetDeath", "EliminatedBy", "Eliminated by: {0}"),
			InReport.KillerName.IsEmpty() ? NSLOCTEXT("OnsetDeath", "UnknownHazard", "Combat Trauma") : InReport.KillerName
		);
		KillerNameText->SetText(FormattedKiller);
	}

	if (PenaltyText)
	{
		const FText FormattedPenalty = FText::Format(
			NSLOCTEXT("OnsetDeath", "PenaltyDesc", "Equipment Durability Penalty: -{0}%"),
			FText::AsNumber(FMath::RoundToInt(InReport.DurabilityLossPercent))
		);
		PenaltyText->SetText(FormattedPenalty);
	}

	if (RespawnLocationText)
	{
		const FText FormattedLoc = FText::Format(
			NSLOCTEXT("OnsetDeath", "RespawnLoc", "Respawn Point: {0}"),
			InReport.RespawnLocationName.IsEmpty() ? NSLOCTEXT("OnsetDeath", "DefaultOutpost", "Home Outpost") : InReport.RespawnLocationName
		);
		RespawnLocationText->SetText(FormattedLoc);
	}

	if (RespawnButton)
	{
		RespawnButton->SetIsEnabled(bRespawnReady);
	}

	BP_OnDeathReportApplied(InReport);

	if (bRespawnReady)
	{
		BP_OnRespawnReady();
	}
}

void UOnsetDeathScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bRespawnReady)
	{
		RemainingSeconds -= InDeltaTime;

		if (CountdownText)
		{
			const int32 SecondsLeft = FMath::CeilToInt(FMath::Max(0.0f, RemainingSeconds));
			CountdownText->SetText(FText::Format(
				NSLOCTEXT("OnsetDeath", "CountdownFmt", "Respawn available in {0}s..."),
				FText::AsNumber(SecondsLeft)
			));
		}

		if (RemainingSeconds <= 0.0f)
		{
			bRespawnReady = true;
			if (CountdownText)
			{
				CountdownText->SetText(NSLOCTEXT("OnsetDeath", "ReadyToRespawn", "Ready to Respawn"));
			}

			if (RespawnButton)
			{
				RespawnButton->SetIsEnabled(true);
			}

			BP_OnRespawnReady();
		}
	}
}

void UOnsetDeathScreen::HandleRespawnClicked()
{
	if (!bRespawnReady)
	{
		return;
	}

	if (AOnsetPlayerController* PC = Cast<AOnsetPlayerController>(GetOwningPlayer()))
	{
		PC->Server_RequestRespawn();
	}
}
