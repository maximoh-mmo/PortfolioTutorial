// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOnsetGameplayTags, Log, All)

// Damage elements
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Physical)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Fire)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Ice)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Lightning)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Poison)

// State tags
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Dead)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Staggered)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Stunned)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Frozen)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Invulnerable)

// Target affinity elements (the type-chart "target side"): a character that owns
// one of these is elemental (e.g. an Ice Golem owns Element.Ice). No tag = Neutral,
// which takes 1.0x from every element.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Element_Neutral)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Element_Fire)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Element_Ice)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Element_Lightning)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Element_Poison)

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
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Event_LevelUp)