#include "AI/OnsetAIController.h"

#include "DrawDebugHelpers.h"
#include "Enemy/Profile/AIProfile.h"

#include "Components/StateTreeAIComponent.h"
#include "Enemy/OnsetEnemy.h"
#include "Engine/Engine.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Core/TargetingComponent.h"
#include "Subsystem/OnsetThreatSubsystem.h" // Include for UOnsetThreatSubsystem

AOnsetAIController::AOnsetAIController()
{
	bStartAILogicOnPossess = true;
	
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeComp"));
	StateTreeComponent->SetStateTree(nullptr);
	StateTreeComponent->SetComponentTickEnabled(true);
	
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	PerceptionComponent->ConfigureSense(*SightConfig);
	PerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	PerceptionComponent->ConfigureSense(*HearingConfig);
	
	PerceptionComponent->OnPerceptionUpdated.AddDynamic(this, &AOnsetAIController::OnPerceptionUpdated);
	
	UCrowdFollowingComponent* CrowdComp = Cast<UCrowdFollowingComponent>(                                           
		 GetPathFollowingComponent());                                                                               
	if (CrowdComp)                                                                                                  
	{                                                                                                               
		CrowdComp->SetCrowdSeparation(true);                                                                        
		CrowdComp->SetCrowdSeparationWeight(8.0f);                                                                       
		CrowdComp->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Good);
	}         
}

void AOnsetAIController::ApplyAIProfile(UAIProfile* Profile)
{
	if (Profile == nullptr)
	{
		if (StateTreeComponent->IsRunning())
			StateTreeComponent->StopLogic(TEXT("Pooled"));
		StateTreeComponent->SetStateTree(nullptr);
		AIProfile = nullptr;
		return;
	}
	StateTreeComponent->StopLogic(TEXT("Applying new profile"));
	StateTreeComponent->SetStateTree(Profile->StateTreeAsset);
	AIProfile = Profile;
}
void AOnsetAIController::ApplyPerceptionProfile(const UPerceptionProfile* Profile)
{
	if (SightConfig)
	{
		SightConfig->SightRadius = Profile->SightRange;
		SightConfig->LoseSightRadius = Profile->SightRange * 1.5f;
		SightConfig->PeripheralVisionAngleDegrees = Profile->SightAngle;
		SightConfig->DetectionByAffiliation.bDetectEnemies = true;
		SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
		PerceptionComponent->ConfigureSense(*SightConfig);
	}

	if (HearingConfig)
	{
		HearingConfig->HearingRange = Profile->HearingRange;
		HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
		HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
		HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
		PerceptionComponent->ConfigureSense(*HearingConfig);
	}

	CachedSightRange = Profile->SightRange;
	CachedHearingRange = Profile->HearingRange;
}

void AOnsetAIController::OnPossess(APawn* InPawn)
{
	bInUse = true;
	Super::OnPossess(InPawn);
	StateTreeComponent->StartLogic();
	TargetingComponent = GetPawn()->FindComponentByClass<UTargetingComponent>();
}

void AOnsetAIController::OnUnPossess()
{
	bInUse = false;
	StateTreeComponent->StopLogic(TEXT("Unpossessed"));
	ApplyAIProfile(nullptr);                                                                                        
	SetActorHiddenInGame(true);                                                                                   
	SetActorTickEnabled(false);                                                                                   
	StateTreeComponent->SetComponentTickEnabled(false);                                                           
	DisableInput(nullptr);                                                                                        
	SetActorEnableCollision(false);
	Super::OnUnPossess();
	TargetingComponent = nullptr;
	HeardNoiseLocation = FVector::ZeroVector;                                                               
	HeardNoiseInstigator = nullptr;                                                                    
	bHasPendingNoise = false;                                                                                  
	LastNoiseHeardTime = 0.0f;       
}

void AOnsetAIController::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		StateTreeComponent->StopLogic(TEXT("Client - No Authority"));
		StateTreeComponent->SetStateTree(nullptr);
	}
}

void AOnsetAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#if WITH_EDITOR
	if (GetPawn() && StateTreeComponent && StateTreeComponent->IsRunning())
	{
		const TArray<FName> ActiveStates = StateTreeComponent->GetActiveStateNames();
		if (ActiveStates.Num() > 0)
		{
			FString DebugStr;
			for (const FName& Name : ActiveStates)
			{
				if (!DebugStr.IsEmpty()) DebugStr += TEXT(" > ");
				DebugStr += Name.ToString();
			}
			DrawDebugString(GetWorld(), GetPawn()->GetActorLocation() + FVector(0, 0, 120),
				DebugStr, nullptr, FColor::Cyan, 0.0f, false);
		}
	}
