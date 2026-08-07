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
 * In-game HUD container. The visual layout lives in a Widget Blueprint
 * (WBP_HUD) which nests the health bar, ability bar, target HUD element, and a
 * damage-number layer. All logic lives here in C++: wiring to the owning pawn's
 * ASC + targeting component, and the pooled damage-number system.
 */
UCLASS(Blueprintable)
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
	UFUNCTION()
	void HandleTargetChanged(AActor* NewTarget);

	/** Spawns a damage number at the screen position of a world location. */
	void SpawnDamageNumber(const FVector& WorldLocation, float Amount, const FLinearColor& Color);

	/** Pre-allocates the damage number pool into the layer. */
	void BuildDamageNumberPool();

private:
	/** Handles Health attribute deltas on the bound target (damage dealt). */
	void HandleTargetHealthChanged(const FOnAttributeChangeData& Data);

	/** Handles Health attribute deltas on the player pawn (damage taken). */
	void HandlePlayerHealthChanged(const FOnAttributeChangeData& Data);

	/** Resolves a world location to viewport coordinates. */
	bool ProjectToScreen(const FVector& WorldLocation, FVector2D& OutScreenPos) const;

	/** Designer-nested sub-widgets (styled in WBP_HUD). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPlayerHealthBarWidget> PlayerHealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UAbilityBarWidget> AbilityBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTargetHUDWidget> TargetHUD;

	/** Designer canvas that receives the pooled damage numbers. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> DamageNumberLayer;

	/** Color for damage the player deals to a target. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	FLinearColor PlayerDamageColor = FLinearColor(1.0f, 0.75f, 0.2f, 1.0f);

	/** Color for damage the player takes from enemies. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	FLinearColor EnemyDamageColor = FLinearColor(1.0f, 0.25f, 0.25f, 1.0f);

	/** Damage number widget class to instantiate into the pool (override with WBP_DamageNumber). */
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UDamageNumberWidget> DamageNumberWidgetClass;

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
