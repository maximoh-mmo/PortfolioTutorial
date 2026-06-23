// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "OnsetPlayerAIController.generated.h"

class UStateTree;
class UStateTreeAIComponent;
class UTargetingComponent;

UCLASS()
class ONSET_API AOnsetPlayerAIController : public AAIController
{
	GENERATED_BODY()

public:
	AOnsetPlayerAIController();
	
	/** StateTree execution component. Started on possess, stopped on unpossess. */
	UPROPERTY(VisibleAnywhere, Category = "Auto Combat")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;
	
	UPROPERTY(EditAnywhere, Category = "Auto Combat")
	float MaxDistance = 0.5f;
	
	UPROPERTY(EditAnywhere, Category = "Auto Combat")
	float Aggression = 0.5f;

	/** Stores the current targeting component via OnPossess, clears on UnPossess. */          
	UTargetingComponent* GetTargetingComponent() const { return TargetingComponent; }
	
	UPROPERTY(EditDefaultsOnly, Category = "Auto Combat Ranges")
	float MinLeash = 800.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Auto Combat Ranges")
	float MaxLeash = 8000.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Auto Combat Ranges")
	float MinAcquire = 500.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Auto Combat Ranges")
	float MaxAcquire = 5000.0f;
	
protected:
	void StartStateTree();
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	
private:
	UPROPERTY()
	TObjectPtr<UTargetingComponent> TargetingComponent;
	
	UPROPERTY()
	TObjectPtr<UStateTree> StateTree;
};
