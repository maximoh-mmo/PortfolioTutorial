// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/OnsetPlayerCharacter.h"

#include "TimerManager.h"
#include "Player/OnsetMovementValidationComponent.h"
#include "Camera/CameraComponent.h"
#include "Combat/OnsetEquipmentLibrary.h"
#include "Combat/OnsetLevelingLibrary.h"
#include "GAS/OnsetAttributeSet.h"
#include "GAS/OnsetCombatAttributeSet.h"
#include "GAS/OnsetGameplayTags.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "Subsystem/OnsetPlayerDataSubsystem.h"

AOnsetPlayerCharacter::AOnsetPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Server-side movement validation for client-authoritative movement input.
	MovementValidator = CreateDefaultSubobject<UOnsetMovementValidationComponent>(TEXT("MovementValidator"));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	// lock rotation setting absolute to ignore pawn rotation.
	CameraBoom->bInheritYaw = false;  // camera stays at fixed world yaw
	CameraBoom->bInheritPitch = false; // also fix pitch for good measure
	CameraBoom->bInheritRoll = false;  // also fix roll  
	CameraBoom->TargetArmLength = 2000.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 8.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
}

void AOnsetPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	HomeTransform = FTransform(
		GetActorRotation().GetEquivalentRotator(),
		GetActorLocation()
		);
}

void AOnsetPlayerCharacter::RespawnPlayer()
{
	if (!HasAuthority()) return;
	OnRespawn();
	if (AttributeSet)
	{
		AttributeSet->SetHealth(AttributeSet->GetMaxHealth());
	}
	SetActorTransform(HomeTransform);

	// The respawn teleport must be trusted by movement validation, otherwise the
	// next sample reads it as a cheat-teleport and yanks the pawn back here.
	if (MovementValidator)
	{
		MovementValidator->SnapToCurrentPosition();
		UE_LOG(LogTemp, Log, TEXT("[ClickMove] %s respawned - validator snapped to HomeTransform"),
			*GetName());
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		EnableInput(PC);
	}
}

void AOnsetPlayerCharacter::EnableCameraLag(bool bEnable)
{
	CameraBoom->bEnableCameraLag = bEnable;
}

void AOnsetPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AOnsetPlayerCharacter, Level, COND_None);
	DOREPLIFETIME_CONDITION(AOnsetPlayerCharacter, Experience, COND_None);
	DOREPLIFETIME_CONDITION(AOnsetPlayerCharacter, UnspentStatPoints, COND_None);
	DOREPLIFETIME_CONDITION(AOnsetPlayerCharacter, PrestigeLevel, COND_None);
}

void AOnsetPlayerCharacter::ApplyCharacterProgression(int32 InLevel, int32 InExperience, int32 InUnspentStatPoints, int32 InPrestigeLevel)
{
	if (!HasAuthority()) return;
	Level = FMath::Max(1, InLevel);
	Experience = FMath::Max(0, InExperience);
	UnspentStatPoints = FMath::Max(0, InUnspentStatPoints);
	PrestigeLevel = FMath::Max(0, InPrestigeLevel);

	// Apply prestige multiplier to CombatAttributes
	if (CombatAttributes && PrestigeLevel > 0)
	{
		const float PrestigeMultiplier = UOnsetEquipmentLibrary::GetPrestigeMultiplier(PrestigeLevel);
		CombatAttributes->SetPrestigeMultiplier(PrestigeMultiplier);
	}

	OnProgressionChanged.Broadcast(Level, Experience);
}

void AOnsetPlayerCharacter::SetPersistIdentity(const FString& Platform, const FString& PlatformID, int32 SlotIndex)
{
	PersistPlatform = Platform;
	PersistPlatformID = PlatformID;
	PersistSlotIndex = SlotIndex;
}

void AOnsetPlayerCharacter::GrantXPFromEnemy(int32 EnemyLevel, int32 XpReward)
{
	if (!HasAuthority()) return;

	const int32 PlayerLevelBefore = Level;
	const int32 BaseXP = UOnsetLevelingLibrary::GetEnemyBaseXP(EnemyLevel, XpReward);
	const float Multiplier = UOnsetLevelingLibrary::GetXPMultiplier(PlayerLevelBefore, EnemyLevel);
	const int32 Granted = UOnsetLevelingLibrary::GetGrantedXP(PlayerLevelBefore, EnemyLevel, XpReward);

	if (Granted > 0)
	{
		AddExperience(Granted);
		UE_LOG(LogTemp, Log, TEXT("XP: player Lv%d killed enemy Lv%d (XpReward=%d, base=%d x %.2f mult) -> +%d XP (now %d/%d)"),
			PlayerLevelBefore, EnemyLevel, XpReward, BaseXP, Multiplier, Granted, Experience, UOnsetLevelingLibrary::GetXPRequired(Level));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("XP: player Lv%d killed enemy Lv%d (XpReward=%d, base=%d x %.2f mult) -> 0 XP (grey mob)"),
			PlayerLevelBefore, EnemyLevel, XpReward, BaseXP, Multiplier);
	}
}

