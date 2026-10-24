#include "Enemy/OnsetEnemy.h"
#include "Player/OnsetCharacterAppearanceComponent.h"
#include "Engine/StaticMesh.h"

#include "TimerManager.h"
#include "Combat/OnsetAbilityLibrary.h"
#include "Combat/OnsetEquipmentLibrary.h"
#include "Combat/OnsetGA_Generic.h"
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
#include "OnsetAppearanceComponent.h"
#include "OnsetAuthoredAppearance.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Quest/UOnsetQuestComponent.h"
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
    // Explicit native recipes own the mesh; a later profile OnRep must not replace it.
    if (CharacterAppearance && !CharacterAppearance->AppearanceJSON.IsEmpty()) { CharacterAppearance->ReapplyAppearance(); return; }
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

	// Modular Authored Appearance integration
	if (!Stats->AuthoredAppearance.IsNull())
	{
		if (UOnsetAuthoredAppearance* Authored = Stats->AuthoredAppearance.LoadSynchronous())
		{
			UOnsetAppearanceComponent* AppComp = FindComponentByClass<UOnsetAppearanceComponent>();
			if (!AppComp)
			{
				AppComp = NewObject<UOnsetAppearanceComponent>(this, TEXT("EnemyAppearance"));
				AppComp->RegisterComponent();
				AppComp->BindExternalBodyMesh(GetMesh());
			}
			AppComp->ApplyRecipe(Authored->Recipe);
		}
	}

	// Grant equipped combat abilities from DT_Abilities
	if (AbilitySystemComponent && Stats->EquippedAbilities.Num() > 0)
	{
		for (const FName& AbilityRow : Stats->EquippedAbilities)
		{
			if (AbilityRow.IsNone()) continue;
			TSubclassOf<UOnsetGameplayAbility> AbilityClass = UOnsetAbilityLibrary::ResolveAbilityClass(AbilityRow);
			if (!AbilityClass)
			{
				AbilityClass = UOnsetGA_Generic::StaticClass();
			}
			FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, this);
			Spec.GetDynamicSpecSourceTags().AddTag(UOnsetAbilityLibrary::MakeAbilityIDTag(AbilityRow));
			AbilitySystemComponent->GiveAbility(Spec);
		}
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
				FOnsetLootContext Context;
				Context.Level = DifficultyTier;
				Context.ZoneTag = ZoneTag;
				const FOnsetEnemyStats* Stats = UOnsetEquipmentLibrary::GetEnemyStats(EnemyStatsRow);
				const TArray<FOnsetInventoryEntry> Loot = Stats ? UOnsetLootLibrary::RollLoot(Stats->LootTable, Context) : TArray<FOnsetInventoryEntry>();
				Corpse->InventoryComponent->SetItems(Loot);
				if (Loot.Num() == 0)
				{
					Corpse->SetLifeSpan(4.0f);
				}
				UE_LOG(LogTemp, Log, TEXT("OnDeath: %s dropped %d items"), *GetName(), Loot.Num());
			}
		}
	}

	// XP grant (combat-formulas §12)
	if (AOnsetPlayerCharacter* PlayerChar = Cast<AOnsetPlayerCharacter>(KillingActor))
	{
		PlayerChar->GrantXPFromEnemy(EnemyLevel, XpReward);

		// Quest signal
		if (PlayerChar->QuestComponent && !EnemyStatsRow.IsNone())
		{
			PlayerChar->QuestComponent->ReportObjectiveProgress(EOnsetQuestObjectiveType::Kill, EnemyStatsRow, 1);
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
