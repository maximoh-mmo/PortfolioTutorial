#include "Enemy/OnsetEnemy.h"

#include "AI/AIProfile.h"
#include "AI/OnsetAIController.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
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
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp) return;

	if (Profile)
	{
		if (!Profile->SkeletalMesh.IsNull())
		{
			USkeletalMesh* Mesh = Profile->SkeletalMesh.LoadSynchronous();
			if (Mesh) MeshComp->SetSkeletalMesh(Mesh);
		}
		if (Profile->AnimBlueprintClass)
		{
			MeshComp->SetAnimInstanceClass(Profile->AnimBlueprintClass);
		}
		if (Profile->OverrideMaterial)
		{
			MeshComp->SetMaterial(0, Profile->OverrideMaterial);
		}
		else
		{
			MeshComp->SetMaterial(0, nullptr);
		}
	}
	else
	{
		MeshComp->SetSkeletalMesh(nullptr);
		MeshComp->SetAnimInstanceClass(nullptr);
		MeshComp->SetMaterial(0, nullptr);
	}
}
