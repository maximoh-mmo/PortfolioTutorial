// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawning/SpawnPoint.h"
#if WITH_EDITOR
#include "Components/SkeletalMeshComponent.h"
#endif

// Sets default values
ASpawnPoint::ASpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

#if WITH_EDITOR
	PreviewMesh = CreateDefaultSubobject<USkeletalMeshComponent>(
		TEXT("PreviewMesh")
	);

	PreviewMesh->SetupAttachment(SceneRoot);

	PreviewMesh->SetIsVisualizationComponent(true);

	PreviewMesh->SetCollisionEnabled(
		ECollisionEnabled::NoCollision
	);

	PreviewMesh->SetGenerateOverlapEvents(false);
	#endif
}
#if WITH_EDITOR
void ASpawnPoint::SetPreview(
	USkeletalMesh* Mesh,
	UMaterialInterface* Material)
{
	if (!PreviewMesh)
	{
		return;
	}

	PreviewMesh->SetSkeletalMeshAsset(Mesh);
	PreviewMesh->SetMaterial(0, Material);
}

#endif