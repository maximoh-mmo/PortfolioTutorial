// © 2026 Max Heinze. All rights reserved.
// All original code, gameplay systems, assets, and documentation included in this project are the intellectual
// property of the author unless otherwise stated. This project uses Unreal Engine.
// Unreal Engine and its logo are trademarks or registered trademarks of Epic Games, Inc.
// All third‑party assets remain the property of their respective creators.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OnsetCharacter.generated.h"

UCLASS()
class ONSET_API AOnsetCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AOnsetCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
