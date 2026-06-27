#include "Enemy/OnsetEnemy.h"

#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Subsystem/OnsetCorpseSubsystem.h"
#include "Enemy/GroupComponent.h"
#include "Enemy/Profile/VisualProfile.h"
#include "Spawning/OnsetSpawner.h"
#include "Subsystem/OnsetThreatSubsystem.h"

AOnsetEnemy::AOnsetEnemy()
{
	this->Tags.Add(FName("Enemy"));
	GroupComp = CreateDefaultSubobject<UGroupComponent>(TEXT("GroupComp"));
	SetReplicateMovement(true);
}

void AOnsetEnemy::ApplyProfile(UVisualProfile* InProfile)
{
	checkf(!InProfile || !InProfile->SkeletalMesh.IsNull(),
	       TEXT("ApplyProfile called with a VisualProfile that has no SkeletalMesh assigned"));
	VisualProfile = InProfile;
	USkeletalMeshComponent* SkeletalComp = GetMesh();
	if (!SkeletalComp) return;

	if (VisualProfile)
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

		if (VisualProfile->OverrideMaterial)
			SkeletalComp->SetMaterial(0, VisualProfile->OverrideMaterial);
		else
			SkeletalComp->SetMaterial(0, nullptr);
	}
	else
	{
		SkeletalComp->SetSkeletalMesh(nullptr);
		SkeletalComp->SetAnimInstanceClass(nullptr);
		SkeletalComp->SetHiddenInGame(true);
		SkeletalComp->SetMaterial(0, nullptr);
	}
}

void AOnsetEnemy::OnRep_VisualProfile()
{
	ApplyProfile(VisualProfile);
}

void AOnsetEnemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AOnsetEnemy, VisualProfile);
}

void AOnsetEnemy::OnDeath(AActor* KillingActor)
{
	Super::OnDeath(KillingActor);
	if (!HasAuthority()) return;
	if (UOnsetCorpseSubsystem* CorpseSub = GetWorld()->GetSubsystem<UOnsetCorpseSubsystem>())
	{
		if (VisualProfile && !VisualProfile->CorpseMesh.IsNull())
		{
			CorpseSub->SpawnCorpse(GetActorTransform(), VisualProfile->CorpseMesh.LoadSynchronous());
		}
	}
	GetWorldTimerManager().SetTimerForNextTick(this, &AOnsetEnemy::DeferredDeathCleanup);
}

void AOnsetEnemy::DeferredDeathCleanup()
{
	if (!HasAuthority()) return;
	if (UOnsetThreatSubsystem* ThreatSub = GetWorld()->GetSubsystem<UOnsetThreatSubsystem>())
	{
		ThreatSub->RemoveEnemy(this);
	}
	if (!OwningSpawner)	return;
	OwningSpawner->OnNPCDeath(this);
}
