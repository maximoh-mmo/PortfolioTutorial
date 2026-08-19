#include "Spawning/OnsetSpawner.h"

#include "TimerManager.h"
#include "AI/OnsetAIController.h"
#include "Combat/OnsetEquipmentLibrary.h"
#include "Enemy/OnsetEnemy.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "Spawning/GroupManagerComponent.h"
#include "Subsystem/OnsetPoolSubsystem.h"
#include "Spawning/SpawnerSlot.h"
#include "Spawning/SpawnPoint.h"

DEFINE_LOG_CATEGORY(LogSpawner);

AOnsetSpawner::AOnsetSpawner()
{
	GroupManager = CreateDefaultSubobject<UGroupManagerComponent>(TEXT("GroupManager"));

	// Default the enemy-stats handle to the same table the library loads at
	// runtime (respects the Onset.Gameplay EnemyStatsDataTable ini seam), so the
	// row-name dropdown in the details panel is populated without extra setup.
	if (Config.EnemyStats.DataTable == nullptr)
	{
		Config.EnemyStats.DataTable = UOnsetEquipmentLibrary::GetEnemyStatsTable();
	}
}

void AOnsetSpawner::SpawnGroup()
{
	if (!HasAuthority()) return;
	if (ResolveAIProfile() == nullptr || Config.GroupSize <= 0) return;     
	if (Slots.Num() == 0) InitSlots();                                                                          
                                                                                                                     
	for (int32 i = 0; i < Slots.Num(); i++)                                                                     
	{                                                                                                           
		FSpawnerSlot& Slot = Slots[i];                                                                          
		if (Slot.Occupant && IsValid(Slot.Occupant))                                            
			continue;                                                                                           
                                                                                                                     
		SpawnEnemyAtSlot(i);                                                                                       
	}                
}

void AOnsetSpawner::DestroyGroup()
{
	if (!HasAuthority()) return;
	for (int32 i = 0; i < Slots.Num(); i++)
	{
		if (Slots[i].Occupant == nullptr || !IsValid(Slots[i].Occupant)) continue; 
		if (GroupManager) GroupManager->UnregisterMember(Slots[i].Occupant);
		if (UOnsetPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UOnsetPoolSubsystem>()){
			PoolSubsystem->ReleasePooledEnemy(Slots[i].Occupant);
		}
		else
		{
			Slots[i].Occupant->Destroy();
		}
		Slots[i].Occupant = nullptr;
	}
}

void AOnsetSpawner::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority()) return;
	InitSlots();                                                                                                
	if (bAutoSpawn) SpawnGroup();          
}

void AOnsetSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AOnsetSpawner::InitSlots()
{
	int32 Count = FMath::Max(1, Config.GroupSize);
	Slots.SetNum(Count);
	
	for (int32 i = 0; i < Count; i++)
	{
		FSpawnerSlot& Slot = Slots[i];
		Slot.Occupant = nullptr;
		
		if (SpawnPoints.IsValidIndex(i) && SpawnPoints[i])
		{
			UE_LOG(LogSpawner, Warning, TEXT("SpawnPoints: %d has valid transform, Slot assigned with %s"), i, *SpawnPoints[i]->GetActorTransform().ToString());
			Slot.SpawnTransform = SpawnPoints[i]->GetActorTransform();
		}
		else
		{
			UE_LOG(LogSpawner, Error, TEXT("SpawnPoints: %d has no valid transform"), i);
			float Angle = (360.0f / Count) * i;
			float Rad = FMath::DegreesToRadians(Angle);
			FVector Offset = FVector(FMath::Cos(Rad), FMath::Sin(Rad), 0.0f) * Config.SpawnRadius;
			Slot.SpawnTransform = FTransform(FRotator::ZeroRotator, GetActorLocation() + Offset);
		}
	}
}

AOnsetEnemy* AOnsetSpawner::SpawnEnemyAtSlot(int32 SlotIndex)
{
	if (!HasAuthority()) return nullptr;
	UOnsetPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UOnsetPoolSubsystem>();
	if (!PoolSubsystem) return nullptr;
	if (!Slots.IsValidIndex(SlotIndex)) return nullptr;
	FSpawnerSlot& Slot = Slots[SlotIndex];
	if (Slot.Occupant && IsValid(Slot.Occupant)) return nullptr; // already occupied
	
	AOnsetEnemy* Spawned = nullptr;
	Spawned = PoolSubsystem->GetPooledEnemy();
	
	if (!Spawned)
	{
		UE_LOG(LogSpawner, Error, TEXT("No pooled enemy to spawn"))
	}
	if (Spawned)
	{
		Spawned->SetActorTransform(Slot.SpawnTransform);
		Spawned->ApplyProfile(const_cast<UVisualProfile*>(ResolveVisualProfile()));
		Spawned->ApplyEnemyStats(Config.EnemyStats.RowName, Config.Tier);
		Spawned->ZoneTag = Config.ZoneTag;
		Spawned->OwningSpawner = this;
		AOnsetAIController* AIController = PoolSubsystem->GetPooledController();
		if (!AIController)
		{
			UE_LOG(LogSpawner, Error, TEXT("SpawnEnemyAtSlot: No pooled controller available."));
			return nullptr;
		}
		
		AIController->ApplyAIProfile(const_cast<UAIProfile*>(ResolveAIProfile()));
		AIController->ApplyPerceptionProfile(ResolvePerceptionProfile());
		AIController->Possess(Spawned);
		Slot.Occupant = Spawned;
		if (GroupManager) GroupManager->RegisterMember(Spawned);
		Spawned->HomeTransform = Slot.SpawnTransform;
	}
	return Spawned;
}

