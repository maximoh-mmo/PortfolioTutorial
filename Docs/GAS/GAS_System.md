## 📘 GAS System — `/Docs/GAS/GASSystem.md`

# **GAS System**

## Purpose
Provide a robust, extensible framework for abilities, effects, attributes, and combat interactions using Unreal’s Gameplay Ability System.

## Responsibilities
- Manage attributes (health, damage, etc.)  
- Execute abilities (attacks, dash, AoE, projectiles)  
- Apply GameplayEffects (damage, buffs, debuffs)  
- Handle cooldowns and cost  
- Replicate ability usage in multiplayer  

## Non‑Responsibilities
- Input mapping (handled by Player System)  
- AI decision making  
- Visual/audio polish  

## Key Classes
- **`UAbilitySystemComponent` (ASC)** — core GAS component  
- **`UBaseAttributeSet`** — health, damage, etc.  
- **`UGameplayAbility` subclasses** — GA_Attack, GA_Dash, GA_AoE, etc.  
- **`UGameplayEffect`** — damage, cooldown, etc.  

## Key Functions
- `GiveAbility()` — grant abilities to player/NPC  
- `TryActivateAbilityByTag()` — used by player and AI  
- `ApplyGameplayEffectToTarget()` — apply damage/effects  

## Data Flow
Input/AI → ASC → Ability → Effects → Attribute changes → Death/Hit reactions

## Interactions
- **Player System:** triggers abilities from input  
- **NPC AI System:** triggers abilities from StateTree  
- **UI:** reads cooldowns, health, etc.  

## Replication
- GAS handles ability and effect replication  
- ASC exists on both server and client  
- Server is authoritative for attribute changes  

## Edge Cases
- Ability activation failure (cooldown, cost)  
- Multiple effects stacking  
- Death mid‑ability  

## Testing Checklist
- [ ] Abilities activate correctly  
- [ ] Damage and death work as expected  
- [ ] Cooldowns behave correctly  
- [ ] Replication is correct (clients see effects)