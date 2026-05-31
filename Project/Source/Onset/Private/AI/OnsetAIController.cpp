#include "AI/OnsetAIController.h"

#include "AI/AIProfile.h"
#include "AI/OnsetStateTreeSchema.h"
#include "Components/StateTreeAIComponent.h"
#include "Enemy/OnsetEnemy.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Player/TargetingComponent.h"

AOnsetAIController::AOnsetAIController()
{
	bStartAILogicOnPossess = false;
	StateTreeComp = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeComp"));
	StateTreeComp->SetStateTree(nullptr);
	StateTreeComp->SetComponentTickEnabled(true);
	
	PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));
	TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("TargetingComp"));

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	PerceptionComp->ConfigureSense(*SightConfig);
	PerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	PerceptionComp->ConfigureSense(*HearingConfig);
	
	PerceptionComp->OnPerceptionUpdated.AddDynamic(this, &AOnsetAIController::OnPerceptionUpdated);

}

void AOnsetAIController::ApplyProfile(const UAIProfile* Profile)
{
	if (Profile == nullptr)
	{
		if (StateTreeComp->IsRunning())
			StateTreeComp->StopLogic(TEXT("Pooled"));
		StateTreeComp->SetStateTree(nullptr);
		return;
	}
	if (Profile != nullptr)
	{
		StateTreeComp->StopLogic(TEXT("Applying new profile"));
		StateTreeComp->SetStateTree(Profile->StateTreeAsset);
	}

	if (SightConfig)
	{
		SightConfig->SightRadius = Profile->SightRange;
		SightConfig->LoseSightRadius = Profile->SightRange * 1.5f;
		SightConfig->PeripheralVisionAngleDegrees = Profile->SightAngle;
		SightConfig->DetectionByAffiliation.bDetectEnemies = true;
		SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
		SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
		PerceptionComp->ConfigureSense(*SightConfig);
	}

	if (HearingConfig)
	{
		HearingConfig->HearingRange = Profile->HearingRange;
		HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
		HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
		HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
		PerceptionComp->ConfigureSense(*HearingConfig);
	}
}

void AOnsetAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	StateTreeComp->StartLogic();
}

void AOnsetAIController::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		StateTreeComp->StopLogic(TEXT("Client - No Authority"));
		StateTreeComp->SetStateTree(nullptr);
	}
}

void AOnsetAIController::OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
	if (GetPawn() == nullptr) return;
	
	TArray<AActor*> PerceivedActors;
	PerceptionComp->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
	PerceptionComp->GetCurrentlyPerceivedActors(UAISense_Hearing::StaticClass(), PerceivedActors);
	
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
