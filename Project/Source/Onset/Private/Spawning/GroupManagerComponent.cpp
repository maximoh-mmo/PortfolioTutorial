#include "Spawning/GroupManagerComponent.h"
#include "Enemy/GroupComponent.h"
#include "Enemy/OnsetEnemy.h"

UGroupManagerComponent::UGroupManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGroupManagerComponent::RegisterMember(AOnsetEnemy* Enemy)
{
	if (!Enemy || Enemy->IsPendingKillPending() || Members.Contains(Enemy)) return;

	Members.Add(Enemy);
	if (auto* Component = Enemy->FindComponentByClass<UGroupComponent>())
	{
		Component->RegisterWithGroup(this);
	}
}

void UGroupManagerComponent::UnregisterMember(AOnsetEnemy* Enemy)
{
	if (!Enemy || Enemy->IsPendingKillPending() || !Members.Contains(Enemy)) return;
	Members.Remove(Enemy);
	if (auto* Component = Enemy->FindComponentByClass<UGroupComponent>())
	{
		Component->ClearGroup();
	}
}

FGroupData UGroupManagerComponent::GetGroupData() const
{
	FGroupData Data;

	if (Members.Num() == 0) return Data;

	FVector AccumulatedLocation = FVector::ZeroVector;
	int32 ValidCount = 0;
	for (const AOnsetEnemy* Enemy : Members)
	{
		if (Enemy && !Enemy->IsPendingKillPending() && !Enemy->IsHidden())
		{
			AccumulatedLocation += Enemy->GetActorLocation();
			ValidCount++;
		}
	}
	if (ValidCount > 0)
	{
		Data.Center = AccumulatedLocation / ValidCount;
	}
	Data.AliveCount = ValidCount;
	return Data;
}

TArray<AOnsetEnemy*> UGroupManagerComponent::GetNearbyAllies(AOnsetEnemy* Source, float Radius) const
{
	TArray<AOnsetEnemy*> Result;

	if (!Source || Source->IsPendingKillPending()) return Result;

	const FVector SourceLocation = Source->GetActorLocation();

	for (AOnsetEnemy* Enemy : Members)
	{
		if (Enemy && !Enemy->IsPendingKillPending() && !Enemy->IsHidden())
		{
			if (FVector::Dist(Enemy->GetActorLocation(), SourceLocation) <= Radius)
			{
				Result.Add(Enemy);
			}
		}
	}
	return Result;
}