// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AIProfile.generated.h"

class UStaticMesh;
class UStateTree;
class UMaterialInterface;
class USkeletalMesh;
class UAnimInstance;

/** Data asset that defines an NPC variant: behaviour parameters, and the StateTree asset. */
UCLASS(BlueprintType)
class ONSET_API UAIProfile : public UDataAsset
{
	GENERATED_BODY()

public:

	/** StateTree asset that drives NPC behaviour. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StateTree")
	TObjectPtr<UStateTree> StateTreeAsset;
	
	/** 0 = passive, 1 = aggressive. Influences agro and attack thresholds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Aggression = 0.7f;

	/** Health fraction below which NPC flees (0 = never flee). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FleeThreshold = 0.0f;

	/** Radius in which hearing a combat noise triggers assist behaviour. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
	float AssistRadius = 600.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")                                              
	float AttackRange = 250.0f; 
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")                                              
	float ChaseRange = 1000.0f;
};
