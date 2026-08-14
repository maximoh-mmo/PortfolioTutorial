#include "SpawnerEditorUtilities.h"

#include "EngineUtils.h"
#include "Spawning/OnsetSpawner.h"
#include "Spawning/SpawnPoint.h"

void FSpawnerEditorUtilities::OnActorDeleted(AActor* Actor)
{
	ASpawnPoint* DeletedActor = Cast<ASpawnPoint>(Actor);
	if (!DeletedActor) return;
	
	for (TActorIterator<AOnsetSpawner> It(DeletedActor->GetWorld()); It; ++It)
	{
		AOnsetSpawner* Spawner = *It;
		if (!Spawner) continue;

		bool bChanged = false;

		const int32 Index = Spawner->SpawnPoints.IndexOfByKey(DeletedActor);
		if (Index != INDEX_NONE)
		{
			Spawner->Modify();
			Spawner->SpawnPoints.RemoveAt(Index);
			bChanged = true;
		}

		// The reference may already have been auto-nulled before this callback
		// fired; drop any remaining null entries so the array stays compact.
		const int32 Before = Spawner->SpawnPoints.Num();
		Spawner->SpawnPoints.RemoveAll(
			[](const TObjectPtr<ASpawnPoint>& Point) { return !IsValid(Point); });
		if (Spawner->SpawnPoints.Num() != Before)
		{
			bChanged = true;
		}

		if (bChanged)
		{
			Spawner->MarkPackageDirty();
			Spawner->RelocateSpawnPoints();
		}
	}
}
