// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOnsetGameplayTags, Log, All)

// Damage types
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Physical)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Magical)

// State tags
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Dead)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Staggered)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Stunned)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Invulnerable)

// Ability tags
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Attack)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_AoE)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Cone)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Shadowstep)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Buff)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Debuff)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Heal)

// Ability ID namespace: the row-name suffix (AbilityID.<RowName>) is carried in a
// spec's DynamicAbilityTags so UOnsetGA_Generic can resolve its DT_Abilities row.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AbilityID_BasicAttack)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AbilityID_AoE)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AbilityID_Cone)

// Cooldown tags
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_BasicAttack)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_AoE)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_Cone)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_Shadowstep)

// Event tags
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_HitReaction)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_Death)