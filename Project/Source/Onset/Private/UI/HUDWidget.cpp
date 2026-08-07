// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HUDWidget.h"

#include "AbilitySystemComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Core/OnsetBaseCharacter.h"
#include "Core/TargetingComponent.h"
#include "Engine/GameViewportClient.h"
#include "GAS/OnsetAttributeSet.h"
#include "Player/OnsetPlayerController.h"
#include "UI/AbilityBarWidget.h"
#include "UI/DamageNumberWidget.h"
#include "UI/PlayerHealthBarWidget.h"
#include "UI/TargetHUDWidget.h"

void UHUDWidget::BindToPlayer(AOnsetPlayerController* InController, AOnsetBaseCharacter* InPawn)
{
	if (!InController || !InPawn)
	{
		return;
	}

	BoundController = InController;
	BoundPawn = InPawn;
	BoundTargeting = InPawn->TargetingComponent;
	PlayerASC = InPawn->AbilitySystemComponent;

	if (PlayerHealthBar)
	{
		PlayerHealthBar->BindToASC(PlayerASC);
	}

	if (AbilityBar)
	{
		AbilityBar->BindToPlayer(InController, PlayerASC);
	}

	if (BoundTargeting)
	{
		BoundTargeting->OnTargetChanged.AddDynamic(this, &UHUDWidget::HandleTargetChanged);
		HandleTargetChanged(BoundTargeting->GetTarget());
	}

	if (PlayerASC)
	{
		PlayerASC->GetGameplayAttributeValueChangeDelegate(UOnsetAttributeSet::GetHealthAttribute())
			.AddUObject(this, &UHUDWidget::HandlePlayerHealthChanged);
	}
}

void UHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// The visual tree is owned by the Widget Blueprint (WBP_HUD). This only
	// populates the designer-provided damage-number layer.
	BuildDamageNumberPool();
}

void UHUDWidget::NativeDestruct()
{
	if (BoundTargeting)
	{
		BoundTargeting->OnTargetChanged.RemoveDynamic(this, &UHUDWidget::HandleTargetChanged);
		BoundTargeting = nullptr;
	}

	if (PlayerASC)
	{
		PlayerASC->GetGameplayAttributeValueChangeDelegate(UOnsetAttributeSet::GetHealthAttribute()).RemoveAll(this);
		PlayerASC = nullptr;
	}

	if (TargetASC)
	{
		TargetASC->GetGameplayAttributeValueChangeDelegate(UOnsetAttributeSet::GetHealthAttribute()).RemoveAll(this);
		TargetASC = nullptr;
	}

	TrackedTarget = nullptr;

	Super::NativeDestruct();
}

void UHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}

void UHUDWidget::HandleTargetChanged(AActor* NewTarget)
{
	// Unbind previous target ASC.
	if (TargetASC)
	{
		TargetASC->GetGameplayAttributeValueChangeDelegate(UOnsetAttributeSet::GetHealthAttribute()).RemoveAll(this);
		TargetASC = nullptr;
	}

	TrackedTarget = Cast<AOnsetBaseCharacter>(NewTarget);

	if (TargetHUD)
	{
		TargetHUD->SetTarget(TrackedTarget);
	}

	if (TrackedTarget)
	{
		TargetASC = TrackedTarget->AbilitySystemComponent;
		if (TargetASC)
		{
			TargetASC->GetGameplayAttributeValueChangeDelegate(UOnsetAttributeSet::GetHealthAttribute())
				.AddUObject(this, &UHUDWidget::HandleTargetHealthChanged);
		}
	}
}

void UHUDWidget::HandleTargetHealthChanged(const FOnAttributeChangeData& Data)
{
	const float Damage = Data.OldValue - Data.NewValue;
	if (Damage <= 0.0f || !TrackedTarget)
	{
		return;
	}

	// Damage dealt by the player to the tracked target.
	SpawnDamageNumber(TrackedTarget->GetActorLocation() + FVector(0.0f, 0.0f, 120.0f), Damage, PlayerDamageColor);
}

