// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Tasks/OnsetStateTreeSearchTask.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"

EStateTreeRunStatus FOnsetStateTreeSearchTask::EnterState(FStateTreeExecutionContext& Context,
                                                          const FStateTreeTransitionResult& Transition) const
{
	AOnsetAIController* Controller = GetController(Context);
	if (!Controller) return EStateTreeRunStatus::Failed;
	
	AOnsetBaseCharacter* Self = GetSelfBaseCharacter(Context);
	if (!Self) return EStateTreeRunStatus::Failed;
	
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	Controller->bHasPendingNoise = false;
	
	InstanceData.SearchCenter = Controller->HeardNoiseLocation;
	if (InstanceData.SearchCenter.IsNearlyZero())
	{
		InstanceData.SearchCenter = Self->GetActorLocation();
	}
	
	UCharacterMovementComponent* CharacterMovement = Self->GetCharacterMovement();
	if (!CharacterMovement) return EStateTreeRunStatus::Failed;
	InstanceData.InitialForward = Self->GetActorForwardVector();                                                
	InstanceData.CachedMovementSpeed = CharacterMovement->MaxWalkSpeed;                          
	InstanceData.CurrentCycle = 0;                                                                              
	InstanceData.ElapsedTime = 0.0f;   
	
	CharacterMovement->MaxWalkSpeed = InstanceData.CachedMovementSpeed * InstanceData.SearchMovementSpeedMultiplier;
	
	InstanceData.CurrentSearchPoint = PickSearchPoint(InstanceData, Self->GetActorLocation(), Self->GetWorld());
	Controller->MoveToLocation(InstanceData.CurrentSearchPoint, InstanceData.AcceptanceRadius);
	return EStateTreeRunStatus::Running;
}


EStateTreeRunStatus FOnsetStateTreeSearchTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	AOnsetAIController* Controller = GetController(Context);
	if (!Controller) return EStateTreeRunStatus::Failed;
	
	AOnsetBaseCharacter* Self = GetSelfBaseCharacter(Context);
	if (!Self) return EStateTreeRunStatus::Failed;
	
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	if (!GetPathFollowingComponent(Context)) return EStateTreeRunStatus::Failed;
	
	InstanceData.ElapsedTime += DeltaTime;
	
	if (HasMoveCompleted(Context))
	{
		InstanceData.CurrentCycle++;
		if (InstanceData.CurrentCycle > InstanceData.MinCycles && InstanceData.ElapsedTime >= InstanceData.MinSearchDuration)
		{
			return EStateTreeRunStatus::Succeeded;		
		}
		InstanceData.CurrentSearchPoint = PickSearchPoint(InstanceData, Self->GetActorLocation(), Self->GetWorld());
		Controller->MoveToLocation(InstanceData.CurrentSearchPoint, InstanceData.AcceptanceRadius);
	}
	
	ApplyYawSweep(Context,Controller,Self);
	return EStateTreeRunStatus::Running;
}

void FOnsetStateTreeSearchTask::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FOnsetStateTreeTaskBase::ExitState(Context, Transition);
}

FVector FOnsetStateTreeSearchTask::PickSearchPoint(const FInstanceDataType& InstanceData, const FVector& Vector, UWorld* World) const
{
	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSystem) return InstanceData.SearchCenter;
	
	for (int32 Attempt = 0; Attempt < 10; ++Attempt)
	{
		FVector2D RandomOffset2D = FMath::RandPointInCircle(InstanceData.SearchRadius);
		FVector RandomPoint = InstanceData.SearchCenter + FVector(RandomOffset2D.X, RandomOffset2D.Y, 0.0f);    
                                                                                                                     
		FVector DirToPoint = (RandomPoint - InstanceData.SearchCenter).GetSafeNormal();                         
		float Dot = FVector::DotProduct(InstanceData.InitialForward, DirToPoint);                               
		float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(InstanceData.ConeHalfAngle));                   
                                                                                                                     
		if (Dot >= CosHalfAngle)                                                                                
		{                                                                                                       
			FNavLocation Projected;                                                                             
			if (NavSystem->ProjectPointToNavigation(RandomPoint, Projected, FVector(InstanceData.SearchRadius * 0.5f)))         
			{                                                                                                   
				return Projected.Location;                                                                      
			}                                                                                                   
		}            
	}
	return InstanceData.SearchCenter;
}

void FOnsetStateTreeSearchTask::ApplyYawSweep(const FStateTreeExecutionContext& Context, AOnsetAIController* Controller, AOnsetBaseCharacter* Self) const                        
{                                                                                                               
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);                                     
	if (!Controller || !Self) return;                                                                           
                                                                                                                     
	FVector MoveDir = (InstanceData.CurrentSearchPoint - Self->GetActorLocation()).GetSafeNormal();             
	if (MoveDir.IsNearlyZero()) return;                                                                         
                                                                                                                     
	float SweepAngle = FMath::Sin(InstanceData.ElapsedTime * 2.0f) * 60.0f;                                     
	FVector SweepDir = MoveDir.RotateAngleAxis(SweepAngle, FVector::UpVector);                                  
	FVector FocusPoint = Self->GetActorLocation() + SweepDir * 1000.0f;                                         
                                                                                                                     
	Controller->SetFocalPoint(FocusPoint, EAIFocusPriority::Gameplay);                                          
}     