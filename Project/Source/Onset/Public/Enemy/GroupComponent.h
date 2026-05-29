#pragma once

#include "CoreMinimal.h"
#include "Enemy/GroupData.h"
#include "Components/ActorComponent.h"
#include "GroupComponent.generated.h"

class UGroupManagerComponent;

UCLASS(meta=(BlueprintSpawnableComponent))
class ONSET_API UGroupComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UGroupComponent();

	void RegisterWithGroup(UGroupManagerComponent* Manager);
	void ClearGroup();

	UFUNCTION(BlueprintCallable)
	void UnregisterFromGroup();

	UFUNCTION(BlueprintCallable)
	FGroupData GetGroupData() const;

	bool IsInGroup() const { return GroupManager.IsValid(); }

	UGroupManagerComponent* GetGroupManager() const { return GroupManager.Get(); }

private:
	UPROPERTY()
	TWeakObjectPtr<UGroupManagerComponent> GroupManager;
};
