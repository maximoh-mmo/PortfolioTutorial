// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VisualProfile.generated.h"

class FDataValidationContext;

/** Data asset that defines an NPC variant: Visual parameters. */
UCLASS()
class ONSET_API UVisualProfile : public UDataAsset
{
	GENERATED_BODY()
	
public:
	/** Skeletal mesh for this NPC variant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	/** Optional static mesh for the corpse. Falls back to a cube if not set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	TSoftObjectPtr<UStaticMesh> CorpseMesh;

	/** Anim BP applied after the skeletal mesh is set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	TSubclassOf<UAnimInstance> AnimBlueprintClass;

	/** Optional material override applied to the mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	TObjectPtr<UMaterialInterface> OverrideMaterial;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
