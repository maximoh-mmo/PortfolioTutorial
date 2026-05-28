// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OnsetStatics.generated.h"

class AOnsetPlayerCharacter;
class UTargetingComponent;
class AOnsetPlayerState;
class AOnsetPlayerController;

/**
 * Function Library for commonly needed Cast<class> operations that are not easily done in Blueprints.
 * For example, casting from AActor to a specific component class, or from AController to a specific Pawn class.
 */

DECLARE_LOG_CATEGORY_EXTERN(OnsetCore, Log, All);

UCLASS()
class ONSET_API UOnsetStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:                                                                                                         
	UFUNCTION(BlueprintPure, Category="Onset")                                                                  
	static AOnsetPlayerController* GetOnsetPlayerController(UObject* WorldContext, int32 PlayerIndex = 0);      
                                                                                                                     
	UFUNCTION(BlueprintPure, Category="Onset")                                                                  
	static AOnsetPlayerState* GetOnsetPlayerState(UObject* WorldContext, int32 PlayerIndex = 0);                
                                                                                                                     
	UFUNCTION(BlueprintPure, Category="Onset")                                                                  
	static UTargetingComponent* GetTargetingComponent(AActor* Actor);                                           
                                                                                                                     
	UFUNCTION(BlueprintPure, Category="Onset")                                                                  
	static AOnsetPlayerCharacter* GetOnsetPlayerCharacter(UObject* WorldContext, int32 PlayerIndex = 0);        
                                                                                                                     
	UFUNCTION(BlueprintPure, Category="Onset")                                                                  
	static bool IsPvPEnabled(UObject* WorldContext, int32 PlayerIndex = 0);                                     
};                                                                                                              