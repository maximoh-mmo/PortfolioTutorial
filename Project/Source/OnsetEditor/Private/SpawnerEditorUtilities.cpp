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
		
		const int32 Index = Spawner->SpawnPoints.IndexOfByKey(
			DeletedActor
			);
		
		if (Index == INDEX_NONE)
		{
			Spawner->Modify();
			Spawner->SpawnPoints.RemoveAt(Index);
			Spawner->MarkPackageDirty();
		}
	}
}
