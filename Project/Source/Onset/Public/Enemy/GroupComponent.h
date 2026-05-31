#pragma once

#include "CoreMinimal.h"
#include "Enemy/GroupData.h"
#include "Components/ActorComponent.h"
#include "GroupComponent.generated.h"

class UGroupManagerComponent;

/** Bridge actor component on each enemy referencing the owning spawner's UGroupManagerComponent. */
UCLASS(meta=(BlueprintSpawnableComponent))
class ONSET_API UGroupComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UGroupComponent();

	/** Register this enemy with the given group manager. */
	void RegisterWithGroup(UGroupManagerComponent* Manager);

	/** Clear group reference without unregistering. */
	void ClearGroup();

	/** Unregister this enemy from its group. */
	UFUNCTION(BlueprintCallable)
	void UnregisterFromGroup();

	/** Returns the current group metrics (center + alive count). */
	UFUNCTION(BlueprintCallable)
	FGroupData GetGroupData() const;

	/** Whether this enemy is currently part of a group. */
	bool IsInGroup() const { return GroupManager.IsValid(); }

	/** Returns the owning group manager, or null. */
	UGroupManagerComponent* GetGroupManager() const { return GroupManager.Get(); }

private:
	/** Weak ref to the group manager to avoid circular ownership. */
	UPROPERTY()
	TWeakObjectPtr<UGroupManagerComponent> GroupManager;
};
