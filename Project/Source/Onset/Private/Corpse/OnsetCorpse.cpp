#include "Corpse/OnsetCorpse.h"

#include "Components/StaticMeshComponent.h"

AOnsetCorpse::AOnsetCorpse()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CorpseMesh"));
	RootComponent = MeshComponent;
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
	MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	MeshComponent->SetWorldScale3D(FVector(0.5f));
	MeshComponent->SetIsReplicated(true);
}
