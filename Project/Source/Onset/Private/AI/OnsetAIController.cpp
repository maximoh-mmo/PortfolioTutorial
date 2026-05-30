#include "AI/OnsetAIController.h"

#include "AI/AIProfile.h"
#include "Enemy/OnsetEnemy.h"
#include "Components/StateTreeComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Player/TargetingComponent.h"

AOnsetAIController::AOnsetAIController()
{
	StateTreeComp = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTreeComp"));
	PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));
	TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("TargetingComp"));

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	PerceptionComp->ConfigureSense(*SightConfig);
	PerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	PerceptionComp->ConfigureSense(*HearingConfig);
}

void AOnsetAIController::ApplyProfile(const UAIProfile* Profile)
{
	if (!Profile)
	{
		StateTreeComp->SetStateTree(nullptr);
		return;
	}

	StateTreeComp->SetStateTree(Profile->StateTreeAsset);

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

	PerceptionComp->OnPerceptionUpdated.AddDynamic(this, &AOnsetAIController::OnPerceptionUpdated);
}

void AOnsetAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (const auto* Enemy = Cast<AOnsetEnemy>(InPawn))
	{
		ApplyProfile(Enemy->Profile);
	}
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
}