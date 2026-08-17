#include "Corpse/OnsetCorpse.h"

#include "Components/StaticMeshComponent.h"
#include "Inventory/UOnsetInventoryComponent.h"
#include "Net/UnrealNetwork.h"

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

	// Loot is replicated to all clients so any player can see what the server rolled.
	InventoryComponent = CreateDefaultSubobject<UOnsetInventoryComponent>(TEXT("InventoryComponent"));
	InventoryComponent->SetReplicateToOwnerOnly(false);
}

void AOnsetCorpse::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AOnsetCorpse, bLooted);
}
