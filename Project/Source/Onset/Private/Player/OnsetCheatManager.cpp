// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/OnsetCheatManager.h"

#include "AbilitySystemComponent.h"
#include "GAS/OnsetAttributeSet.h"
#include "GAS/OnsetCombatAttributeSet.h"
#include "GAS/OnsetCooldownSlowEffect.h"
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

void UOnsetCheatManager::ApplyCooldownSlow(float RateMod, float Duration)
{
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponent();
	if (!AbilitySystemComponent) return;

	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(UOnsetCooldownSlowEffect::StaticClass(), 1.0f, Context);
	if (!SpecHandle.IsValid()) return;

	SpecHandle.Data->SetSetByCallerMagnitude(FName("CooldownRateMod"), FMath::Max(1.0f, RateMod));
	SpecHandle.Data->SetSetByCallerMagnitude(FName("Duration"), FMath::Max(0.0f, Duration));

	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);

	AOnsetBaseCharacter* Character = GetOnsetCharacter();
	const float NewMultiplier = Character && Character->CombatAttributes ? Character->CombatAttributes->GetCooldownMultiplier() : 1.0f;
	UE_LOG(LogTemp, Log, TEXT("CooldownSlow applied: RateMod=%.2f Duration=%.1f (CooldownMultiplier now %.2f)"), RateMod, Duration, NewMultiplier);
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