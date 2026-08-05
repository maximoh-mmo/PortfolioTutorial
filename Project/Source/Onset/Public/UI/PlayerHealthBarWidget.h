// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHealthBarWidget.generated.h"

class UAbilitySystemComponent;
class UProgressBar;
class UTextBlock;
struct FOnAttributeChangeData;

/**
 * Player health bar. Binds to the owning character's ASC Health/MaxHealth
 * attribute change delegates and updates a progress bar + text.
 */
UCLASS()
class ONSET_API UPlayerHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Binds to the ASC attribute change delegates. Safe to call once. */
	void BindToASC(UAbilitySystemComponent* InASC);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Reads current Health/MaxHealth and refreshes visuals. */
	void RefreshHealth();

	/** Delegate handler for ASC attribute changes. */
	void HandleAttributeChanged(const FOnAttributeChangeData& Data);

private:
	UPROPERTY()
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY()
	TObjectPtr<UTextBlock> HealthText;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> BoundASC;
};