const UVisualProfile* AOnsetSpawner::ResolveVisualProfile() const
{
	if (Config.EnemyVisualProfile)
	{
		return Config.EnemyVisualProfile;
	}

	if (Config.EnemyStats.DataTable && !Config.EnemyStats.RowName.IsNone())
	{
		if (const FOnsetEnemyStats* Stats = Config.EnemyStats.DataTable->FindRow<FOnsetEnemyStats>(Config.EnemyStats.RowName, nullptr))
		{
			return Stats->VisualProfile;
		}
	}

	return nullptr;
}

const UAIProfile* AOnsetSpawner::ResolveAIProfile() const
{
	if (Config.EnemyAIProfile)
	{
		return Config.EnemyAIProfile;
	}

	if (Config.EnemyStats.DataTable && !Config.EnemyStats.RowName.IsNone())
	{
		if (const FOnsetEnemyStats* Stats = Config.EnemyStats.DataTable->FindRow<FOnsetEnemyStats>(Config.EnemyStats.RowName, nullptr))
		{
			return Stats->AIProfile;
		}
	}

	return nullptr;
}

const UPerceptionProfile* AOnsetSpawner::ResolvePerceptionProfile() const
{
	if (Config.EnemyPerceptionProfile)
	{
		return Config.EnemyPerceptionProfile;
	}

	if (Config.EnemyStats.DataTable && !Config.EnemyStats.RowName.IsNone())
	{
		if (const FOnsetEnemyStats* Stats = Config.EnemyStats.DataTable->FindRow<FOnsetEnemyStats>(Config.EnemyStats.RowName, nullptr))
		{
			return Stats->PerceptionProfile;
		}
	}

	return nullptr;
}
#if WITH_EDITOR

void AOnsetSpawner::UpdateAllSpawnPointPreviews()
{
	const UVisualProfile* Resolved = ResolveVisualProfile();
	if (!Resolved)	return;
	
	USkeletalMesh* PreviewMesh = 
		Resolved->SkeletalMesh.LoadSynchronous();
	
	UMaterialInterface* PreviewMaterial =
		Resolved->OverrideMaterial.Get();
	
	for (ASpawnPoint* SpawnPoint : SpawnPoints)
	{
		if (!IsValid(SpawnPoint)) continue;
		SpawnPoint->SetPreview(
			PreviewMesh,
			PreviewMaterial
			);
	}
}

void AOnsetSpawner::RemoveInvalidSpawnPoints()
{
}

void AOnsetSpawner::DestroySpawnPoint(ASpawnPoint* SpawnPoint)
{
}

void AOnsetSpawner::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	if (!PropertyChangedEvent.Property) return;
	
	const FName PropertyName = PropertyChangedEvent.Property->GetFName();
	
	const FName MemberPropertyName = 
		PropertyChangedEvent.MemberProperty
			? PropertyChangedEvent.MemberProperty->GetFName() 
			: NAME_None;
	
	if (PropertyName == GET_MEMBER_NAME_CHECKED(
			AOnsetSpawner, 
			Config)
		|| 
		MemberPropertyName == GET_MEMBER_NAME_CHECKED(
			AOnsetSpawner,
			Config))
	{
		UpdateAllSpawnPointPreviews();
	}
	
	if (PropertyName ==	GET_MEMBER_NAME_CHECKED(
		AOnsetSpawner,
		SpawnPoints))
	{
		// The array's native "+" button appends a null slot; turn it into a
		// real spawn point actor so the two can't drift apart.
		if (PropertyChangedEvent.ChangeType == EPropertyChangeType::ArrayAdd)
		{
			const int32 LastIndex = SpawnPoints.Num() - 1;

			if (LastIndex >= 0 && !IsValid(SpawnPoints[LastIndex]))
			{
				if (ASpawnPoint* NewSpawnPoint = CreateSpawnPoint())
				{
					SpawnPoints[LastIndex] = NewSpawnPoint;
					MarkPackageDirty();
				}
			}
		}

		SyncSpawnPoints();
	    RelocateSpawnPoints();
	    UpdateAllSpawnPointPreviews();

		// Keep GroupSize in sync with the spawn point count so every point is
		// used; only ever grow it, never shrink, to respect the user's intent.
		if (SpawnPoints.Num() > Config.GroupSize)
		{
			Config.GroupSize = SpawnPoints.Num();
			MarkPackageDirty();
		}
	}
}

