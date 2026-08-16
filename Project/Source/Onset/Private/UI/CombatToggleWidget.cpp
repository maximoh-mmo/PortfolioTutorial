// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/CombatToggleWidget.h"

#include "CommonButtonBase.h"
#include "Player/OnsetPlayerController.h"
#include "Player/OnsetPlayerState.h"

void UCombatToggleWidget::BindToPlayer(AOnsetPlayerController* InController)
{
	if (!InController)
	{
		return;
	}

	BoundController = InController;

	AOnsetPlayerState* PS = BoundController->GetPlayerState<AOnsetPlayerState>();
	if (PS)
	{
		SubscribeToPlayerState(PS);
	}
	else if (UWorld* World = GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("CombatToggle BindToPlayer: PS not replicated yet, scheduling retry"));
		World->GetTimerManager().SetTimer(RetryBindTimerHandle, this, &UCombatToggleWidget::RetryBindToPlayer, 0.5f, true);
	}

	UE_LOG(LogTemp, Warning, TEXT("CombatToggle BindToPlayer: AutoplayBtn=%s ContinueBtn=%s PS=%s"),
		AutoplayToggleButton ? TEXT("bound") : TEXT("NULL"),
		ContinueOnDisconnectToggleButton ? TEXT("bound") : TEXT("NULL"),
		PS ? *PS->GetName() : TEXT("NULL"));

	if (AutoplayToggleButton)
	{
		AutoplayToggleButton->OnClicked().AddUObject(this, &UCombatToggleWidget::OnAutoplayToggled);
	}

	if (ContinueOnDisconnectToggleButton)
	{
		ContinueOnDisconnectToggleButton->OnClicked().AddUObject(this, &UCombatToggleWidget::OnContinueOnDisconnectToggled);
	}

	RefreshToggleStates();
}

void UCombatToggleWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RetryBindTimerHandle);
	}

	if (BoundPlayerState)
	{
		BoundPlayerState->OnPlayerSettingsChanged.RemoveDynamic(this, &UCombatToggleWidget::HandlePlayerSettingsChanged);
		BoundPlayerState = nullptr;
	}

	BoundController = nullptr;

	Super::NativeDestruct();
}

void UCombatToggleWidget::RetryBindToPlayer()
{
	if (!BoundController || BoundPlayerState)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(RetryBindTimerHandle);
		}
		return;
	}

	if (AOnsetPlayerState* PS = BoundController->GetPlayerState<AOnsetPlayerState>())
	{
		UE_LOG(LogTemp, Warning, TEXT("CombatToggle RetryBindToPlayer: PS now available (%s)"), *PS->GetName());
		SubscribeToPlayerState(PS);
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(RetryBindTimerHandle);
		}
		RefreshToggleStates();
	}
}

void UCombatToggleWidget::SubscribeToPlayerState(AOnsetPlayerState* PS)
{
	if (!PS || BoundPlayerState == PS)
	{
		return;
	}

	BoundPlayerState = PS;
	PS->OnPlayerSettingsChanged.AddDynamic(this, &UCombatToggleWidget::HandlePlayerSettingsChanged);
	UE_LOG(LogTemp, Warning, TEXT("CombatToggle SubscribeToPlayerState: subscribed to %s"), *PS->GetName());
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

	AOnsetPlayerState* PS = BoundPlayerState;
	const bool bNewAutoplay = PS && PS->bAutoplayEnabled;
	const bool bNewContinue = !PS || PS->bContinueOnDisconnect;

	if (bNewAutoplay != bAutoplayEnabled)
	{
		bAutoplayEnabled = bNewAutoplay;
		UE_LOG(LogTemp, Warning, TEXT("CombatToggle: autoplay UI -> %d"), bAutoplayEnabled);
		OnAutoplayStateChanged(bAutoplayEnabled);
	}

	// The replicated state has arrived, so any in-flight request is resolved.
	bAutoplayRequestPending = false;

	if (AutoplayToggleButton)
	{
		UE_LOG(LogTemp, Warning, TEXT("CombatToggle: autoplay UI -> %d"), bAutoplayEnabled);
		AutoplayToggleButton->SetIsSelected(bAutoplayEnabled, false);
	}

	if (bNewContinue != bContinueOnDisconnect)
	{
		bContinueOnDisconnect = bNewContinue;
		OnContinueOnDisconnectStateChanged(bContinueOnDisconnect);
	}

	bContinueOnDisconnectRequestPending = false;

	if (ContinueOnDisconnectToggleButton)
	{
		ContinueOnDisconnectToggleButton->SetIsSelected(bContinueOnDisconnect, false);
	}
}

void UCombatToggleWidget::OnAutoplayToggled()
{
	if (!BoundController || bAutoplayRequestPending)
	{
		return;
	}

	// CommonButtonBase flips its own selected state locally on click. Revert it so
	// the toggle only shows the new state once the server's replicated state confirms it.
	bAutoplayRequestPending = true;
	if (AutoplayToggleButton)
	{
		UE_LOG(LogTemp, Warning, TEXT("Setting AutoPlayToggle button to: %d"), bAutoplayEnabled);
		AutoplayToggleButton->SetIsSelected(bAutoplayEnabled, false);
	}

	const bool bDesired = !bAutoplayEnabled;
	UE_LOG(LogTemp, Warning, TEXT("CombatToggle OnAutoplayToggled: sending %d (pending confirmation)"),
		bDesired ? 1 : 0);
	BoundController->SetAutoCombatEnabled(bDesired);

	if (BoundController->HasAuthority())
	{
		// On the server the change applies synchronously; re-read immediately so the
		// pending gate clears and the UI reflects the confirmed state right away.
		bAutoplayRequestPending = false;
		RefreshToggleStates();
	}
}

void UCombatToggleWidget::OnContinueOnDisconnectToggled()
{
	if (!BoundController || bContinueOnDisconnectRequestPending)
	{
		return;
	}

	bContinueOnDisconnectRequestPending = true;
	if (ContinueOnDisconnectToggleButton)
	{
		ContinueOnDisconnectToggleButton->SetIsSelected(bContinueOnDisconnect, false);
	}

	BoundController->SetContinueOnDisconnect(!bContinueOnDisconnect);

	if (BoundController->HasAuthority())
	{
		bContinueOnDisconnectRequestPending = false;
		RefreshToggleStates();
	}
}
