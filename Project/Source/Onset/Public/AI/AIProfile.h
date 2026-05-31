// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AIProfile.generated.h"

class UStateTree;
class UMaterialInterface;
class USkeletalMesh;
class UAnimInstance;

/** Data asset that defines an NPC variant: visuals, behaviour parameters, and the StateTree asset. */
UCLASS(BlueprintType)
class ONSET_API UAIProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Skeletal mesh for this NPC variant. Leave null to use cube fallback. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	/** Anim BP applied after the skeletal mesh is set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	TSubclassOf<UAnimInstance> AnimBlueprintClass;

	/** Optional material override applied to the mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	TObjectPtr<UMaterialInterface> OverrideMaterial;

	/** StateTree asset that drives NPC behaviour. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StateTree")
	TObjectPtr<UStateTree> StateTreeAsset;

	/** Maximum distance at which sight sense can detect targets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	float SightRange = 1500.0f;

	/** Horizontal field-of-view angle for sight detection (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	float SightAngle = 90.0f;

	/** Maximum distance at which hearing sense can detect noise events. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	float HearingRange = 800.0f;

	/** 0 = passive, 1 = aggressive. Influences agro and attack thresholds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Aggression = 0.7f;

	/** Health fraction below which NPC flees (0 = never flee). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FleeThreshold = 0.0f;

	/** Radius in which hearing a combat noise triggers assist behaviour. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
	float AssistRadius = 600.0f;
};
