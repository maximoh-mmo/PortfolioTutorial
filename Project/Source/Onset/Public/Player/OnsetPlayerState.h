// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "OnsetPlayerState.generated.h"

UCLASS()
class ONSET_API AOnsetPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	UPROPERTY(ReplicatedUsing=OnRep_PvPEnabled) 
	bool bIsPvPEnabled;
	UFUNCTION()
	void OnRep_PvPEnabled();	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
};
