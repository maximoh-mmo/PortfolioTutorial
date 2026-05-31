// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StateTreeAIComponentSchema.h"
#include "OnsetStateTreeSchema.generated.h"

/** Schema that restricts ST_NPC_Base to Onset-specific tasks and context data. */
UCLASS()
class ONSET_API UOnsetStateTreeSchema : public UStateTreeAIComponentSchema
{
	GENERATED_BODY()
	
public:
	UOnsetStateTreeSchema();
};
