#include "World/OnsetZoneGate.h"
#include "Components/BoxComponent.h"
#include "Multiplayer/OnsetGameModeBase.h"
#include "Engine/World.h"

AOnsetZoneGate::AOnsetZoneGate()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetBoxExtent(FVector(200.0f, 200.0f, 200.0f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetRootComponent(TriggerBox);
}

void AOnsetZoneGate::BeginPlay()
{
	Super::BeginPlay();
	OnActorBeginOverlap.AddDynamic(this, &AOnsetZoneGate::OnOverlapBegin);
}

void AOnsetZoneGate::OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor)
{
	if (!HasAuthority() || TargetZone.IsEmpty()) return;
	if (!OtherActor || !OtherActor->IsA<APawn>()) return;

	AOnsetGameModeBase* GM = GetWorld()->GetAuthGameMode<AOnsetGameModeBase>();
	if (GM)
	{
		GM->TravelToZone(TargetZone, EntryPointName);
	}
}