void AOnsetPlayerCharacter::GrantQuestXP(int32 Amount)
{
	if (!HasAuthority() || Amount <= 0)
	{
		return;
	}
	AddExperience(Amount);
	UE_LOG(LogTemp, Log, TEXT("XP: quest reward +%d XP (now %d/%d)"), Amount, Experience, UOnsetLevelingLibrary::GetXPRequired(Level));
}

void AOnsetPlayerCharacter::AddExperience(int32 Amount)
{
	if (Amount <= 0) return;

	const int32 LevelCap = UOnsetLevelingLibrary::GetLevelCap();
	if (Level >= LevelCap)
	{
		return; // XP capped at level cap; prestige requires quest completion
	}

	const int32 OldLevel = Level;
	Experience += Amount;

	while (Level < LevelCap && Experience >= UOnsetLevelingLibrary::GetXPRequired(Level))
	{
		Experience -= UOnsetLevelingLibrary::GetXPRequired(Level);
		Level += 1;
		UnspentStatPoints += UOnsetLevelingLibrary::GetStatPointsPerLevel();
	}

	// Clamp at LevelCap: any excess XP is discarded until prestige.
	if (Level >= LevelCap)
	{
		Level = LevelCap;
		Experience = 0;
	}

	if (Level > OldLevel)
	{
		UE_LOG(LogTemp, Log, TEXT("XP: LEVEL UP! Lv %d -> %d (+%d stat points, full heal)"),
			OldLevel, Level, UOnsetLevelingLibrary::GetStatPointsPerLevel());
	}

	// Level-up: full heal + event tag (TAG_Event_LevelUp, GAS event pattern).
	if (Level > OldLevel && AttributeSet)
	{
		AttributeSet->SetHealth(AttributeSet->GetMaxHealth());
	}
	if (Level > OldLevel && AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(TAG_Event_LevelUp);
		AbilitySystemComponent->RemoveLooseGameplayTag(TAG_Event_LevelUp);
	}

	OnProgressionChanged.Broadcast(Level, Experience);
	PersistProgression();
}

/** Called when the prestige quest is completed. Resets level/XP, increments PrestigeLevel, applies multiplier. */
void AOnsetPlayerCharacter::PrestigeUp()
{
	if (!HasAuthority()) return;

	const int32 LevelCap = UOnsetLevelingLibrary::GetLevelCap();
	if (Level < LevelCap)
	{
		UE_LOG(LogTemp, Warning, TEXT("PrestigeUp: player not at level cap (%d/%d)"), Level, LevelCap);
		return;
	}

	PrestigeLevel += 1;
	Level = 1;
	Experience = 0;
	UnspentStatPoints += UOnsetLevelingLibrary::GetStatPointsPerLevel();

	// Apply prestige multiplier to CombatAttributes
	if (CombatAttributes)
	{
		const float PrestigeMultiplier = UOnsetEquipmentLibrary::GetPrestigeMultiplier(PrestigeLevel);
		CombatAttributes->SetPrestigeMultiplier(PrestigeMultiplier);
	}

	UE_LOG(LogTemp, Log, TEXT("PRESTIGE: Player reached Prestige %d (multiplier %.2fx)"),
		PrestigeLevel, UOnsetEquipmentLibrary::GetPrestigeMultiplier(PrestigeLevel));

	OnPrestigeChanged.Broadcast(PrestigeLevel, 0);
	OnProgressionChanged.Broadcast(Level, Experience);
	PersistProgression();
}

void AOnsetPlayerCharacter::PersistProgression()
{
	// Use the identity stored on the pawn (SetPersistIdentity), not GetPlayerState():
	// under autoplay / continue-on-disconnect the AI controller possesses the pawn and
	// its PlayerState is null (AAIController owns no PlayerState), so GetPlayerState
	// would silently drop XP earned while the player is away.
	if (PersistSlotIndex < 0 || PersistPlatform.IsEmpty())
	{
		return;
	}

	UOnsetPlayerDataSubsystem* DataSubsystem = GetWorld()->GetSubsystem<UOnsetPlayerDataSubsystem>();
	if (!DataSubsystem) return;

	DataSubsystem->UpdateRuntimeProgression(PersistPlatform, PersistPlatformID, PersistSlotIndex, Level, Experience, UnspentStatPoints, PrestigeLevel);
}

int32 AOnsetPlayerCharacter::GetXPRequiredForNextLevel() const
{
	return UOnsetLevelingLibrary::GetXPRequired(Level);
}

float AOnsetPlayerCharacter::GetXPProgressPercent() const
{
	const int32 Required = UOnsetLevelingLibrary::GetXPRequired(Level);
	if (Required <= 0) return 0.0f;
	return FMath::Clamp(static_cast<float>(Experience) / static_cast<float>(Required), 0.0f, 1.0f);
}

void AOnsetPlayerCharacter::OnDeath(AActor* KillingActor)
{
	Super::OnDeath(KillingActor);
	DisableInput(nullptr);
	if (!HasAuthority()) return;
	GetWorldTimerManager().SetTimerForNextTick(this, &AOnsetPlayerCharacter::RespawnPlayer);
}
