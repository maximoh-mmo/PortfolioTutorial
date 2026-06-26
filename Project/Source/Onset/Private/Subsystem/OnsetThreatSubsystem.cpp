// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystem/OnsetThreatSubsystem.h"
#include "Enemy/OnsetEnemy.h"

DEFINE_LOG_CATEGORY_STATIC(LogOnsetThreat, Log, All);

void UOnsetThreatSubsystem::AddThreat(AOnsetBaseCharacter* PlayerCharacter, AOnsetEnemy* Enemy, float ThreatAmount)
{
	if (!PlayerCharacter || !Enemy) return;
	TMap<TWeakObjectPtr<AOnsetBaseCharacter>, float>& EnemyThreats = ThreatTable.FindOrAdd(TWeakObjectPtr(Enemy));
	float& CurrentThreat = EnemyThreats.FindOrAdd(TWeakObjectPtr(PlayerCharacter));
	CurrentThreat = FMath::Max(0.0f, CurrentThreat + ThreatAmount);

	UE_LOG(LogOnsetThreat, Log, TEXT("Threat: %s %+.1f -> %.1f (%s)"),
		*Enemy->GetName(), ThreatAmount, CurrentThreat, *PlayerCharacter->GetName());
	
	if (CurrentThreat <= 0.0f)
	{
		EnemyThreats.Remove(TWeakObjectPtr(PlayerCharacter));
		UE_LOG(LogOnsetThreat, Log, TEXT("Threat: %s -- removed %s (zero)"),
			*Enemy->GetName(), *PlayerCharacter->GetName());
		if (EnemyThreats.IsEmpty())
		{
			RemoveEnemy(Enemy);
		}
	}
}

void UOnsetThreatSubsystem::RemovePlayer(const AOnsetBaseCharacter* PlayerCharacter)
{
	if (!PlayerCharacter) return;
	UE_LOG(LogOnsetThreat, Log, TEXT("RemovePlayer: %s"), *PlayerCharacter->GetName());
	TArray<TWeakObjectPtr<AOnsetEnemy>> Enemies;
	ThreatTable.GetKeys(Enemies);
	const TWeakObjectPtr WeakPlayer(PlayerCharacter);
	for (TWeakObjectPtr<AOnsetEnemy>& OnsetEnemy : Enemies)
	{
		if (auto* EnemyThreats = ThreatTable.Find(OnsetEnemy))                                                  
		{                                                                                                       
			EnemyThreats->Remove(WeakPlayer);                                                                   
			if (EnemyThreats->IsEmpty())                                                                        
				ThreatTable.Remove(OnsetEnemy);                                                                 
		} 
	}
}

int32 UOnsetThreatSubsystem::GetTargetRank(AOnsetEnemy* Enemy, AOnsetBaseCharacter* PlayerCharacter)
{
	const auto EnemyThreatList = ThreatTable.Find(Enemy);
	if (!EnemyThreatList) return -1;
	
	TArray<TPair<AOnsetBaseCharacter*, float>> Sorted;
	Sorted.Reserve(EnemyThreatList->Num());
	for (const auto& Pair : *EnemyThreatList)
		Sorted.Emplace(Pair.Key.Get(), Pair.Value);
	
	Sorted.Sort([](const auto& A, const auto& B) { return A.Value > B.Value; });
	for (int i = 0; i < Sorted.Num(); ++i)
	{
		if (Sorted[i].Key == PlayerCharacter) return i;
	}	
	return -1;
}

void UOnsetThreatSubsystem::RegisterEngaged(AOnsetBaseCharacter* PlayerCharacter, AOnsetEnemy* Enemy)
{
	if (!PlayerCharacter || !Enemy) return;
	UE_LOG(LogOnsetThreat, Log, TEXT("Engage: %s -> %s"), *Enemy->GetName(), *PlayerCharacter->GetName());
	const TWeakObjectPtr WeakEnemy(Enemy);
	auto& EnemyList = EngagementTable.FindOrAdd(PlayerCharacter);
	if (!EnemyList.Find(WeakEnemy))
	{
		EnemyList.Add(WeakEnemy);
	}
}

void UOnsetThreatSubsystem::UnregisterEngaged(AOnsetEnemy* Enemy)
{
	if (!Enemy) return;
	UE_LOG(LogOnsetThreat, Log, TEXT("Disengage: %s"), *Enemy->GetName());
	const TWeakObjectPtr WeakEnemy(Enemy);
	for (auto& Player : EngagementTable)
	{
		if (Player.Value.Contains(WeakEnemy))
		{
			Player.Value.Remove(WeakEnemy);
		}
	}
}

