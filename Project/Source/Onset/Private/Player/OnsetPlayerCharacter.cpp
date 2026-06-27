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
	// lock rotation setting absolute to ignore pawn rotation.
	CameraBoom->bInheritYaw = false;  // camera stays at fixed world yaw
	CameraBoom->bInheritPitch = false; // also fix pitch for good measure
	CameraBoom->bInheritRoll = false;  // also fix roll  
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
	if (!HasAuthority()) return;
	OnRespawn();
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

void AOnsetPlayerCharacter::EnableCameraLag(bool bEnable)
{
	CameraBoom->bEnableCameraLag = bEnable;
}

void AOnsetPlayerCharacter::OnDeath(AActor* KillingActor)
{
	Super::OnDeath(KillingActor);
	DisableInput(nullptr);
	if (!HasAuthority()) return;
	GetWorldTimerManager().SetTimerForNextTick(this, &AOnsetPlayerCharacter::RespawnPlayer);
}
