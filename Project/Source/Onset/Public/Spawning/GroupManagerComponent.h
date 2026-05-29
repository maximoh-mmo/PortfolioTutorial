#pragma once

#include "CoreMinimal.h"
#include "Enemy/GroupData.h"
#include "Components/ActorComponent.h"
#include "GroupManagerComponent.generated.h"

class AOnsetEnemy;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ONSET_API UGroupManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGroupManagerComponent();

	UFUNCTION(BlueprintCallable)
	void RegisterMember(AOnsetEnemy* Enemy);

	UFUNCTION(BlueprintCallable)
	void UnregisterMember(AOnsetEnemy* Enemy);

	UFUNCTION(BlueprintCallable)
	FGroupData GetGroupData() const;

	TArray<AOnsetEnemy*> GetNearbyAllies(AOnsetEnemy* Source, float Radius) const;

	void NotifyMemberAttacked(AOnsetEnemy* Victim, AActor* Instigator);

private:
	UPROPERTY()
	TArray<AOnsetEnemy*> Members;
};
