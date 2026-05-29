#include "Enemy/OnsetEnemy.h"

#include "AI/AIProfile.h"
#include "AI/OnsetAIController.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Enemy/GroupComponent.h"

AOnsetEnemy::AOnsetEnemy()
{
	this->Tags.Add(FName("Enemy"));
	AIControllerClass = AOnsetAIController::StaticClass();
	GroupComp = CreateDefaultSubobject<UGroupComponent>(TEXT("GroupComp"));
}

void AOnsetEnemy::ApplyProfile(UAIProfile* InProfile)
{
	Profile = InProfile;
	USkeletalMeshComponent* SkeletalComp = GetMesh();
	if (!SkeletalComp) return;

	if (UStaticMeshComponent* OldCube = FindComponentByClass<UStaticMeshComponent>())
		OldCube->DestroyComponent(false);

	if (Profile)
	{
		const bool bHasSkeletal = !Profile->SkeletalMesh.IsNull();

		if (bHasSkeletal)
		{
			if (USkeletalMesh* Mesh = Profile->SkeletalMesh.LoadSynchronous())
				SkeletalComp->SetSkeletalMesh(Mesh);
			SkeletalComp->SetHiddenInGame(false);

			if (Profile->AnimBlueprintClass)
				SkeletalComp->SetAnimInstanceClass(Profile->AnimBlueprintClass);
		}
		else
		{
			SkeletalComp->SetSkeletalMesh(nullptr);
			SkeletalComp->SetHiddenInGame(true);

			UStaticMeshComponent* CubeVis = NewObject<UStaticMeshComponent>(this, TEXT("CubeVis"));
			CubeVis->SetupAttachment(RootComponent);
			CubeVis->RegisterComponent();
			CubeVis->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			if (UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
				CubeVis->SetStaticMesh(CubeMesh);
			CubeVis->SetHiddenInGame(false);
		}

		if (Profile->OverrideMaterial)
		{
			if (bHasSkeletal)
				SkeletalComp->SetMaterial(0, Profile->OverrideMaterial);
			else if (UStaticMeshComponent* CubeVis = FindComponentByClass<UStaticMeshComponent>())
				CubeVis->SetMaterial(0, Profile->OverrideMaterial);
		}
		else
		{
			SkeletalComp->SetMaterial(0, nullptr);
		}
	}
	else
	{
		SkeletalComp->SetSkeletalMesh(nullptr);
		SkeletalComp->SetAnimInstanceClass(nullptr);
		SkeletalComp->SetHiddenInGame(true);
		SkeletalComp->SetMaterial(0, nullptr);
	}
}
