// Fill out your copyright notice in the Description page of Project Settings.
#include "GAS/OnsetAttributeSet.h"

#include "GameplayEffectExtension.h"

#include "GAS/OnsetGameplayTags.h"
#include "Combat/OnsetEquipmentLibrary.h"
#include "Combat/OnsetGameplayAbility.h"
#include "Data/OnsetClassInfoTypes.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AISense_Hearing.h"
#include "Core/OnsetBaseCharacter.h"
#include "Enemy/OnsetEnemy.h"
#include "Subsystem/OnsetThreatSubsystem.h"

UOnsetAttributeSet::UOnsetAttributeSet()
{
	InitHealth(100.0f);
	InitMaxHealth(100.0f);
}

void UOnsetAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		float OldHealth = GetHealth() - Data.EvaluatedData.Magnitude; // Calculate old health before modification
		// Clamp Health to [0, MaxHealth]
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
		
		if (GetHealth() == 0.0f)
		{
			if (OldHealth != GetHealth())
			{
				// Handle death logic here
			
				FGameplayEventData Payload;                                                                                 
				Payload.EventTag = TAG_Event_Death;                                                                   
				Payload.Instigator = Data.EffectSpec.GetContext().GetInstigator();                                          
				Payload.Target = GetOwningActor();
				
				if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
				{
					ASC->HandleGameplayEvent(TAG_Event_Death, &Payload);
				}
				
				if (AOnsetBaseCharacter* Character = Cast<AOnsetBaseCharacter>(GetOwningActor()))
				{
					Character->OnDeath(Data.EffectSpec.GetContext().GetInstigator());
				}
			}
		}

		if (Data.EvaluatedData.Magnitude < 0.0f && GetHealth() > 0.0f)                                                                        
		{
			FGameplayEventData Payload;                                                                                 
			Payload.EventTag = TAG_Event_HitReaction;                                                                   
			Payload.Instigator = Data.EffectSpec.GetContext().GetInstigator();                                          
			Payload.Target = GetOwningActor();                                                                          
			Payload.EventMagnitude = FMath::Abs(Data.EvaluatedData.Magnitude);                                   
                                                                                                                     
			if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())                                       
			{                                                                                                           
				ASC->HandleGameplayEvent(TAG_Event_HitReaction, &Payload);                                              
			}   
			
			if (AActor* OwningActor = GetOwningActor())
			{
				UAISense_Hearing::ReportNoiseEvent(
					OwningActor->GetWorld(),
					OwningActor->GetActorLocation(),
					FMath::Abs(Data.EvaluatedData.Magnitude),  // Loudness = damage amount
					OwningActor,                                // Instigator
					0.0f                                       // Max range 0 = unlimited                       
					);
			}      
			
			if (UOnsetThreatSubsystem* ThreatSub = GetWorld()->GetSubsystem<UOnsetThreatSubsystem>())                   
			{       
				AOnsetEnemy* TargetEnemy = Cast<AOnsetEnemy>(Data.Target.GetOwnerActor());
				AOnsetBaseCharacter* Instigator = Cast<AOnsetBaseCharacter>(Data.EffectSpec.GetContext().GetInstigator());
				if (TargetEnemy && Instigator)                                                                       
				{                                                                                                    
					// Threat = damage x ability multiplier x class multiplier. The ability
					// multiplier travels on the effect context (set by UOnsetGA_Generic);
					// the class multiplier comes from the instigator's DT_ClassInfo row
					// (Tank generates extra threat without extra DPS).
					float ThreatMultiplier = 1.0f;
					if (const UOnsetGameplayAbility* GA = Cast<UOnsetGameplayAbility>(Data.EffectSpec.GetContext().GetAbility()))
					{
						ThreatMultiplier *= GA->GetThreatMultiplier();
					}
					if (const FOnsetCharacterClassInfo* ClassInfo = UOnsetEquipmentLibrary::GetClassInfo(Instigator->GetCharacterClass()))
					{
						ThreatMultiplier *= ClassInfo->ThreatMultiplier;
					}
					ThreatSub->AddThreat(Instigator, TargetEnemy,
						FMath::Abs(Data.EvaluatedData.Magnitude) * FMath::Max(0.0f, ThreatMultiplier));
				}                                                                                                   
			}           
		}        
	}
}

void UOnsetAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UOnsetAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOnsetAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
}

void UOnsetAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOnsetAttributeSet, MaxHealth, OldMaxHealth);
}

void UOnsetAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UOnsetAttributeSet, Health, OldHealth);
}
