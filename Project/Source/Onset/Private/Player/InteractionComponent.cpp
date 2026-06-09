// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/InteractionComponent.h"
#include "NavigationSystem.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Player/OnsetPlayerController.h"
#include "Player/TargetingComponent.h"


class UNavigationSystemV1;

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInteractionComponent::ProcessPrimaryInteraction(FVector2D ScreenPosition)
{
	AOnsetPlayerController* PlayerController = Cast<AOnsetPlayerController>(GetOwner());
	if (!PlayerController) return;
	
	if (!TargetingComponent)
	{
		if (APawn* Pawn = PlayerController->GetPawn())
		{
			TargetingComponent = Pawn->FindComponentByClass<UTargetingComponent>();
			if (!TargetingComponent) return;
		}
	}
	
	FHitResult HitResult;
	if (!PlayerController->GetHitResultAtScreenPosition(ScreenPosition, ECC_Visibility, false, HitResult)) return;
	AActor* HitActor = HitResult.GetActor();                                                                    
         UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());               
         FNavLocation NavLoc;                                                                                        
         bool bIsHostile = false;                                                                                    
                                                                                                                     
         if (HitActor)                                                                                               
         {                                                                                                           
             if (HitActor->ActorHasTag("Enemy") || TargetingComponent->IsActorTargetPVPValid(HitActor, PlayerController->GetPawn()))  
             {                                                                                                       
                 TargetingComponent->SetTarget(HitActor);                                                            
                 PlayerController->StartAutoAttack();                                                                
                 bIsHostile = true;                                                                                  
             }                                                                                                       
         }                                                                                                           
                                                                                                                     
         if (NavSys && NavSys->ProjectPointToNavigation(HitResult.Location, NavLoc))                                 
         {                                                                                                           
             UAIBlueprintHelperLibrary::SimpleMoveToLocation(PlayerController, NavLoc.Location);                     
         }                                                                                                           
         else if (HitActor)                                                                                          
         {                                                                                                           
             UAIBlueprintHelperLibrary::SimpleMoveToActor(PlayerController, HitActor);                               
         }                                                                                                           
                                                                                                                     
         if (!bIsHostile)                                                                                            
         {                                                                                                           
             TargetingComponent->ClearTarget();                                                                      
             PlayerController->StopAutoAttack();                                                                     
         }                           
}