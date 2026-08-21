// Fill out your copyright notice in the Description page of Project Settings.


#include "Core/OnsetBaseCharacter.h"

#include "AbilitySystemComponent.h"
#include "Combat/OnsetEquipmentLibrary.h"
#include "Core/OnsetCCDiminishingComponent.h"
#include "Data/OnsetClassInfoTypes.h"
#include "GAS/OnsetAttributeSet.h"
#include "Combat/OnsetGA_BasicAttack.h"
#include "Combat/OnsetGA_HitReaction.h"
#include "Combat/OnsetGA_AoE.h"
#include "Combat/OnsetGA_Cone.h"
#include "Combat/OnsetGA_Shadowstep.h"
#include "Combat/OnsetGA_CooldownSlow.h"
#include "Combat/OnsetGA_Generic.h"
#include "Combat/OnsetAbilityLibrary.h"
#include "Data/OnsetAbilityTypes.h"
#include "Inventory/UOnsetInventoryComponent.h"
#include "Quest/UOnsetQuestComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "GAS/OnsetMovementAttributeSet.h"
#include "GAS/OnsetCombatAttributeSet.h"
#include "Materials/MaterialInterface.h"
#include "Core/TargetingComponent.h"
#include "Dom/JsonObject.h"
#include "Net/UnrealNetwork.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/ConstructorHelpers.h"

AOnsetBaseCharacter::AOnsetBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("TargetingComponent"));
	AttributeSet = CreateDefaultSubobject<UOnsetAttributeSet>(TEXT("AttributeSet"));
	MovementAttributes = CreateDefaultSubobject<UOnsetMovementAttributeSet>(TEXT("MovementAttributes"));
	CombatAttributes = CreateDefaultSubobject<UOnsetCombatAttributeSet>(TEXT("CombatAttributes"));
	CCDiminishing = CreateDefaultSubobject<UOnsetCCDiminishingComponent>(TEXT("CCDiminishing"));

	// Player pawns replicate the bag + loadout to their owner only.
	InventoryComponent = CreateDefaultSubobject<UOnsetInventoryComponent>(TEXT("InventoryComponent"));
	InventoryComponent->SetReplicateToOwnerOnly(true);
	InventoryComponent->OnInventoryChanged.AddUObject(this, &AOnsetBaseCharacter::HandleInventoryChanged);

	// Quest tracker (player pawns; inert on enemies/corpses).
	QuestComponent = CreateDefaultSubobject<UOnsetQuestComponent>(TEXT("QuestComponent"));

	// Ground reticule decal: hidden until this character becomes the player's target.
	TargetReticuleDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("TargetReticuleDecal"));
	TargetReticuleDecal->SetupAttachment(GetCapsuleComponent());
	TargetReticuleDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	TargetReticuleDecal->SetRelativeLocation(FVector(0.0f, 0.0f, -(GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 60.0f)));
	TargetReticuleDecal->DecalSize = FVector(120.0f, 50.0f, 50.0f);
	TargetReticuleDecal->SetHiddenInGame(true);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultReticuleMat(TEXT("/Game/Materials/M_TargetReticule.M_TargetReticule"));
	if (DefaultReticuleMat.Succeeded())
	{
		TargetReticuleMaterial = DefaultReticuleMat.Object;
	}
	else
	{
		// Fallback so the reticule still renders if the content asset is missing.
		static ConstructorHelpers::FObjectFinder<UMaterialInterface> FallbackReticuleMat(TEXT("/Engine/EngineDebugMaterials/DefaultDeferredDecalMaterial.DefaultDeferredDecalMaterial"));
		if (FallbackReticuleMat.Succeeded())
		{
			TargetReticuleMaterial = FallbackReticuleMat.Object;
		}
	}
}

void AOnsetBaseCharacter::InitAbilityActorInfo()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void AOnsetBaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (!HasAuthority()) return;
	InitAbilityActorInfo();
	GrantDefaultAbilities();
}

