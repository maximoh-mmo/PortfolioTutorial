// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy/Profile/VisualProfile.h"
#include "GameFramework/Actor.h"
#include "SpawnPoint.generated.h"

class USkeletalMesh;

UCLASS()
class ONSET_API ASpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASpawnPoint();

	/** Permanent root component so the actor transform is valid in all builds. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

#if WITH_EDITOR
	void SetPreview(USkeletalMesh* Mesh, UMaterialInterface* Material);
#endif
	
#if WITH_EDITORONLY_DATA
	/**
	 * True once this point has been repositioned by the user in the editor.
	 * Such points are left alone by the spawner's auto-relocation logic and
	 * still count as occupying their equidistant slot.
	 */
	UPROPERTY()
	bool bUserPlaced = false;
#endif
	
#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Transient)
	TObjectPtr<USkeletalMeshComponent> PreviewMesh;
#endif
};
