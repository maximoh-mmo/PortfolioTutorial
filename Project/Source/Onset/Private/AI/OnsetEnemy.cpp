// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/OnsetEnemy.h"

#include "AI/GroupComponent.h"
#include "AI/OnsetAIController.h"

AOnsetEnemy::AOnsetEnemy()
{
	this->Tags.Add(FName("Enemy"));
	AIControllerClass = AOnsetAIController::StaticClass();
	GroupComp = CreateDefaultSubobject<UGroupComponent>(TEXT("GroupComp"));                                     
}