FVector AOnsetSpawner::GetIdealSpawnPointLocation(int32 Index, int32 Count) const
{
	const float Angle = (360.0f / Count) * Index;
	const float Rad = FMath::DegreesToRadians(Angle);
	const FVector Offset =
		FVector(FMath::Cos(Rad), FMath::Sin(Rad), 0.0f) * Config.SpawnRadius;
	return GetActorLocation() + Offset;
}

void AOnsetSpawner::RelocateSpawnPoints()
{
	const int32 Count = SpawnPoints.Num();
	if (Count <= 0) return;

	for (int32 i = 0; i < Count; i++)
	{
		ASpawnPoint* Point = SpawnPoints[i];
		if (!IsValid(Point)) continue;

		// Never move a point the user placed by hand.
		if (Point->bUserPlaced) continue;

		const FVector Ideal = GetIdealSpawnPointLocation(i, Count);
		if (!Point->GetActorLocation().Equals(Ideal, 0.1f))
		{
			// Snapshot for undo/redo before moving.
			Point->Modify();
			Point->SetActorLocation(Ideal);
		}
	}
}

void AOnsetSpawner::SyncSpawnPoints()
{
	// Find actors that were removed from the array.

	for (auto CachedPoint : EditorSpawnPointCache)
	{
		if (!IsValid(CachedPoint.Get()))
		{
			continue;
		}

		if (!SpawnPoints.Contains(CachedPoint.Get()))
		{
			CachedPoint->Destroy();
		}
	}

	// Refresh cache.

	EditorSpawnPointCache = SpawnPoints;
}

void AOnsetSpawner::PostEditUndo()
{
	Super::PostEditUndo();
	
	UpdateAllSpawnPointPreviews();

	EditorSpawnPointCache = SpawnPoints;
}

void AOnsetSpawner::PostLoad()
{
	Super::PostLoad();

	// Seed the cache so the first SpawnPoints edit can tell removed entries
	// apart from existing ones.
	EditorSpawnPointCache = SpawnPoints;
}

ASpawnPoint* AOnsetSpawner::CreateSpawnPoint()
{
	UWorld* World = GetWorld();

	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASpawnPoint* NewSpawnPoint =
		World->SpawnActor<ASpawnPoint>(
			ASpawnPoint::StaticClass(),
			GetActorLocation(),
			GetActorRotation(),
			SpawnParams
		);

	if (!NewSpawnPoint)
	{
		return nullptr;
	}

	// Nest the spawn point under the spawner so it moves with it and appears
	// grouped underneath it in the outliner.
	NewSpawnPoint->AttachToActor(
		this,
		FAttachmentTransformRules::KeepWorldTransform
	);

	if (const UVisualProfile* Profile = ResolveVisualProfile())
	{
		NewSpawnPoint->SetPreview(
			Profile->SkeletalMesh.LoadSynchronous(),
			Profile->OverrideMaterial.Get()
		);
	}

	return NewSpawnPoint;
}

#endif

void AOnsetSpawner::DebugKillAll()
{
	if (!HasAuthority()) return;
	DestroyGroup();
}

void AOnsetSpawner::DebugKillLast()
{
	if (!HasAuthority()) return;
	for (int32 i = Slots.Num() - 1; i >= 0; i--)
	{
		if (Slots[i].Occupant && IsValid(Slots[i].Occupant))
		{
			if (GroupManager) GroupManager->UnregisterMember(Slots[i].Occupant);
			if (UOnsetPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UOnsetPoolSubsystem>())
			{
				PoolSubsystem->ReleasePooledEnemy(Slots[i].Occupant);
			}
			else Slots[i].Occupant->Destroy();
			Slots[i].Occupant = nullptr;
			return;
		}
	}
}

void AOnsetSpawner::OnNPCDeath(AOnsetEnemy* Enemy)
{
	if (!HasAuthority() || !Enemy) return;
	
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		if (Slots[i].Occupant == Enemy)
		{
			Slots[i].Occupant = nullptr;
			GetWorldTimerManager().SetTimer(
				Slots[i].RespawnTimerHandle,
				FTimerDelegate::CreateUObject(this, &AOnsetSpawner::RespawnNPC, i),
				Config.RespawnDelay,
				false);
			break;
		}
	}
	UOnsetPoolSubsystem* PoolSubsystem = GetWorld()->GetSubsystem<UOnsetPoolSubsystem>();
	if (PoolSubsystem)
	{
		PoolSubsystem->ReleasePooledEnemy(Enemy);
	}
}

void AOnsetSpawner::RespawnNPC(int32 SlotIndex)
{
	if (!HasAuthority() || !Slots.IsValidIndex(SlotIndex)) return;
	
	Slots[SlotIndex].RespawnTimerHandle.Invalidate();
	SpawnEnemyAtSlot(SlotIndex);
}

