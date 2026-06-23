// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/OnsetCheatManager.h"

#include "AbilitySystemComponent.h"
#include "GAS/OnsetAttributeSet.h"
#include "GAS/OnsetGameplayTags.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Player/OnsetPlayerController.h"


void UOnsetCheatManager::God()
{
	bGodMode = !bGodMode;
	UE_LOG(LogTemp, Log, TEXT("God mode %hs"), bGodMode ? "enabled" : "disabled");
	
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponent();
	if (!AbilitySystemComponent) return;
	if (bGodMode)
	{
		AbilitySystemComponent->AddLooseGameplayTag(TAG_State_Invulnerable);
	}
	else
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(TAG_State_Invulnerable);
	}
}

void UOnsetCheatManager::Heal()
{
	AOnsetBaseCharacter* Character = GetOnsetCharacter();
	if (!Character) return;
	Character->AttributeSet->SetHealth(Character->AttributeSet->GetMaxHealth());
}

const AController* UOnsetCheatManager::GetController() const
{
	APlayerController* PlayerController =  GetPlayerController();
	if (!PlayerController) return nullptr;
	AOnsetPlayerController* OnsetPlayerController = Cast<AOnsetPlayerController>(PlayerController);
	return OnsetPlayerController ? OnsetPlayerController->GetActiveController() : PlayerController;
}

AOnsetBaseCharacter* UOnsetCheatManager::GetOnsetCharacter() const
{
	const AController* Controller = GetController();
	if (!Controller) return nullptr;
	return Controller->GetPawn<AOnsetBaseCharacter>();
}

UAbilitySystemComponent* UOnsetCheatManager::GetAbilitySystemComponent() const
{
	AOnsetBaseCharacter* Character = GetOnsetCharacter();
	if (!Character) return nullptr;
	return Character->AbilitySystemComponent;
}