// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/OnsetCheatManager.h"

#include "AbilitySystemComponent.h"
#include "GAS/OnsetAttributeSet.h"
#include "GAS/OnsetGameplayTags.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Player/OnsetPlayerController.h"
#include "Widgets/Text/ISlateEditableTextWidget.h"

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
	const auto Character = GetOnsetCharacter();
	if (!Character) return;
	Character->AttributeSet->SetHealth(Character->AttributeSet->GetMaxHealth());
}

AOnsetPlayerController* UOnsetCheatManager::GetOnsetPlayerController() const
{                                                                                                               
	return Cast<AOnsetPlayerController>(GetPlayerController());                                                 
}

AOnsetBaseCharacter* UOnsetCheatManager::GetOnsetCharacter() const
{
	AOnsetPlayerController* Controller = GetOnsetPlayerController();
	if (!Controller) return nullptr;
	return Controller->GetPawn<AOnsetBaseCharacter>();
}

UAbilitySystemComponent* UOnsetCheatManager::GetAbilitySystemComponent() const
{
	UAbilitySystemComponent* AbilitySystemComponent = GetOnsetCharacter()->AbilitySystemComponent;
	return AbilitySystemComponent;
}