#endif

	if (++LodTickCounter >= 30)
	{
		LodTickCounter = 0;
		UpdateLodTier();
	}
}

void AOnsetAIController::UpdateLodTier()
{
	if (!GetPawn() || !HasAuthority()) return;

	float NearestDistSq = FLT_MAX;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (APawn* PlayerPawn = PC->GetPawn())
			{
				const float DistSq = FVector::DistSquared(GetPawn()->GetActorLocation(), PlayerPawn->GetActorLocation());
				if (DistSq < NearestDistSq) NearestDistSq = DistSq;
			}
		}
	}

	const float NearestDist = FMath::Sqrt(NearestDistSq);

	if (NearestDist < CachedSightRange)
	{
		SetActorTickInterval(0.0f);
		StateTreeComponent->SetComponentTickEnabled(true);
	}
	else if (NearestDist < CachedHearingRange)
	{
		SetActorTickInterval(0.2f);
		StateTreeComponent->SetComponentTickEnabled(true);
	}
	else
	{
		SetActorTickInterval(0.5f);
		StateTreeComponent->SetComponentTickEnabled(false);
	}
}

void AOnsetAIController::OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
	if (GetPawn() == nullptr) return;
	
	// --- Sight: set targeting component and add threat ---
	TArray<AActor*> SeenActors;
	PerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), SeenActors);
	AActor* BestTarget = nullptr;
	float BestDist = FLT_MAX;
	FVector MyLocation = GetPawn()->GetActorLocation();
	
	for (AActor* Actor : SeenActors)
	{
		if (Actor == nullptr) continue;
		
		if (Actor->ActorHasTag(FName("Player")) == GetPawn()->ActorHasTag(FName("Player")))
		{
			continue;
		}
		float Dist = FVector::DistSquared(MyLocation, Actor->GetActorLocation());
		if (Dist < BestDist)	
		{
			BestDist = Dist;
			BestTarget = Actor;
		}
	}
	if (BestTarget)
	{
		TargetingComponent->SetTarget(BestTarget);

		// Add threat when a player is visually perceived
		if (UOnsetThreatSubsystem* ThreatSub = GetWorld()->GetSubsystem<UOnsetThreatSubsystem>())
		{
			AOnsetEnemy* SelfEnemy = Cast<AOnsetEnemy>(GetPawn());
			AOnsetBaseCharacter* PlayerChar = Cast<AOnsetBaseCharacter>(BestTarget);
			if (SelfEnemy && PlayerChar)
			{
				// Only add threat if the player is not already engaged with this enemy
				// This prevents spamming threat if the player is already the target
				if (!ThreatSub->IsEnemyEngagedWithPlayer(SelfEnemy, PlayerChar))
				{
					ThreatSub->AddThreat(PlayerChar, SelfEnemy, 1.0f); // Add a base threat of 1.0
				}
			}
		}
	}
	else if (SeenActors.Num() == 0)
	{
		TargetingComponent->ClearTarget();
	}
	
	// --- Hearing: store closest noise ---
	TArray<AActor*> HeardActors;
	PerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Hearing::StaticClass(), HeardActors);
	
	if (HeardActors.Num() > 0)
	{
		AActor* ClosestHeard = nullptr;                                                                         
             float ClosestDistSq = FLT_MAX;                                                                            
                                                                                                                     
             for (AActor* Actor : HeardActors)                                                                       
             {                                                                                                       
                 if (Actor == nullptr) continue;                                                                     
                 float DistSq = FVector::DistSquared(MyLocation, Actor->GetActorLocation());                           
                 if (DistSq < ClosestDistSq)                                                                             
                 {                                                                                                   
                     ClosestDistSq = DistSq;                                                                             
                     ClosestHeard = Actor;                                                                           
                 }                                                                                                   
             }                                                                                                       
                                                                                                                     
             if (ClosestHeard)                                                                                       
             {                                                                                                       
                 HeardNoiseLocation = ClosestHeard->GetActorLocation();                                              
                 HeardNoiseInstigator = ClosestHeard;                                                                
                 bHasPendingNoise = true;                                                                            
                 LastNoiseHeardTime = GetWorld()->GetTimeSeconds();                                                       
             }           
	}
	else
	{
		bHasPendingNoise = false;	
	}
}