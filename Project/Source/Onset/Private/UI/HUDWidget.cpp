// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/HUDWidget.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Core/OnsetBaseCharacter.h"
#include "Core/TargetingComponent.h"
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

	if (!WidgetTree)
	{
		return;
	}

	RootPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootPanel"));
	if (!RootPanel)
	{
		return;
	}
	WidgetTree->RootWidget = RootPanel;

	// Damage number layer - full viewport, hit-test transparent.
	DamageNumberLayer = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("DamageNumberLayer"));
	if (DamageNumberLayer)
	{
		UCanvasPanelSlot* LayerSlot = RootPanel->AddChildToCanvas(DamageNumberLayer);
		if (LayerSlot)
		{
			LayerSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		}
		DamageNumberLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	BuildDamageNumberPool();

	// Sub-widgets are stretched to the full viewport so their internal
	// canvas anchors resolve against the screen, not against a collapsed child.
	APlayerController* OwningPC = GetOwningPlayer();

	if (PlayerHealthBar = CreateWidget<UPlayerHealthBarWidget>(OwningPC, UPlayerHealthBarWidget::StaticClass()))
	{
		if (UCanvasPanelSlot* HealthBarSlot = RootPanel->AddChildToCanvas(PlayerHealthBar))
		{
			HealthBarSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		}
	}

	if (AbilityBar = CreateWidget<UAbilityBarWidget>(OwningPC, UAbilityBarWidget::StaticClass()))
	{
		if (UCanvasPanelSlot* AbilityBarSlot = RootPanel->AddChildToCanvas(AbilityBar))
		{
			AbilityBarSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		}
	}

	if (TargetHUD = CreateWidget<UTargetHUDWidget>(OwningPC, UTargetHUDWidget::StaticClass()))
	{
		if (UCanvasPanelSlot* TargetHUDSlot = RootPanel->AddChildToCanvas(TargetHUD))
		{
			TargetHUDSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		}
	}
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

	SpawnDamageNumber(TrackedTarget->GetActorLocation() + FVector(0.0f, 0.0f, 120.0f), Damage);
}

void UHUDWidget::HandlePlayerHealthChanged(const FOnAttributeChangeData& Data)
{
	const float Damage = Data.OldValue - Data.NewValue;
	if (Damage <= 0.0f || !BoundPawn)
	{
		return;
	}

	SpawnDamageNumber(BoundPawn->GetActorLocation() + FVector(0.0f, 0.0f, 120.0f), Damage);
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
		UDamageNumberWidget* DamageNumber = CreateWidget<UDamageNumberWidget>(OwningPC, UDamageNumberWidget::StaticClass());
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

void UHUDWidget::SpawnDamageNumber(const FVector& WorldLocation, float Amount)
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
		DamageNumber->ShowDamage(Amount, ScreenPos);
	}

	NextDamageNumberIndex = (NextDamageNumberIndex + 1) % DamageNumberPool.Num();
}

bool UHUDWidget::ProjectToScreen(const FVector& WorldLocation, FVector2D& OutScreenPos) const
{
	if (const APlayerController* PC = GetOwningPlayer())
	{
		return PC->ProjectWorldLocationToScreen(WorldLocation, OutScreenPos, false);
	}
	return false;
}
