// © 2026 Max Heinze. All rights reserved.
// All original code, gameplay systems, assets, and documentation included in this project are the intellectual
// property of the author unless otherwise stated. This project uses Unreal Engine.
// Unreal Engine and its logo are trademarks or registered trademarks of Epic Games, Inc.
// All third‑party assets remain the property of their respective creators.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Onset/Combat/TargetingComponent.h"
#include "OnsetPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class ONSET_API AOnsetPlayerController : public APlayerController
{
	GENERATED_BODY()
	
	public:
	AOnsetPlayerController();
	
	virtual void SetupInputComponent() override;
	
	UFUNCTION()	
	void OnClick();
	
	protected:
	virtual void BeginPlay() override;

	private:
	UPROPERTY()
	UTargetingComponent* TargetingComponent;
};
