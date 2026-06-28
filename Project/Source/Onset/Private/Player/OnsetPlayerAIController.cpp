// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/OnsetPlayerAIController.h"
#include "StateTree.h"
#include "Components/StateTreeAIComponent.h"
#include "Core/TargetingComponent.h"


// Sets default values
AOnsetPlayerAIController::AOnsetPlayerAIController()
{
	bStartAILogicOnPossess = true;
	StateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeComponent"));
	StateTreeComponent->SetComponentTickEnabled(false);
	StateTree = LoadObject<UStateTree>(nullptr, TEXT("/Game/AI/PlayerAutoCombat.PlayerAutoCombat"));
}

void AOnsetPlayerAIController::StartStateTree()
{
	if (!StateTree)
	{
		StateTree = LoadObject<UStateTree>(nullptr, TEXT("/Game/AI/PlayerAutoCombat.PlayerAutoCombat"));
	}
	if (StateTree)
	{
#if WITH_EDITOR
	
		if (!StateTree->IsReadyToRun())
		{
			StateTree->MarkAsModified(false);
			StateTree->CompileIfChanged();
		}
	
#endif
	
		if (StateTree->IsReadyToRun())
		{
			StateTreeComponent->SetStateTree(StateTree);
			StateTreeComponent->SetComponentTickEnabled(true);
			StateTreeComponent->StartLogic();
		}
	}
	else
	{
		UE_LOG(LogController, Error, TEXT("StateTree not ready. Open /Game/AI/PlayerAutoCombat and save it."));
	}
}

void AOnsetPlayerAIController::StopStateTree()
{
	if (StateTreeComponent)
	{
		StateTreeComponent->StopLogic(TEXT("PlayerOverride"));
		StateTreeComponent->SetComponentTickEnabled(false);
	}
}

void AOnsetPlayerAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (!HasAuthority()) return;
	TargetingComponent = InPawn->FindComponentByClass<UTargetingComponent>();
	StartStateTree();
	UE_LOG(LogActor, Warning, TEXT("AOnsetPlayerAIController: Possessed player"));
}

void AOnsetPlayerAIController::OnUnPossess()
{
	StateTreeComponent->StopLogic(TEXT("PlayerOverride"));
	StateTreeComponent->SetComponentTickEnabled(false);
	UE_LOG(LogActor, Warning, TEXT("AOnsetPlayerAIController: Returned control to player"));

	Super::OnUnPossess();
}