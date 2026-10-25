// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "OnsetBuffBarWidget.generated.h"

class UAbilitySystemComponent;
class UHorizontalBox;
class UImage;
class UCommonTextBlock;

/** Individual buff or debuff badge icon. */
UCLASS(Abstract, Blueprintable)
class ONSET_API UOnsetBuffIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Sets the icon appearance for a given gameplay tag (Stun, Freeze, Burn, Slow, Haste). */
	UFUNCTION(BlueprintCallable, Category = "Onset|Buffs")
	void SetupForTag(FGameplayTag Tag, bool bIsDebuff, float Duration = 0.0f);

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonTextBlock> DurationText;

	UPROPERTY(BlueprintReadOnly, Category = "Onset|Buffs")
	FGameplayTag TrackedTag;

	UPROPERTY(BlueprintReadOnly, Category = "Onset|Buffs")
	bool bIsNegative = false;
};

/**
 * Tactical HUD overlay displaying active buffs and debuffs on the player.
 * Subscribes to the player's AbilitySystemComponent.
 */
UCLASS(Abstract, Blueprintable)
class ONSET_API UOnsetBuffBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Binds this bar to the player's AbilitySystemComponent. */
	UFUNCTION(BlueprintCallable, Category = "Onset|Buffs")
	void BindToASC(UAbilitySystemComponent* InASC);

protected:
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> BuffContainer;

	UPROPERTY(EditDefaultsOnly, Category = "Onset|Buffs")
	TSubclassOf<UOnsetBuffIconWidget> BuffIconWidgetClass;

private:
	void RefreshActiveTags();

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	TMap<FGameplayTag, TObjectPtr<UOnsetBuffIconWidget>> ActiveIconWidgets;
};