int32 UOnsetThreatSubsystem::GetEngagedCount(AOnsetBaseCharacter* PlayerCharacter)
{
	if (const auto* EnemyList = EngagementTable.Find(PlayerCharacter))                                          
		return EnemyList->Num();                                                                                
	return 0;     
}

int32 UOnsetThreatSubsystem::GetEngagedIndex(AOnsetEnemy* Enemy, AOnsetBaseCharacter* PlayerCharacter)
{
	const auto* EnemyList = EngagementTable.Find(PlayerCharacter);                                              
	if (!EnemyList) return -1;                                                                                  
	const TWeakObjectPtr WeakEnemy(Enemy);                                                                      
	for (int i = 0; i < EnemyList->Num(); ++i)                                                                  
	{                                                                                                           
		if ((*EnemyList)[i] == WeakEnemy)                                                                       
			return i;                                                                                           
	}                                                                                                           
	return -1;     
}

void UOnsetThreatSubsystem::SwitchTarget(AOnsetEnemy* Enemy, AOnsetBaseCharacter* NewPlayer)
{
	if (!Enemy || !NewPlayer) return;
	UE_LOG(LogOnsetThreat, Log, TEXT("SwitchTarget: %s -> %s"), *Enemy->GetName(), *NewPlayer->GetName());
	UnregisterEngaged(Enemy);
	RegisterEngaged(NewPlayer, Enemy);
}

void UOnsetThreatSubsystem::RemoveEnemy(AOnsetEnemy* Enemy)
{
	if (!Enemy) return;
	UE_LOG(LogOnsetThreat, Log, TEXT("RemoveEnemy: %s"), *Enemy->GetName());
	ThreatTable.Remove(Enemy);
	UnregisterEngaged(Enemy);	
}

APawn* UOnsetThreatSubsystem::GetBestTarget(AOnsetEnemy* Enemy, float AttackRange, float ChaseRange)
{
	const auto* ThreatList = ThreatTable.Find(Enemy);
	if (!ThreatList || ThreatList->IsEmpty()) return nullptr;

	AOnsetBaseCharacter* Best = nullptr;
	float BestScore = -FLT_MAX;
	const FVector EnemyLoc = Enemy->GetActorLocation();
	const float AttackRangeSq = FMath::Square(AttackRange);
	const float ChaseRangeSq = FMath::Square(ChaseRange);

	for (const auto& Pair : *ThreatList)
	{
		AOnsetBaseCharacter* Player = Pair.Key.Get();
		if (!Player) continue;

		const float DistSq = FVector::DistSquared(EnemyLoc, Player->GetActorLocation());
		const float ThreatVal = Pair.Value;

		const float DistWeight = (DistSq <= AttackRangeSq) ? 1.0f
		                     : (DistSq <= ChaseRangeSq)  ? 0.5f
		                     : 0.1f;

		const float Score = ThreatVal * DistWeight;
		if (Score > BestScore)
		{
			BestScore = Score;
			Best = Player;
		}
	}

	return Best;
}

APawn* UOnsetThreatSubsystem::GetPrimaryTarget(AOnsetEnemy* Enemy)
{
	if (!Enemy) return nullptr;
	if (const auto EnemyThreatList = ThreatTable.Find(Enemy))
	{
		TWeakObjectPtr<AOnsetBaseCharacter> HighestThreat = nullptr;
		float BestThreat = -1.0f;
		for (const auto& Pair : *EnemyThreatList)
		{
			if (Pair.Value > BestThreat)
			{
				BestThreat = Pair.Value;
				HighestThreat = Pair.Key;
			}
		}
		return HighestThreat.IsValid() ? HighestThreat.Get() : nullptr;
	}
	return nullptr;
}

APawn* UOnsetThreatSubsystem::GetNthTarget(int32 Rank, AOnsetEnemy* Enemy)
{
	const auto EnemyThreatList = ThreatTable.Find(Enemy);
	if (!EnemyThreatList || Rank < 0) return nullptr;
	
	TArray<TPair<AOnsetBaseCharacter*, float>> Sorted;
	Sorted.Reserve(EnemyThreatList->Num());
	for (const auto& Pair : *EnemyThreatList)
		Sorted.Emplace(Pair.Key.Get(), Pair.Value);
	
	if (Rank >= Sorted.Num()) return nullptr;
	Sorted.Sort([](const auto& A, const auto& B) { return A.Value > B.Value; });
	return Sorted[Rank].Key;
}

int32 UOnsetThreatSubsystem::GetTargetCount(AOnsetEnemy* Enemy)
{
	if (!Enemy) return 0;
	const auto EnemyThreats = ThreatTable.Find(Enemy);
	if (EnemyThreats) return EnemyThreats->Num();
	return 0;
}

void UOnsetThreatSubsystem::ClearAll()
{
	ThreatTable = {};
	EngagementTable = {};
}