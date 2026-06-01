// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/OnsetBaseCharacter.h"
#include "OnsetEnemy.generated.h"

class UGroupComponent;
class UAIProfile;

/** NPC pawn owned by AOnsetAIController. Visuals are driven by UAIProfile via ApplyProfile(). */
UCLASS()
class ONSET_API AOnsetEnemy : public AOnsetBaseCharacter
{
	GENERATED_BODY()

public:
	AOnsetEnemy();

	/** Applies or clears the profile — sets mesh, anim BP, material, and capsule size. */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void ApplyProfile(UAIProfile* InProfile);

	/** Group membership component. Pawn-level bridge to UGroupManagerComponent. */
	UPROPERTY()
	TObjectPtr<UGroupComponent> GroupComp;

	/** The active profile asset. Set by ApplyProfile(). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	TObjectPtr<UAIProfile> Profile;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	FVector HomeLocation;
};
