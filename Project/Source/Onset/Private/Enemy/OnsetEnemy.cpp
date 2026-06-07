#include "Enemy/OnsetEnemy.h"

#include "TimerManager.h"
#include "AI/AIProfile.h"
#include "AI/OnsetAIController.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Corpse/OnsetCorpseSubsystem.h"
#include "Enemy/GroupComponent.h"
#include "Spawning/OnsetSpawner.h"

AOnsetEnemy::AOnsetEnemy()
{
	this->Tags.Add(FName("Enemy"));
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
			if (USkeletalMesh* SkeletalMesh = Profile->SkeletalMesh.LoadSynchronous())
			{
				SkeletalComp->SetSkeletalMesh(SkeletalMesh);
				FBoxSphereBounds Bounds = SkeletalMesh->GetImportedBounds();
				float Radius = FMath::Max(Bounds.BoxExtent.X, Bounds.BoxExtent.Y);
				float HalfHeight = FMath::Max(Radius, Bounds.BoxExtent.Z);
				GetCapsuleComponent()->SetCapsuleSize(Radius, HalfHeight);
			}
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
			CubeVis->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			CubeVis->SetCollisionObjectType(ECC_WorldDynamic);
			CubeVis->SetCollisionResponseToAllChannels(ECR_Block);
			CubeVis->SetWorldScale3D(FVector(1.0f));
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

void AOnsetEnemy::OnDeath(AActor* KillingActor)
{
	if (UOnsetCorpseSubsystem* CorpseSub = GetWorld()->GetSubsystem<UOnsetCorpseSubsystem>())
	{
		UStaticMesh* CorpseMesh = Profile->CorpseMesh.IsNull() ? nullptr : Profile->CorpseMesh.LoadSynchronous();
		CorpseSub->SpawnCorpse(GetActorTransform(), CorpseMesh);
	}

	GetWorldTimerManager().SetTimerForNextTick(this, &AOnsetEnemy::DeferredDeathCleanup);
}

void AOnsetEnemy::DeferredDeathCleanup()
{
	OwningSpawner->OnNPCDeath(this);
}
