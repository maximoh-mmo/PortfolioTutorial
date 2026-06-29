#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OnsetZoneGate.generated.h"

class UBoxComponent;

/** Trigger volume that saves player state and ServerTravels to the target zone. */
UCLASS()
class ONSET_API AOnsetZoneGate : public AActor
{
	GENERATED_BODY()

public:
	AOnsetZoneGate();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor);

	UPROPERTY(EditInstanceOnly, Category = "Zone Gate")
	FString TargetZone;

	UPROPERTY(EditInstanceOnly, Category = "Zone Gate")
	FString EntryPointName;

	UPROPERTY(VisibleAnywhere, Category = "Zone Gate")
	TObjectPtr<UBoxComponent> TriggerBox;
};
