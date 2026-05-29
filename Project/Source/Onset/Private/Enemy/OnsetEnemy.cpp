#include "Enemy/OnsetEnemy.h"

#include "AI/AIProfile.h"
#include "AI/OnsetAIController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Enemy/GroupComponent.h"

AOnsetEnemy::AOnsetEnemy()
{
	this->Tags.Add(FName("Enemy"));
	AIControllerClass = AOnsetAIController::StaticClass();
	GroupComp = CreateDefaultSubobject<UGroupComponent>(TEXT("GroupComp"));
}

void AOnsetEnemy::ApplyProfile(const UAIProfile* InProfile)
{
	Profile = InProfile;
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp) return;

	if (Profile && Profile->OverrideMaterial)
	{
		MeshComp->SetMaterial(0, Profile->OverrideMaterial);
	}
	else
	{
		MeshComp->SetMaterial(0, nullptr);
	}
}
