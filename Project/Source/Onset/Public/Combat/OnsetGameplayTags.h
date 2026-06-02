// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "NativeGameplayTags.h"

DECLARE_LOG_CATEGORY_EXTERN(LogOnsetGameplayTags, Log, All)

// Damage types
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Physical)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Damage_Magical)

// Slate tags
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Dead)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Staggered)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_State_Invulnerable)

// Cooldown tags
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_Melee)