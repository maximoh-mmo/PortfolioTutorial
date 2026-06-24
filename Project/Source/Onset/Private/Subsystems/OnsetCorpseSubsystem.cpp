#include "Corpse/OnsetCorpseSubsystem.h"

#include "Components/StaticMeshComponent.h"
#include "Corpse/OnsetCorpse.h"
#include "Engine/World.h"

AOnsetCorpse* UOnsetCorpseSubsystem::SpawnCorpse(const FTransform& Transform, UStaticMesh* CorpseMesh)
{
	SweepDeadCorpses();

	while (ActiveCorpses.Num() >= MaxActiveCorpses && ActiveCorpses.Num() > 0 && MaxActiveCorpses > 0)
	{
		if (AOnsetCorpse* Oldest = ActiveCorpses[0].Get())
		{
			Oldest->Destroy();
		}
		ActiveCorpses.RemoveAt(0);
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AOnsetCorpse* Corpse = GetWorld()->SpawnActor<AOnsetCorpse>(AOnsetCorpse::StaticClass(), Transform, Params);
	if (Corpse)
	{
		if (CorpseMesh)
		{
			Corpse->MeshComponent->SetStaticMesh(CorpseMesh);
		} 
		Corpse->SetLifeSpan(CorpseLifespan);
		ActiveCorpses.Add(Corpse);
	}
	return Corpse;
}

void UOnsetCorpseSubsystem::SweepDeadCorpses()
{
	ActiveCorpses.RemoveAll([](const TWeakObjectPtr<AOnsetCorpse>& Ptr)
	{
		return !Ptr.IsValid();
	});
}
