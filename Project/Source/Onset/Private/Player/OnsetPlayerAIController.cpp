// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/OnsetPlayerAIController.h"

#include "StateTree.h"
#include "Components/StateTreeAIComponent.h"
#include "Player/TargetingComponent.h"
#include "UObject/ConstructorHelpers.h"


// Sets default values
AOnsetPlayerAIController::AOnsetPlayerAIController()
{
	bStartAILogicOnPossess = true;
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeComponent"));
	StateTreeComponent->SetComponentTickEnabled(false);
	StateTree = LoadObject<UStateTree>(nullptr, TEXT("/Game/AI/PlayerAutoCombat.PlayerAutoCombat"));
}

void AOnsetPlayerAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	TargetingComponent = InPawn->FindComponentByClass<UTargetingComponent>();
	if (StateTree)
	{
		StateTreeComponent->SetStateTree(StateTree);
	}
	else
	{		
		StateTree = LoadObject<UStateTree>(nullptr, TEXT("/Game/AI/PlayerAutoCombat.PlayerAutoCombat"));
		if (StateTree)
		{
			StateTreeComponent->SetStateTree(StateTree);
		}
		else
		{
			UE_LOG(LogController, Warning, TEXT("AOnsetPlayerAIController::OnPossess: No tree component"));
		}
	}
	StateTreeComponent->SetComponentTickEnabled(true);
	StateTreeComponent->StartLogic();
	UE_LOG(LogActor, Warning, TEXT("AOnsetPlayerAIController: Possessed player"));
}

void AOnsetPlayerAIController::OnUnPossess()
{
	StateTreeComponent->StopLogic(TEXT("PlayerOverride"));
	StateTreeComponent->SetComponentTickEnabled(false);
	UE_LOG(LogActor, Warning, TEXT("AOnsetPlayerAIController: Returned control to player"));

	Super::OnUnPossess();
}