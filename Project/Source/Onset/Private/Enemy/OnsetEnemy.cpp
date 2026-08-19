#include "Enemy/OnsetEnemy.h"

#include "TimerManager.h"
#include "Combat/OnsetEquipmentLibrary.h"
#include "Combat/OnsetLootLibrary.h"
#include "Corpse/OnsetCorpse.h"
#include "Data/OnsetEquipmentTypes.h"
#include "GAS/OnsetAttributeSet.h"
#include "GAS/OnsetCombatAttributeSet.h"
#include "Inventory/UOnsetInventoryComponent.h"
#include "GAS/OnsetGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Subsystem/OnsetCorpseSubsystem.h"
#include "Enemy/GroupComponent.h"
#include "Enemy/Profile/VisualProfile.h"
#include "Player/OnsetPlayerCharacter.h"
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

void AOnsetEnemy::ApplyEnemyStats(FName RowName, int32 Tier)
{
	if (!AttributeSet || !CombatAttributes || !AbilitySystemComponent)
	{
		return;
	}

	// Remember for death-time loot rolling.
	EnemyStatsRow = RowName;
	DifficultyTier = Tier;

	// Clear any affinity tag from a previous life so a neutral row is truly neutral.
	const FGameplayTag AffinityTags[] = { TAG_Element_Fire, TAG_Element_Ice, TAG_Element_Lightning, TAG_Element_Poison };
	for (const FGameplayTag& Tag : AffinityTags)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(Tag);
	}

	const FOnsetEnemyStats* Stats = UOnsetEquipmentLibrary::GetEnemyStats(RowName);
	if (!Stats)
	{
		// No row (or no table): reset to the shared defaults + class default weapon.
		// NOTE: stats only — do NOT call ResetAttributes() here, it clears bIsAlive
		// (the pool-alive flag OnRespawn() just set) which would hide the enemy and
		// disable its collision on clients via OnRep_bIsAlive.
		EnemyLevel = 1;
		XpReward = 0;
		AttributeSet->InitMaxHealth(100.0f);
		AttributeSet->InitHealth(100.0f);
		if (CombatAttributes)
		{
			CombatAttributes->ResetToDefaults();
		}
		SetEnemyWeaponStats(0.0f, EOnsetWeaponArchetype::Sword);
		return;
	}

	// XP progression inputs (combat-formulas §12).
	EnemyLevel = FMath::Max(1, Stats->Level);
	XpReward = FMath::Max(0, Stats->XpReward);

	// EnemyStats_loopN = base x (1 + d)^Tier (d = 15%, combat-formulas §10).
	const float Difficulty = UOnsetEquipmentLibrary::GetEnemyDifficultyMultiplier(Tier);
	const float ScaledMaxHealth = FMath::Max(1.0f, Stats->MaxHealth * Difficulty);

	AttributeSet->InitMaxHealth(ScaledMaxHealth);
	AttributeSet->InitHealth(ScaledMaxHealth);

	CombatAttributes->InitDefense(Stats->Defense);
	CombatAttributes->InitResistanceFire(Stats->ResistanceFire);
	CombatAttributes->InitResistanceIce(Stats->ResistanceIce);
	CombatAttributes->InitResistanceLightning(Stats->ResistanceLightning);
	CombatAttributes->InitResistancePoison(Stats->ResistancePoison);
	CombatAttributes->InitLuck(Stats->Luck);

	SetEnemyWeaponStats(FMath::Max(0.0f, Stats->DamageBase * Difficulty), Stats->WeaponArchetype);

	// Element affinity feeds the type chart (target side).
	switch (Stats->ElementAffinity)
	{
		case EOnsetDamageElement::Fire:			AbilitySystemComponent->AddLooseGameplayTag(TAG_Element_Fire); break;
		case EOnsetDamageElement::Ice:			AbilitySystemComponent->AddLooseGameplayTag(TAG_Element_Ice); break;
		case EOnsetDamageElement::Lightning:	AbilitySystemComponent->AddLooseGameplayTag(TAG_Element_Lightning); break;
		case EOnsetDamageElement::Poison:		AbilitySystemComponent->AddLooseGameplayTag(TAG_Element_Poison); break;
		default: break; // Physical = neutral.
	}
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
			AOnsetCorpse* Corpse = CorpseSub->SpawnCorpse(GetActorTransform(), VisualProfile->CorpseMesh.LoadSynchronous());
			if (Corpse)
			{
				// Roll the enemy's loot server-side and stamp it before the corpse's
				// first replication so every client sees the same contents.
				FOnsetLootContext Context;
				Context.Level = DifficultyTier;
				Context.ZoneTag = ZoneTag;
				const FOnsetEnemyStats* Stats = UOnsetEquipmentLibrary::GetEnemyStats(EnemyStatsRow);
				const TArray<FOnsetInventoryEntry> Loot = Stats ? UOnsetLootLibrary::RollLoot(Stats->LootTable, Context) : TArray<FOnsetInventoryEntry>();
				Corpse->InventoryComponent->SetItems(Loot);
				if (Loot.Num() == 0)
				{
					// Nothing to loot: expire fast so the field isn't littered with empty corpses.
					Corpse->SetLifeSpan(4.0f);
				}
				UE_LOG(LogTemp, Log, TEXT("OnDeath: %s dropped %d items"), *GetName(), Loot.Num());
			}
		}
	}

	// XP grant (combat-formulas §12): idle/autoplay kills count because KillingActor
	// is the player's pawn regardless of who possesses it.
	if (AOnsetPlayerCharacter* PlayerChar = Cast<AOnsetPlayerCharacter>(KillingActor))
	{
		PlayerChar->GrantXPFromEnemy(EnemyLevel, XpReward);
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
