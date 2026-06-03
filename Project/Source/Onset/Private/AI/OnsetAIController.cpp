#include "AI/OnsetAIController.h"

#include "AI/AIProfile.h"
#include "AI/OnsetStateTreeSchema.h"
#include "Components/StateTreeAIComponent.h"
#include "Enemy/OnsetEnemy.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Player/TargetingComponent.h"

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
		CrowdComp->SetCrowdSeparationWeight(2.0f);                                                                       
		CrowdComp->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::Good);
	}         
}

void AOnsetAIController::ApplyProfile(const UAIProfile* Profile)
{
	if (Profile == nullptr)
	{
		if (StateTreeComponent->IsRunning())
			StateTreeComponent->StopLogic(TEXT("Pooled"));
		StateTreeComponent->SetStateTree(nullptr);
		return;
	}
	if (Profile != nullptr)
	{
		StateTreeComponent->StopLogic(TEXT("Applying new profile"));
		StateTreeComponent->SetStateTree(Profile->StateTreeAsset);
	}

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
	Super::OnUnPossess();
	TargetingComponent = nullptr;
}

void AOnsetAIController::ResetForPool()
{
	UnPossess();
	bInUse = false;
	StateTreeComponent->StopLogic(TEXT("Reset for pool"));
	ApplyProfile(nullptr);                                                                                        
	SetActorHiddenInGame(true);                                                                                   
	SetActorTickEnabled(false);                                                                                   
	StateTreeComponent->SetComponentTickEnabled(false);                                                           
	DisableInput(nullptr);                                                                                        
	SetActorEnableCollision(false);
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

void AOnsetAIController::OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
	if (GetPawn() == nullptr) return;
	
	TArray<AActor*> PerceivedActors;
	PerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
	PerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Hearing::StaticClass(), PerceivedActors);
	
	AActor* BestTarget = nullptr;
	float BestDist = FLT_MAX;
	FVector MyLocation = GetPawn()->GetActorLocation();
	
	for (AActor* Actor : PerceivedActors)
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
	if (BestTarget == nullptr)
	{
		TargetingComponent->ClearTarget();
		return;
	}
	TargetingComponent->SetTarget(BestTarget);
		
}