void UHUDWidget::HandlePlayerHealthChanged(const FOnAttributeChangeData& Data)
{
	const float Damage = Data.OldValue - Data.NewValue;
	if (Damage <= 0.0f || !BoundPawn)
	{
		return;
	}

	// Damage taken by the player (dealt by enemies).
	SpawnDamageNumber(BoundPawn->GetActorLocation() + FVector(0.0f, 0.0f, 120.0f), Damage, EnemyDamageColor);
}

void UHUDWidget::BuildDamageNumberPool()
{
	if (!DamageNumberLayer || DamageNumberPool.Num() > 0)
	{
		return;
	}

	APlayerController* OwningPC = GetOwningPlayer();

	for (int32 i = 0; i < MaxDamageNumbers; ++i)
	{
		UDamageNumberWidget* DamageNumber = nullptr;
		if (DamageNumberWidgetClass)
		{
			DamageNumber = CreateWidget<UDamageNumberWidget>(OwningPC, DamageNumberWidgetClass);
		}
		if (!DamageNumber)
		{
			continue;
		}

		if (UCanvasPanelSlot* DamageNumberSlot = DamageNumberLayer->AddChildToCanvas(DamageNumber))
		{
			DamageNumberSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			DamageNumberSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			DamageNumberSlot->SetAutoSize(true);
		}

		DamageNumber->SetVisibility(ESlateVisibility::Collapsed);
		DamageNumberPool.Add(DamageNumber);
	}

	NextDamageNumberIndex = 0;
}

void UHUDWidget::SpawnDamageNumber(const FVector& WorldLocation, float Amount, const FLinearColor& Color)
{
	if (DamageNumberPool.IsEmpty() || Amount <= 0.0f)
	{
		return;
	}

	FVector2D ScreenPos;
	if (!ProjectToScreen(WorldLocation, ScreenPos))
	{
		return;
	}

	UDamageNumberWidget* DamageNumber = DamageNumberPool[NextDamageNumberIndex];
	if (DamageNumber && DamageNumber->IsActive())
	{
		// Pool exhausted — recycle the oldest (round-robin) entry to show the newest hit.
		DamageNumber->Deactivate();
	}

	if (DamageNumber)
	{
		DamageNumber->ShowDamage(Amount, ScreenPos, Color);
	}

	NextDamageNumberIndex = (NextDamageNumberIndex + 1) % DamageNumberPool.Num();
}

bool UHUDWidget::ProjectToScreen(const FVector& WorldLocation, FVector2D& OutScreenPos) const
{
	if (const APlayerController* PC = GetOwningPlayer())
	{
		// ProjectWorldLocationToScreen returns viewport pixels; the UMG layer is
		// scaled by the HUD widget's geometry scale, so convert pixels to layout
		// units before the caller positions widgets on the canvas.
		FVector2D ViewportPixels;
		const bool bProjected = PC->ProjectWorldLocationToScreen(WorldLocation, ViewportPixels, false);
		if (bProjected)
		{
			const float GeometryScale = GetCachedGeometry().Scale > 0.0f ? GetCachedGeometry().Scale : 1.0f;
			OutScreenPos = ViewportPixels / GeometryScale;
		}

		static bool bLogged = false;
		if (!bLogged)
		{
			bLogged = true;
			UGameViewportClient* GVC = PC->GetWorld() ? PC->GetWorld()->GetGameViewport() : nullptr;
			FVector2D ViewportSize(0.0f);
			if (GVC && GVC->Viewport)
			{
				const FIntPoint Sz = GVC->Viewport->GetSizeXY();
				ViewportSize = FVector2D(Sz.X, Sz.Y);
			}
			const float DPIScale = GVC ? GVC->GetDPIScale() : 1.0f;
			const float GeometryScale = GetCachedGeometry().Scale;
			UE_LOG(LogTemp, Warning, TEXT("[HUDDiag] ProjectToScreen first call: World=%s Pixels=%s Layout=%s ViewportSize=%s DPIScale=%.3f WidgetGeomScale=%.3f"),
				*WorldLocation.ToString(), bProjected ? *ViewportPixels.ToString() : TEXT("FAILED"),
				bProjected ? *OutScreenPos.ToString() : TEXT("FAILED"),
				*ViewportSize.ToString(), DPIScale, GeometryScale);
		}
		return bProjected;
	}
	return false;
}
