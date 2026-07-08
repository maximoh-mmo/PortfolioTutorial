// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnsetPlayerDataTypes.h"
#include "OnsetScreenBase.h"
#include "CharacterSelectScreen.generated.h"

class AOnsetPlayerController;
/**
 * 
 */
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
	
protected:                                  
	TObjectPtr<AOnsetPlayerController> CachedPlayerController;
	UFUNCTION(BlueprintImplementableEvent)                                                                      
	void BP_OnAccountDataReady(const FOnsetAccountData& AccountData);
	UPROPERTY(BlueprintReadOnly)
	FOnsetAccountData CachedAccountData;                                                                             
	int32 SelectedSlot = -1;
};
