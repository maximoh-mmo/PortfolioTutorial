// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawning/SpawnPoint.h"
#include "Components/SkeletalMeshComponent.h"
#include "Enemy/Profile/VisualProfile.h"
#include "Spawning/OnsetSpawner.h"
#include "VisualLogger/VisualLoggerTypes.h"


// Sets default values
ASpawnPoint::ASpawnPoint()
{
	if (AOnsetSpawner* ParentActor = Cast<AOnsetSpawner>(GetParentActor()))
	{
		USkeletalMeshComponent* Mesh = CreateDefaultSubobject<USkeletalMeshComponent>("SkeletalMeshComponent");
		USkeletalMesh* MeshAsset = ParentActor->Config.EnemyVisualProfile->SkeletalMesh.LoadSynchronous();
		UMaterialInterface* Material = ParentActor->Config.EnemyVisualProfile->OverrideMaterial.Get();
		Mesh->SetSkeletalMesh(MeshAsset);
		Mesh->SetMaterial(0,Material);
	}
}