// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/OnsetGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Damage_Physical, "Damage.Physical");                                                 
UE_DEFINE_GAMEPLAY_TAG(TAG_Damage_Magical, "Damage.Magical");                                                  
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Dead, "State.Dead");                                                    
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Staggered, "State.Staggered");                                               
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Invulnerable,"State.Invulnerable");                                            
UE_DEFINE_GAMEPLAY_TAG(TAG_Cooldown_BasicAttack, "Cooldown.BasicAttack");  
UE_DEFINE_GAMEPLAY_TAG(TAG_Event_HitReaction, "Event.HitReaction");
UE_DEFINE_GAMEPLAY_TAG(TAG_Event_Death, "Event.Death");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Attack, "Ability.Type.Attack");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Buff, "Ability.Type.Buff");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Debuff, "Ability.Type.Debuff");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Heal, "Ability.Type.Heal");