void AOnsetBaseCharacter::GrantDefaultAbilities()
{
	if (!AbilitySystemComponent || bAbilitiesGranted) return;

	// System abilities are not input-bound in DT_Abilities; grant them directly.
	// These are required for combat regardless of the table.
	UClass* BasicAttackAbility = LoadObject<UClass>(nullptr, (TEXT("/Game/Game/Combat/GA_BasicAttack.GA_BasicAttack_C")));
	if (!BasicAttackAbility) BasicAttackAbility = UOnsetGA_BasicAttack::StaticClass();
	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(BasicAttackAbility, 1, INDEX_NONE, this));

	UClass* HitReaction = LoadObject<UClass>(nullptr, (TEXT("/Game/Game/Combat/GA_HitReaction.GA_HitReaction_C")));
	if (!HitReaction) HitReaction = UOnsetGA_HitReaction::StaticClass();
	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(HitReaction, 1, INDEX_NONE, this));

	UClass* ShadowStepAbility = LoadObject<UClass>(nullptr, (TEXT("/Game/Game/Combat/GA_ShadowStep.GA_ShadowStep_C")));
	if (!ShadowStepAbility) ShadowStepAbility = UOnsetGA_Shadowstep::StaticClass();
	AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(ShadowStepAbility, 1, INDEX_NONE, this)); // Passive

	// Input-bound abilities come from DT_Abilities. Each row with InputID >= 0 is
	// granted as the row's AbilityClass (default UOnsetGA_Generic) with its ID tag
	// (AbilityID.<RowName>) in DynamicAbilityTags so the generic runtime can resolve
	// the row. Falls back to the legacy hardcoded grants when the table is missing.
	UDataTable* AbilityTable = UOnsetAbilityLibrary::GetAbilityTable();
	bool bGrantedFromTable = false;
	if (AbilityTable && AbilityTable->GetRowMap().Num() > 0)
	{
		UOnsetAbilityLibrary::ValidateDefinitions();

		for (const TPair<FName, uint8*>& RowPair : AbilityTable->GetRowMap())
		{
			const FOnsetAbilityDefinition* Definition = reinterpret_cast<const FOnsetAbilityDefinition*>(RowPair.Value);
			if (!Definition || Definition->InputID < 0)
			{
				continue;
			}

			TSubclassOf<UOnsetGameplayAbility> AbilityClass =
				UOnsetAbilityLibrary::ResolveAbilityClass(RowPair.Key);
			if (!AbilityClass)
			{
				AbilityClass = UOnsetGA_Generic::StaticClass();
			}

			FGameplayAbilitySpec Spec(AbilityClass, 1, Definition->InputID, this);
			Spec.DynamicAbilityTags.AddTag(UOnsetAbilityLibrary::MakeAbilityIDTag(RowPair.Key));
			AbilitySystemComponent->GiveAbility(Spec);
			bGrantedFromTable = true;
		}
	}

	if (!bGrantedFromTable)
	{
		UClass* AoEAbility = LoadObject<UClass>(nullptr, (TEXT("/Game/Game/Combat/GA_AoE.GA_AoE_C")));
		if (!AoEAbility) AoEAbility = UOnsetGA_AoE::StaticClass();
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AoEAbility, 1, 1, this)); // Input ID 1

		UClass* ConeAbility = LoadObject<UClass>(nullptr, (TEXT("/Game/Game/Combat/GA_Cone.GA_Cone_C")));
		if (!ConeAbility) ConeAbility = UOnsetGA_Cone::StaticClass();
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(ConeAbility, 1, 2, this)); // Input ID 2

		UClass* CooldownSlowAbility = LoadObject<UClass>(nullptr, (TEXT("/Game/Game/Combat/GA_CooldownSlow.GA_CooldownSlow_C")));
		if (!CooldownSlowAbility) CooldownSlowAbility = UOnsetGA_CooldownSlow::StaticClass();
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(CooldownSlowAbility, 1, 3, this)); // Input ID 3
	}

	bAbilitiesGranted = true;
}

void AOnsetBaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AOnsetBaseCharacter, bIsAlive, COND_None);
}

void AOnsetBaseCharacter::OnRep_bIsAlive()
{
	SetActorHiddenInGame(!bIsAlive);
	SetActorEnableCollision(bIsAlive);
}

void AOnsetBaseCharacter::ResetAttributes()
{
	if (!AbilitySystemComponent) return;
	AttributeSet->InitHealth(AttributeSet->GetMaxHealth());
	AttributeSet->InitMaxHealth(100.0f);
	MovementAttributes->InitMovementSpeed(600.0f);
	if (CombatAttributes)
	{
		CombatAttributes->ResetToDefaults();
	}
	bIsAlive = false; // Pool return — OnRespawn() re-enables on retrieval
}

void AOnsetBaseCharacter::ApplyCharacterBuild(EOnsetCharacterClass Class, const FString& EquipmentJSON)
{
	CurrentClass = Class;
	DeserializeEquipmentJSON(EquipmentJSON);
	RecalculateDerivedStats();
}

