#include "Combat/AbilityTargetingLibrary.h"
#include "Core/TargetingComponent.h"
#include "GameFramework/Actor.h"

FOnsetTargetData UAbilityTargetingLibrary::GetTargetData(UTargetingComponent* TargetingComponent, AActor* SourceActor)
{
	FOnsetTargetData Data;

	if (!TargetingComponent) return Data;

	Data.TargetActor = TargetingComponent->GetTarget();
	if (!Data.TargetActor) return Data;

	Data.TargetLocation = Data.TargetActor->GetActorLocation();

	if (SourceActor)
	{
		Data.TargetDirection = (Data.TargetLocation - SourceActor->GetActorLocation()).GetSafeNormal();
	}

	return Data;
}
