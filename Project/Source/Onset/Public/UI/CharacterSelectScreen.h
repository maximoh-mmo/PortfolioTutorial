// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnsetPlayerDataTypes.h"
#include "OnsetScreenBase.h"
#include "CharacterSelectScreen.generated.h"

class AOnsetPlayerController;
class UCharacterCreationScreen;

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
};
