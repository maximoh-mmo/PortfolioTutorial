// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/OnsetBaseCharacter.h"

AOnsetBaseCharacter::AOnsetBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AOnsetBaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void AOnsetBaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
