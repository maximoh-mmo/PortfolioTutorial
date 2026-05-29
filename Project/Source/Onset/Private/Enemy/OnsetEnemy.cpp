#include "Enemy/OnsetEnemy.h"

#include "AI/AIProfile.h"
#include "AI/OnsetAIController.h"
#include "Animation/AnimInstance.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Enemy/GroupComponent.h"
#include "UObject/ConstructorHelpers.h"

AOnsetEnemy::AOnsetEnemy()
{
	this->Tags.Add(FName("Enemy"));
	AIControllerClass = AOnsetAIController::StaticClass();
	GroupComp = CreateDefaultSubobject<UGroupComponent>(TEXT("GroupComp"));

	FallbackMeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FallbackMeshComp"));
	FallbackMeshComp->SetupAttachment(RootComponent);
	FallbackMeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		FallbackMeshComp->SetStaticMesh(CubeMesh.Object);
	}
	FallbackMeshComp->SetHiddenInGame(true);
}

void AOnsetEnemy::ApplyProfile(UAIProfile* InProfile)
{
	Profile = InProfile;
	USkeletalMeshComponent* SkeletalMeshComp = GetMesh();
	if (!SkeletalMeshComp) return;

	if (Profile)
	{
		const bool bHasSkeletalMesh = !Profile->SkeletalMesh.IsNull();

		if (bHasSkeletalMesh)
		{
			if (USkeletalMesh* NewMesh = Profile->SkeletalMesh.LoadSynchronous())
				SkeletalMeshComp->SetSkeletalMesh(NewMesh);
			SkeletalMeshComp->SetHiddenInGame(false);
			if (Profile->AnimBlueprintClass)
				SkeletalMeshComp->SetAnimInstanceClass(Profile->AnimBlueprintClass);
		}
		else
		{
			SkeletalMeshComp->SetSkeletalMesh(nullptr);
			SkeletalMeshComp->SetAnimInstanceClass(nullptr);
			SkeletalMeshComp->SetHiddenInGame(true);
		}

		if (FallbackMeshComp)
			FallbackMeshComp->SetHiddenInGame(bHasSkeletalMesh);

		UMeshComponent* ActiveMesh = bHasSkeletalMesh
			? static_cast<UMeshComponent*>(SkeletalMeshComp)
			: static_cast<UMeshComponent*>(FallbackMeshComp);
		if (ActiveMesh)
		{
			if (Profile->OverrideMaterial)
				ActiveMesh->SetMaterial(0, Profile->OverrideMaterial);
			else
				ActiveMesh->SetMaterial(0, nullptr);
		}
	}
	else
	{
		SkeletalMeshComp->SetSkeletalMesh(nullptr);
		SkeletalMeshComp->SetAnimInstanceClass(nullptr);
		SkeletalMeshComp->SetHiddenInGame(true);
		if (FallbackMeshComp) FallbackMeshComp->SetHiddenInGame(true);
	}
}
