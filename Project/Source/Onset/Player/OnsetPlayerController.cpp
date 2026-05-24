// © 2026 Max Heinze. All rights reserved.
// All original code, gameplay systems, assets, and documentation included in this project are the intellectual
// property of the author unless otherwise stated. This project uses Unreal Engine.
// Unreal Engine and its logo are trademarks or registered trademarks of Epic Games, Inc.
// All third‑party assets remain the property of their respective creators.


#include "OnsetPlayerController.h"

#include "Components/InputComponent.h"
#include "Engine/HitResult.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"

AOnsetPlayerController::AOnsetPlayerController()
{
	TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("TargetingComponent"));
	
}

void AOnsetPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindAction("ClickMove", IE_Pressed, this, &AOnsetPlayerController::OnClick);
}

void AOnsetPlayerController::OnClick()
{
	FHitResult HitResult;
	GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, false, HitResult);
	if (HitResult.bBlockingHit)
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor && HitActor->ActorHasTag("Enemy"))
		{
			TargetingComponent->SetCurrentTarget(HitActor);
		}
		else
		{
			TargetingComponent->ClearCurrentTarget();
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, HitResult.Location);
		}
	}
	else
	{
		TargetingComponent->ClearCurrentTarget();
	}
}
