// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnsetPlayerDataTypes.h"
#include "OnsetScreenBase.h"
#include "CharacterSelectScreen.generated.h"

class AOnsetPlayerController;
class UCharacterCreationScreen;
class UCharacterSlot;
class UPanelWidget;

UCLASS(Abstract)
class ONSET_API UCharacterSelectScreen : public UOnsetScreenBase
{
	GENERATED_BODY()
public:
	void SetAccountData(const FOnsetAccountData& InAccountData);
	void SetPlayerController(AOnsetPlayerController* PlayerController);
	UFUNCTION(BlueprintCallable)
	void SelectSlot(int32 SlotIndex);
	UFUNCTION(BlueprintCallable)
	void EnterWorld() const;
	UFUNCTION(BlueprintCallable)
	void DeleteCharacter(int32 SlotIndex);
	UFUNCTION(BlueprintCallable)
	void OpenCreateCharacter(int32 SlotIndex);
	UFUNCTION(BlueprintCallable)
	void RefreshAccountData();
	UFUNCTION(BlueprintCallable)
	void CreateCharacter(int32 SlotIndex, const FString& CharacterName, EOnsetCharacterClass CharacterClass, int32 AppearancePresetIndex);

protected:
	TObjectPtr<AOnsetPlayerController> CachedPlayerController;
	UFUNCTION(BlueprintImplementableEvent)
	void BP_OnAccountDataReady(const FOnsetAccountData& AccountData);
	UPROPERTY(BlueprintReadOnly)
	FOnsetAccountData CachedAccountData;
	int32 SelectedSlot = -1;

	UPROPERTY(EditDefaultsOnly, Category = "Character Selection")
	TSubclassOf<UCharacterCreationScreen> CharacterCreationScreenClass;

	/** Blueprint widget class used to build each character slot. */
	UPROPERTY(EditDefaultsOnly, Category = "Character Selection")
	TSubclassOf<UCharacterSlot> CharacterSlotClass;

	/** Container into which the slot widgets are created. Must be a panel widget named CharacterSlotContainer. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> CharacterSlotContainer;

	//~ Begin UOnsetScreenBase interface
	virtual void NativeOnActivated() override;
	//~ End UOnsetScreenBase interface

	virtual void NativeDestruct() override;

private:
	void BuildSlotWidgets();
	void RefreshSlotWidgets();
	TArray<TObjectPtr<UCharacterSlot>> SlotWidgets;

	UFUNCTION()
	void HandleAccountDataChanged();

	UFUNCTION()
	void HandleSlotActivated(int32 InSlotIndex);

	UFUNCTION()
	void HandleDeleteRequested(int32 InSlotIndex);
};
