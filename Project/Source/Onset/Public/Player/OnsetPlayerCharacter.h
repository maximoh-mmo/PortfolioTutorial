// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/OnsetBaseCharacter.h"
#include "OnsetPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;

/**
 * The player-controlled character. Handles camera setup; input and targeting
 * are owned by the PlayerController.
 */
UCLASS(Blueprintable)
class ONSET_API AOnsetPlayerCharacter : public AOnsetBaseCharacter
{
	GENERATED_BODY()

public:
	AOnsetPlayerCharacter();

protected:
	virtual void BeginPlay() override;

	// --- Camera ---

	/** Spring arm that provides the top-down view angle and collision push-back. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Camera attached to the spring arm socket. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
