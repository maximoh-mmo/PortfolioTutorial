#include "Enemy/OnsetEnemy.h"

#include "Enemy/GroupComponent.h"
#include "AI/OnsetAIController.h"

AOnsetEnemy::AOnsetEnemy()
{
	this->Tags.Add(FName("Enemy"));
	AIControllerClass = AOnsetAIController::StaticClass();
	GroupComp = CreateDefaultSubobject<UGroupComponent>(TEXT("GroupComp"));
}