void AOnsetBaseCharacter::RecalculateDerivedStats()
{
	if (!CombatAttributes || !AttributeSet)
	{
		return;
	}

	const FOnsetClassBaseStats Base = UOnsetEquipmentLibrary::GetClassBaseStats(CurrentClass);

	// StartingMaxHealth comes from DT_ClassInfo (fallback 100).
	float StartingMaxHealth = 100.0f;
	if (const FOnsetCharacterClassInfo* Info = UOnsetEquipmentLibrary::GetClassInfo(CurrentClass))
	{
		StartingMaxHealth = Info->StartingMaxHealth;
	}

	// Sum flat gear bonuses across the loadout; block chance comes from the shield.
	float StrengthBonus = 0.0f;
	float IntellectBonus = 0.0f;
	float VitalityBonus = 0.0f;
	float DefenseBonus = 0.0f;
	float AgilityBonus = 0.0f;
	float LuckBonus = 0.0f;
	float BlockChance = 0.0f;

	for (const TPair<EOnsetEquipmentSlot, FName>& Pair : InventoryComponent ? InventoryComponent->GetEquippedMap() : TMap<EOnsetEquipmentSlot, FName>())
	{
		const FOnsetEquipmentDefinition* Def = UOnsetEquipmentLibrary::GetDefinition(Pair.Value);
		if (!Def)
		{
			continue;
		}

		StrengthBonus += Def->StrengthBonus;
		IntellectBonus += Def->IntellectBonus;
		VitalityBonus += Def->VitalityBonus;
		AgilityBonus += Def->AgilityBonus;
		LuckBonus += Def->LuckBonus;

		if (Pair.Key == EOnsetEquipmentSlot::Shield)
		{
			// Shield block/DEF from the item, plus the Tank mastery bonus:
			// BlockChance = item + TankMasteryBlock; ShieldDEF = item + TankMasteryDEF.
			float Block = Def->BlockChance;
			float ShieldDef = Def->DefenseBonus;
			if (CurrentClass == EOnsetCharacterClass::Tank)
			{
				Block += UOnsetEquipmentLibrary::GetTankMasteryBlock();
				ShieldDef += UOnsetEquipmentLibrary::GetTankMasteryDefense();
			}
			BlockChance = FMath::Max(BlockChance, Block);
			DefenseBonus += ShieldDef;
		}
		else if (Pair.Key == EOnsetEquipmentSlot::Head || Pair.Key == EOnsetEquipmentSlot::Chest ||
			Pair.Key == EOnsetEquipmentSlot::Hands || Pair.Key == EOnsetEquipmentSlot::Legs ||
			Pair.Key == EOnsetEquipmentSlot::Feet)
		{
			// Armor pieces contribute flat DEF; accessories only carry stat bonuses.
			DefenseBonus += Def->DefenseBonus;
		}
	}

	CombatAttributes->InitStrength(Base.Strength + StrengthBonus);
	CombatAttributes->InitIntellect(Base.Intellect + IntellectBonus);
	CombatAttributes->InitVitality(Base.Vitality + VitalityBonus);
	CombatAttributes->InitDefense(Base.Defense + DefenseBonus);
	CombatAttributes->InitAgility(Base.Agility + AgilityBonus);
	CombatAttributes->InitLuck(Base.Luck + LuckBonus);
	CombatAttributes->InitBlockChance(BlockChance);

	// MaxHealth = class base + VIT × HealthPerVitality (derived, not persisted).
	constexpr float HealthPerVitality = 10.0f;
	AttributeSet->InitMaxHealth(StartingMaxHealth + CombatAttributes->GetVitality() * HealthPerVitality);
}

FString AOnsetBaseCharacter::SerializeEquipmentJSON() const
{
	return InventoryComponent ? InventoryComponent->SerializeEquipmentJSON() : TEXT("{}");
}

void AOnsetBaseCharacter::DeserializeEquipmentJSON(const FString& JSON)
{
	if (InventoryComponent)
	{
		InventoryComponent->DeserializeEquipmentJSON(JSON);
	}
}

FString AOnsetBaseCharacter::SerializeInventoryJSON() const
{
	return InventoryComponent ? InventoryComponent->SerializeInventoryJSON() : TEXT("[]");
}

void AOnsetBaseCharacter::DeserializeInventoryJSON(const FString& JSON)
{
	if (InventoryComponent)
	{
		InventoryComponent->DeserializeInventoryJSON(JSON);
	}
}

void AOnsetBaseCharacter::EquipItem(EOnsetEquipmentSlot Slot, FName RowName)
{
	if (!InventoryComponent)
	{
		return;
	}
	InventoryComponent->EquipItem(Slot, RowName);
}

bool AOnsetBaseCharacter::EquipFromInventory(FName RowName)
{
	return InventoryComponent ? InventoryComponent->EquipFromInventory(RowName) : false;
}

