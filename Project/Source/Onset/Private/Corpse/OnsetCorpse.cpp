#include "Corpse/OnsetCorpse.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Inventory/UOnsetInventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/OnsetPlayerController.h"
#include "UMHInteractableComponent.h"

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

	InteractableComponent = CreateDefaultSubobject<UMHInteractableComponent>(TEXT("InteractableComponent"));
	InteractableComponent->Definition.InteractableId = FName(TEXT("Corpse"));
	InteractableComponent->Definition.DisplayName = NSLOCTEXT("OnsetCorpse", "CorpseName", "Remains");
	InteractableComponent->Definition.ActionPrompt = NSLOCTEXT("OnsetCorpse", "LootPrompt", "Loot");
	InteractableComponent->Definition.InteractionRange = 250.0f;
	InteractableComponent->Definition.TriggerMode = EMHInteractionTriggerMode::ExplicitOnly;
}

void AOnsetCorpse::BeginPlay()
{
	Super::BeginPlay();

	if (InteractableComponent)
	{
		InteractableComponent->OnCanInteractQuery.BindUObject(this, &AOnsetCorpse::QueryCanInteract);
		InteractableComponent->OnInteractedNative.AddUObject(this, &AOnsetCorpse::HandleInteracted);
	}
}

void AOnsetCorpse::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AOnsetCorpse, bLooted);
}

bool AOnsetCorpse::QueryCanInteract(const FMHInteractionContext& Context, FText& OutFailureReason) const
{
	if (bLooted)
	{
		OutFailureReason = NSLOCTEXT("OnsetCorpse", "AlreadyLooted", "Already looted.");
		return false;
	}
	return true;
}

void AOnsetCorpse::HandleInteracted(const FMHInteractionContext& Context)
{
	PerformLoot(Context.InstigatorActor, Context.InstigatorController);
}

bool AOnsetCorpse::PerformLoot(AActor* InstigatorPawn, AController* InstigatorController)
{
	if (!HasAuthority() || bLooted || !InstigatorPawn || !InventoryComponent)
	{
		return false;
	}

	UOnsetInventoryComponent* PawnInventory = InstigatorPawn->FindComponentByClass<UOnsetInventoryComponent>();
	if (!PawnInventory)
	{
		return false;
	}

	const TArray<FOnsetInventoryEntry> Loot = InventoryComponent->GetItems();
	if (Loot.Num() > 0)
	{
		PawnInventory->AddItems(Loot);

		AOnsetPlayerController* PC = Cast<AOnsetPlayerController>(InstigatorController);
		if (!PC)
		{
			if (APawn* AsPawn = Cast<APawn>(InstigatorPawn))
			{
				PC = Cast<AOnsetPlayerController>(AsPawn->GetController());
			}
		}
		if (PC)
		{
			PC->Client_ShowLootOverlay(Loot);
		}
	}

	bLooted = true;
	if (InteractableComponent)
	{
		InteractableComponent->SetEnabled(false);
	}
	Destroy();

	UE_LOG(LogTemp, Log, TEXT("PerformLoot: %s looted %d item(s)"), *InstigatorPawn->GetName(), Loot.Num());
	return true;
}
