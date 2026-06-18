// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/OnsetPlayerCharacter.h"

#include "TimerManager.h"
#include "Camera/CameraComponent.h"
#include "GAS/OnsetAttributeSet.h"
#include "GameFramework/SpringArmComponent.h"

AOnsetPlayerCharacter::AOnsetPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 2000.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 8.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
}

void AOnsetPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	HomeTransform = FTransform(
		GetActorRotation().GetEquivalentRotator(),
		GetActorLocation()
		);
}

void AOnsetPlayerCharacter::RespawnPlayer()
{
	if (AttributeSet)                                                                                           
	{                                                                                                           
		AttributeSet->SetHealth(AttributeSet->GetMaxHealth());                                                  
	}                                                                                                           
	SetActorTransform(HomeTransform);                                                                             
	if (APlayerController* PC = Cast<APlayerController>(GetController()))                                       
	{                                                                                                           
		EnableInput(PC);                                                                                        
	}                        
}

void AOnsetPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AOnsetPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AOnsetPlayerCharacter::OnDeath(AActor* KillingActor)
{
	Super::OnDeath(KillingActor);
	DisableInput(nullptr);
	GetWorldTimerManager().SetTimerForNextTick(this, &AOnsetPlayerCharacter::RespawnPlayer);
}