float AOnsetBaseCharacter::GetEquippedWeaponDamage() const
{
	if (const FOnsetEquipmentDefinition* Weapon = GetEquippedItem(EOnsetEquipmentSlot::Weapon))
	{
		if (Weapon->WeaponDamage > 0.0f)
		{
			return Weapon->WeaponDamage;
		}
	}

	// Enemy-authored DamageBase (DT_EnemyStats) wins over the class default.
	if (EnemyWeaponBaseOverride > 0.0f)
	{
		return EnemyWeaponBaseOverride;
	}

	// No equipped weapon (or an unresolvable row): fall back to the class default weapon.
	return UOnsetEquipmentLibrary::MakeDefaultWeaponForClass(CurrentClass).WeaponDamage;
}

void AOnsetBaseCharacter::SetEnemyWeaponStats(float InWeaponBase, EOnsetWeaponArchetype InArchetype)
{
	EnemyWeaponBaseOverride = FMath::Max(0.0f, InWeaponBase);
	EnemyWeaponArchetype = InArchetype;
}

EOnsetWeaponArchetype AOnsetBaseCharacter::GetBaseWeaponArchetype() const
{
	if (const FOnsetEquipmentDefinition* Weapon = GetEquippedItem(EOnsetEquipmentSlot::Weapon))
	{
		return Weapon->Archetype;
	}
	if (EnemyWeaponBaseOverride > 0.0f)
	{
		return EnemyWeaponArchetype;
	}
	return UOnsetEquipmentLibrary::MakeDefaultWeaponForClass(CurrentClass).Archetype;
}

const FOnsetEquipmentDefinition* AOnsetBaseCharacter::GetEquippedItem(EOnsetEquipmentSlot Slot) const
{
	return InventoryComponent ? InventoryComponent->GetEquippedItem(Slot) : nullptr;
}

void AOnsetBaseCharacter::HandleInventoryChanged()
{
	// Derived stats are server-authoritative; clients receive the replicated attributes.
	if (HasAuthority())
	{
		RecalculateDerivedStats();
	}
}

void AOnsetBaseCharacter::OnDeath(AActor* KillingActor)
{
	bIsAlive = false;
	SetTargetReticule(false);
}

bool AOnsetBaseCharacter::IsAlive() const
{
	return bIsAlive && AttributeSet && AttributeSet->GetHealth() > 0.0f;
}

void AOnsetBaseCharacter::OnRespawn()
{
	bIsAlive = true;
	SetActorHiddenInGame(false);                                                                   
	SetActorTickEnabled(true);
	SetActorEnableCollision(true);
}

void AOnsetBaseCharacter::SetTargetReticule(bool bShow)
{
	if (!TargetReticuleDecal)
	{
		return;
	}

	bTargetReticleVisible = bShow;

	if (bShow)
	{
		if (TargetReticuleMaterial)
		{
			TargetReticuleDecal->SetDecalMaterial(TargetReticuleMaterial);
		}

		UpdateTargetReticule();
		TargetReticuleDecal->SetHiddenInGame(false);
	}
	else
	{
		TargetReticuleDecal->SetHiddenInGame(true);
	}
}

void AOnsetBaseCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bTargetReticleVisible)
	{
		UpdateTargetReticule();
	}
}

void AOnsetBaseCharacter::UpdateTargetReticule()
{
	if (!TargetReticuleDecal || !GetWorld())
	{
		return;
	}

	// Trace straight down from the capsule centre to find the surface below,
	// then park the decal there so it hugs uneven terrain. The projection box
	// is centered on the decal (DecalSize is a half-extent), so sink it by the
	// full depth: the top face sits at the surface, and it never reaches back
	// up into the character's body.
	const FVector TraceStart = GetCapsuleComponent()->GetComponentLocation();
	const FVector TraceEnd = TraceStart - FVector(0.0f, 0.0f, 2000.0f);

	FHitResult Hit;
	FCollisionQueryParams QueryParams(FName(TEXT("TargetReticuleTrace")), false, this);
	QueryParams.bTraceComplex = false;

	if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams) && Hit.bBlockingHit)
	{
		const float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
		constexpr float ReticuleDepth = 80.0f;

		// Decals project along local +X, so point +X against the surface normal.
		const FRotator SurfaceRotation = FRotationMatrix::MakeFromX(-Hit.ImpactNormal).Rotator();

		// Slight margin above the surface so the ground plane is strictly inside the box.
		const FVector ReticuleCenter = Hit.ImpactPoint - Hit.ImpactNormal * (ReticuleDepth - 5.0f);
		TargetReticuleDecal->SetWorldLocationAndRotation(ReticuleCenter, SurfaceRotation);
		TargetReticuleDecal->DecalSize = FVector(ReticuleDepth, Radius * 1.4f, Radius * 1.4f);
	}
}
