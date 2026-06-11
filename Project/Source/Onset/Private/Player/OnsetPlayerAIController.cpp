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
	// Load shared auto-combat tree
	static ConstructorHelpers::FObjectFinder<UStateTree> TreeFinder(
		TEXT("/Game/Game/AI/PlayerAutoCombat.PlayerAutoCombat"));
	if (TreeFinder.Succeeded())                                                                                          
	{                                                                                                           
		StateTreeComponent->SetStateTree(TreeFinder.Object);                                                    
	}       
	StateTreeComponent->SetComponentTickEnabled(false);
}

void AOnsetPlayerAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	TargetingComponent = InPawn->FindComponentByClass<UTargetingComponent>();
	StateTreeComponent->SetComponentTickEnabled(true);
	StateTreeComponent->StartLogic();
}

void AOnsetPlayerAIController::OnUnPossess()
{
	StateTreeComponent->StopLogic(TEXT("PlayerOverride"));
	StateTreeComponent->SetComponentTickEnabled(false);
	Super::OnUnPossess();
}