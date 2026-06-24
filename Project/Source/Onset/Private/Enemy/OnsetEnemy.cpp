#include "Enemy/OnsetEnemy.h"

#include "TimerManager.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Subsystems/OnsetCorpseSubsystem.h"
#include "Enemy/GroupComponent.h"
#include "Enemy/Profile/VisualProfile.h"
#include "Spawning/OnsetSpawner.h"
#include "Subsystems/OnsetThreatSubsystem.h"

AOnsetEnemy::AOnsetEnemy()
{
	this->Tags.Add(FName("Enemy"));
	GroupComp = CreateDefaultSubobject<UGroupComponent>(TEXT("GroupComp"));
}

void AOnsetEnemy::ApplyProfile(UVisualProfile* InProfile)
{
	VisualProfile = InProfile;
	USkeletalMeshComponent* SkeletalComp = GetMesh();
	if (!SkeletalComp) return;

	if (UStaticMeshComponent* OldCube = FindComponentByClass<UStaticMeshComponent>())
		OldCube->DestroyComponent(false);

	if (VisualProfile)
	{
		const bool bHasSkeletal = !VisualProfile->SkeletalMesh.IsNull();

		if (bHasSkeletal)
		{
			if (USkeletalMesh* SkeletalMesh = VisualProfile->SkeletalMesh.LoadSynchronous())
			{
				SkeletalComp->SetSkeletalMesh(SkeletalMesh);
				FBoxSphereBounds Bounds = SkeletalMesh->GetImportedBounds();
				float Radius = FMath::Max(Bounds.BoxExtent.X, Bounds.BoxExtent.Y);
				float HalfHeight = FMath::Max(Radius, Bounds.BoxExtent.Z);
				GetCapsuleComponent()->SetCapsuleSize(Radius, HalfHeight);
			}
			SkeletalComp->SetHiddenInGame(false);

			if (VisualProfile->AnimBlueprintClass)
				SkeletalComp->SetAnimInstanceClass(VisualProfile->AnimBlueprintClass);
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

		if (VisualProfile->OverrideMaterial)
		{
			if (bHasSkeletal)
				SkeletalComp->SetMaterial(0, VisualProfile->OverrideMaterial);
			else if (UStaticMeshComponent* CubeVis = FindComponentByClass<UStaticMeshComponent>())
				CubeVis->SetMaterial(0, VisualProfile->OverrideMaterial);
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
	Super::OnDeath(KillingActor);
	if (UOnsetCorpseSubsystem* CorpseSub = GetWorld()->GetSubsystem<UOnsetCorpseSubsystem>())
	{
		if (VisualProfile)
		{
			UStaticMesh* CorpseMesh = VisualProfile->CorpseMesh.IsNull() ? nullptr : VisualProfile->CorpseMesh.LoadSynchronous();
			CorpseSub->SpawnCorpse(GetActorTransform(), CorpseMesh);
		}
	}
	GetWorldTimerManager().SetTimerForNextTick(this, &AOnsetEnemy::DeferredDeathCleanup);
}

void AOnsetEnemy::DeferredDeathCleanup()
{
	if (UOnsetThreatSubsystem* ThreatSub = GetWorld()->GetSubsystem<UOnsetThreatSubsystem>())
	{
		ThreatSub->RemoveEnemy(this);
	}
	if (!OwningSpawner)	return;
	OwningSpawner->OnNPCDeath(this);
}
