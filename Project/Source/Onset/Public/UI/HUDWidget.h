// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDWidget.generated.h"

class AOnsetBaseCharacter;
class AOnsetPlayerController;
class UAbilityBarWidget;
class UAbilitySystemComponent;
class UCanvasPanel;
class UDamageNumberWidget;
class UPlayerHealthBarWidget;
class UTargetHUDWidget;
class UTargetingComponent;
struct FOnAttributeChangeData;

/**
 * In-game HUD container. Holds the player health bar, ability bar, target HUD
 * element, and the damage-number layer. Wired to the owning pawn's ASC and
 * targeting component, so health/target changes flow straight into the widgets.
 */
UCLASS()
class ONSET_API UHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Binds the HUD to a possessed pawn + its controller. Safe to call once. */
	void BindToPlayer(AOnsetPlayerController* InController, AOnsetBaseCharacter* InPawn);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Reacts to TargetingComponent target changes, updating the target HUD. */
	void HandleTargetChanged(AActor* NewTarget);

	/** Spawns a damage number at the screen position of a world location. */
	void SpawnDamageNumber(const FVector& WorldLocation, float Amount);

	/** Pre-allocates the damage number pool into the layer. */
	void BuildDamageNumberPool();

private:
	/** Handles Health attribute deltas on the bound target (damage dealt). */
	void HandleTargetHealthChanged(const FOnAttributeChangeData& Data);

	/** Handles Health attribute deltas on the player pawn (damage taken). */
	void HandlePlayerHealthChanged(const FOnAttributeChangeData& Data);

	/** Resolves a world location to viewport coordinates. */
	bool ProjectToScreen(const FVector& WorldLocation, FVector2D& OutScreenPos) const;

	UPROPERTY()
	TObjectPtr<UCanvasPanel> RootPanel;

	UPROPERTY()
	TObjectPtr<UCanvasPanel> DamageNumberLayer;

	UPROPERTY()
	TObjectPtr<UPlayerHealthBarWidget> PlayerHealthBar;

	UPROPERTY()
	TObjectPtr<UAbilityBarWidget> AbilityBar;

	UPROPERTY()
	TObjectPtr<UTargetHUDWidget> TargetHUD;

	UPROPERTY()
	TObjectPtr<AOnsetPlayerController> BoundController;

	UPROPERTY()
	TObjectPtr<AOnsetBaseCharacter> BoundPawn;

	UPROPERTY()
	TObjectPtr<UTargetingComponent> BoundTargeting;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> PlayerASC;

	UPROPERTY()
	TObjectPtr<AOnsetBaseCharacter> TrackedTarget;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> TargetASC;

	/** Hard cap on simultaneously visible damage numbers. */
	static constexpr int32 MaxDamageNumbers = 64;

	/** Pool of pre-allocated damage number widgets; re-used round-robin. */
	UPROPERTY()
	TArray<TObjectPtr<UDamageNumberWidget>> DamageNumberPool;

	/** Index of the next pool entry to reuse. */
	int32 NextDamageNumberIndex = 0;
};
