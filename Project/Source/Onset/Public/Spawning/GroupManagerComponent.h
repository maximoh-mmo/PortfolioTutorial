// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enemy/GroupData.h"
#include "Components/ActorComponent.h"
#include "GroupManagerComponent.generated.h"

class AOnsetEnemy;

/** Manages a group of enemies on the spawner. Tracks membership and provides group metrics. */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ONSET_API UGroupManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGroupManagerComponent();

	/** Register an enemy into the group. Called by the spawner after spawning. */
	UFUNCTION(BlueprintCallable)
	void RegisterMember(AOnsetEnemy* Enemy);

	/** Remove an enemy from the group. Called on pool return or death. */
	UFUNCTION(BlueprintCallable)
	void UnregisterMember(AOnsetEnemy* Enemy);

	/** Compute center + alive count of all currently-registered members. */
	UFUNCTION(BlueprintCallable)
	FGroupData GetGroupData() const;

private:
	/** Returns alive group members within a given radius of a source enemy. Used by roam/flee decisions. */
	TArray<AOnsetEnemy*> GetNearbyAllies(AOnsetEnemy* Source, float Radius) const;

	/** All currently-registered group members. */
	UPROPERTY()
	TArray<AOnsetEnemy*> Members;
};
