// © 2026 Max Heinze. All rights reserved.
// All original code, gameplay systems, assets, and documentation included in this project are the intellectual
// property of the author unless otherwise stated. This project uses Unreal Engine.
// Unreal Engine and its logo are trademarks or registered trademarks of Epic Games, Inc.
// All third‑party assets remain the property of their respective creators.


#include "OnsetCharacter.h"

// Sets default values
AOnsetCharacter::AOnsetCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AOnsetCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AOnsetCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AOnsetCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

