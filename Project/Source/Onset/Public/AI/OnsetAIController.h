#pragma once

#include "CoreMinimal.h"
#include "DetourCrowdAIController.h"
#include "OnsetAIController.generated.h"

class UTargetingComponent;
class UStateTreeComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UAIProfile;

UCLASS()
class ONSET_API AOnsetAIController : public ADetourCrowdAIController
{
	GENERATED_BODY()

public:
	AOnsetAIController();

	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UStateTreeComponent> StateTreeComp;

	UPROPERTY(VisibleAnywhere, Category = "AI")
	TObjectPtr<UAIPerceptionComponent> PerceptionComp;

	UFUNCTION(BlueprintCallable, Category = "AI")
	void ApplyProfile(const UAIProfile* Profile);
	
	/** Stores the current target with validation. Set by context resolution in OnPrimaryInteraction. */          
	UPROPERTY()
	UTargetingComponent* TargetingComponent;
protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors);

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;
};
