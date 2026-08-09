// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/CombatToggleWidget.h"

#include "Components/Button.h"
#include "Player/OnsetPlayerController.h"
#include "Player/OnsetPlayerState.h"

void UCombatToggleWidget::BindToPlayer(AOnsetPlayerController* InController)
{
	if (!InController)
	{
		return;
	}

	BoundController = InController;

	if (AOnsetPlayerState* PS = BoundController->GetPlayerState<AOnsetPlayerState>())
	{
		PS->OnPlayerSettingsChanged.AddDynamic(this, &UCombatToggleWidget::HandlePlayerSettingsChanged);
	}

	if (AutoplayToggleButton)
	{
		AutoplayToggleButton->OnClicked.AddDynamic(this, &UCombatToggleWidget::OnAutoplayToggled);
	}

	if (ContinueOnDisconnectToggleButton)
	{
		ContinueOnDisconnectToggleButton->OnClicked.AddDynamic(this, &UCombatToggleWidget::OnContinueOnDisconnectToggled);
	}

	RefreshToggleStates();
}

void UCombatToggleWidget::NativeDestruct()
{
	if (BoundController)
	{
		if (AOnsetPlayerState* PS = BoundController->GetPlayerState<AOnsetPlayerState>())
		{
			PS->OnPlayerSettingsChanged.RemoveDynamic(this, &UCombatToggleWidget::HandlePlayerSettingsChanged);
		}
		BoundController = nullptr;
	}

	Super::NativeDestruct();
}

void UCombatToggleWidget::HandlePlayerSettingsChanged()
{
	RefreshToggleStates();
}

void UCombatToggleWidget::RefreshToggleStates()
{
	if (!BoundController)
	{
		return;
	}

	AOnsetPlayerState* PS = BoundController->GetPlayerState<AOnsetPlayerState>();
	const bool bNewAutoplay = PS && PS->bAutoplayEnabled;
	const bool bNewContinue = !PS || PS->bContinueOnDisconnect;

	if (bNewAutoplay != bAutoplayEnabled)
	{
		bAutoplayEnabled = bNewAutoplay;
		OnAutoplayStateChanged(bAutoplayEnabled);
	}

	if (bNewContinue != bContinueOnDisconnect)
	{
		bContinueOnDisconnect = bNewContinue;
		OnContinueOnDisconnectStateChanged(bContinueOnDisconnect);
	}
}

void UCombatToggleWidget::OnAutoplayToggled()
{
	if (BoundController)
	{
		BoundController->SetAutoCombatEnabled(!bAutoplayEnabled);
	}
}

void UCombatToggleWidget::OnContinueOnDisconnectToggled()
{
	if (BoundController)
	{
		BoundController->SetContinueOnDisconnect(!bContinueOnDisconnect);
	}
}
