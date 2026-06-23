// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/OnsetBaseCharacter.h"
#include "OnsetEnemy.generated.h"

class UGroupComponent;
class UVisualProfile;
class AOnsetSpawner;

/** NPC pawn owned by AOnsetAIController. Visuals are driven by UAIProfile via ApplyProfile(). */
UCLASS()
class ONSET_API AOnsetEnemy : public AOnsetBaseCharacter
{
	GENERATED_BODY()

public:
	AOnsetEnemy();

	/** Applies or clears the profile — sets mesh, anim BP, material, and capsule size. */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void ApplyProfile(UVisualProfile* InProfile);
	
	/** Group membership component. Pawn-level bridge to UGroupManagerComponent. */
	UPROPERTY()
	TObjectPtr<UGroupComponent> GroupComp;
	
	/** Visual profile defining mesh, anim BP, material, and capsule size. Applied via ApplyProfile(). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual Profile")
	TObjectPtr<UVisualProfile> VisualProfile;
	
	UPROPERTY()
	TObjectPtr<AOnsetSpawner> OwningSpawner;
	
	virtual void OnDeath(AActor* KillingActor = nullptr) override;
	
protected:

	void DeferredDeathCleanup();
};
