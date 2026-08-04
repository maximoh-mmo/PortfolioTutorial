# 📘 Combat System Design
**File:** `Design/Combat/Combat_System_Design.md`

---

## Overview
This document defines the combat abilities for the A4.6 deferred work. It adopts the concrete specs drafted in `TODO/DONE/06-11-26.md`, `06-12-26.md`, `06-15-26.md` and adds the missing A4.2 (anim montage) and A4.3 (hitstop/stagger) specs.

All abilities are implemented as `UGameplayAbility` subclasses with matching Blueprint children for tuning.

---

## Ability Slot Mapping

| Slot | Ability | BP Child | Tag |
|------|---------|----------|-----|
| 0 (Basic) | `GA_OnsetBasicAttack` | `GA_BasicAttack` | `TAG_Ability_Attack` |
| 1 | `GA_OnsetAoE` | `GA_AoE` | `TAG_Ability_AoE` |
| 2 | `GA_OnsetCone` | `GA_Cone` | `TAG_Ability_Cone` |
| Passive | `GA_OnsetShadowstep` | `GA_Shadowstep` | `TAG_Ability_Shadowstep` |

---

## 1. AoE Ability — `GA_OnsetAoE` (Slot 1)

### Gameplay
Target-centered damage volume. Activated by player input (Slot 1 keybind). NPCs do not use this.

### Implementation
- **C++ Class:** `GA_OnsetAoE : public UGameplayAbility`
- **Activation:** `ActivateAbility()` → `UWorld::OverlapMultiByChannel()` (sphere) at target location
- **Channel:** `ECC_GameTraceChannel1` (or dedicated `ECC_AbilityOverlap`)
- **Radius:** Configurable via `FAbilityParams` in BP child (default 300 units)
- **Damage GE:** `GE_AoE_Damage` (instant, magnitude = `DamageAttribute` from AttributeSet)
- **Target Data:** `FAbilityTargetData` from `UAbilityTargetingLibrary::GetTargetData()` — uses `TargetLocation` as sphere center
- **PvP Filtering:** Iterate overlap results, per-actor call `ShouldApplyDamage(Source, Target)` which checks PvP flag (see [PvP System](../Gameplay/PVP_System.md))
- **Cooldown:** `GE_AoE_Cooldown` (duration configurable in BP, default 8s)
- **Cost:** `GE_AoE_Cost` (mana/stamina if implemented, otherwise none)

### BP Child: `Content/Game/Combat/GA_AoE`
- Tunable: Radius, Damage, Cooldown, Cost
- Visual: Particle system at target location, screen shake

### Verification
- [ ] AoE damages enemies in radius
- [ ] AoE respects PvP rules (filters per target — skip players when PvP OFF)
- [ ] AoE activates from Slot 1 keybind
- [ ] Cooldown prevents spam
- [ ] Works in multiplayer (server-authoritative)

---

## 2. Cone Ability — `GA_OnsetCone` (Slot 2)

### Gameplay
Directional frontal cone from character forward. Activated by player input (Slot 2 keybind). NPCs do not use this.

### Implementation
- **C++ Class:** `GA_OnsetCone : public UGameplayAbility`
- **Activation:** `ActivateAbility()` → `UKismetSystemLibrary::ConeOverlapActors()` from character location + forward vector
- **Cone Params:** Angle (default 90°), Range (default 500 units), configurable in BP
- **Damage GE:** `GE_Cone_Damage` (instant)
- **Target Data:** Source = owning pawn, Direction = pawn forward, Location = pawn location
- **PvP Filtering:** Per-actor `ShouldApplyDamage(Source, Target)`
- **Cooldown:** `GE_Cone_Cooldown` (default 10s)
- **Cost:** `GE_Cone_Cost`

### BP Child: `Content/Game/Combat/GA_Cone`
- Tunable: Angle, Range, Damage, Cooldown, Cost
- Visual: Cone decal/particle, directional impact FX

### Verification
- [ ] Cone damages enemies in correct shape
- [ ] Cone respects PvP rules (per-target filtering)
- [ ] Cone activates from Slot 2 keybind
- [ ] Cooldown prevents spam
- [ ] Works in multiplayer

---

## 3. Shadowstep Passive — `GA_OnsetShadowstep`

### Gameplay
On-kill blink behind nearest enemy within distance gate. Passive (triggered by death event), not player-activated.

### Implementation
- **C++ Class:** `GA_OnsetShadowstep : public UGameplayAbility` (passive, `bIsPassive = true`)
- **Trigger:** `FAbilityTriggerData(TAG_Event_Death)` — same pattern as `GA_HitReaction`
- **Logic on Trigger:**
  1. Find nearest living enemy within `DistanceGate` (default 1500 units) from killer location
  2. If valid target found:
     - Teleport behind: `TargetLocation = Enemy->GetActorLocation() + Enemy->GetActorForwardVector() * -BehindOffset` (default 200 units)
     - Apply brief invulnerability: `AddLooseGameplayTag(TAG_State_Invulnerable, 0.5s)`
  3. If no valid target → ability fizzles (no teleport, no cooldown)
- **Cooldown:** `GE_Shadowstep_Cooldown` (default 15s) — prevents chain-kill spam
- **Cost:** None (passive)
- **Target Data:** Not used (ability finds its own target)

### BP Child: `Content/Game/Combat/GA_Shadowstep`
- Tunable: DistanceGate, BehindOffset, InvulnerabilityDuration, Cooldown

### Verification
- [ ] Shadowstep triggers on kill
- [ ] Teleports behind nearest enemy within distance gate
- [ ] Brief invulnerability applied
- [ ] Cooldown prevents spam
- [ ] Fizzles if no valid target (no teleport, no cooldown)
- [ ] Works in multiplayer

---

## 4. A4.2 — Melee Swing Anim Montage (Basic Attack)

### Gameplay
`GA_BasicAttack` plays a swing montage. Root motion optional.

### Implementation
- **Montage Asset:** `AM_BasicAttack_Swing` (single swing, ~0.8s)
- **GA_BasicAttack Changes:**
  - `ActivateAbility()` → `PlayMontageAndWait(AM_BasicAttack_Swing)`
  - On montage end → apply damage GE to current target (via `FAbilityTargetData`)
  - Add `TAG_Ability_Attack` to ability (for Player AI filtering)
- **Hit Timing:** Damage applied at montage notify `HitFrame` (or fixed 0.3s into montage)
- **Cancel/Interrupt:** Standard GAS ability cancellation (movement input cancels if `bCancelOnMovement`)

### BP Child: `Content/Game/Combat/GA_BasicAttack` (existing — add montage)

### Verification
- [ ] Montage plays on basic attack activation
- [ ] Damage applied at correct frame
- [ ] Root motion / movement handled correctly
- [ ] Works in multiplayer

---

## 5. A4.3 — Hitstop / Stagger Effect

### Gameplay
Brief time dilation + knockback on hit. Applied via `GE_Stagger`.

### Implementation
- **GameplayEffect:** `GE_Stagger` (Instant, Duration = 0.15s)
  - **Modifiers:** None (purely for tag application)
  - **Granted Tags:** `TAG_State_Staggered` (duration 0.15s)
  - **Execution:** Custom `UGameplayEffectExecutionCalculation` (`ExecCalc_Stagger`)
    - Applies knockback: `LaunchCharacter(KnockbackDirection * Magnitude, true, true)`
    - Applies global time dilation: `UGameplayStatics::SetGlobalTimeDilation(0.1)` for 0.1s, then restore
- **Application:** `GA_BasicAttack`, `GA_AoE`, `GA_Cone`, NPC `GA_EnemyAttack` all apply `GE_Stagger` on successful hit
- **Immunity:** `TAG_State_Invulnerable` blocks stagger (Shadowstep grants this)

### Verification
- [ ] Hitstop (time dilation) triggers on hit
- [ ] Stagger knockback applied
- [ ] Duration matches spec (0.15s)
- [ ] Invulnerability blocks stagger
- [ ] Works in multiplayer

---

## 6. Ability Bar UI Stub (A4.6)

### Purpose
Minimal in-game UI showing 3 ability slots + basic attack with cooldown overlays. Full HUD in A6.1.

### Implementation
- **Widget:** `UAbilityBarWidget` (planned in A6.1, stub here)
- **Slots:** 4 (Basic, Slot 1, Slot 2, Passive indicator)
- **Data Source:** ASC cooldown queries (`GetCooldownRemainingForTag()`)
- **Visual:** Icon + radial cooldown sweep + keybind label

### Verification
- [ ] Shows 4 slots
- [ ] Cooldowns update in real-time
- [ ] Keybinds displayed correctly

---

## 7. PvP Filtering Verification (All Abilities)

### Rule
Per-target check in damage execution: `ShouldApplyDamage(Source, Target)` returns false if:
- Source and Target are both players
- Source's `bIsPvPEnabled == false` OR Target's `bIsPvPEnabled == false`

### Test Matrix
| Ability | PvP OFF: Enemy | PvP OFF: Player | PvP ON: Enemy | PvP ON: Player |
|---------|----------------|-----------------|---------------|----------------|
| Basic   | ✅ Dmg         | ❌ Blocked      | ✅ Dmg        | ✅ Dmg         |
| AoE     | ✅ Dmg         | ❌ Blocked      | ✅ Dmg        | ✅ Dmg         |
| Cone    | ✅ Dmg         | ❌ Blocked      | ✅ Dmg        | ✅ Dmg         |
| Shadowstep | N/A (passive) | N/A           | N/A           | N/A            |

---

## 8. Multiplayer Notes

- All abilities are **server-authoritative**: Client predicts activation, server re-validates target + range + PvP
- `FAbilityTargetData` replicated with activation RPC
- Montages replicate via GAS ability replication (animation root motion on server)
- Cooldowns: Server authoritative, client predicts UI
- Shadowstep teleport: Server executes, replicates location

---

## 9. Asset Checklist

| Asset | Path | Status |
|-------|------|--------|
| `GA_OnsetAoE.h/.cpp` | `Source/Onset/Public/Combat/GA_OnsetAoE.h` | To create |
| `GA_AoE` BP | `Content/Game/Combat/GA_AoE` | To create |
| `GE_AoE_Damage` | `Content/Game/Combat/GE_AoE_Damage` | To create |
| `GE_AoE_Cooldown` | `Content/Game/Combat/GE_AoE_Cooldown` | To create |
| `GA_OnsetCone.h/.cpp` | `Source/Onset/Public/Combat/GA_OnsetCone.h` | To create |
| `GA_Cone` BP | `Content/Game/Combat/GA_Cone` | To create |
| `GE_Cone_Damage` | `Content/Game/Combat/GE_Cone_Damage` | To create |
| `GE_Cone_Cooldown` | `Content/Game/Combat/GE_Cone_Cooldown` | To create |
| `GA_OnsetShadowstep.h/.cpp` | `Source/Onset/Public/Combat/GA_OnsetShadowstep.h` | To create |
| `GA_Shadowstep` BP | `Content/Game/Combat/GA_Shadowstep` | To create |
| `GE_Shadowstep_Cooldown` | `Content/Game/Combat/GE_Shadowstep_Cooldown` | To create |
| `AM_BasicAttack_Swing` | `Content/Game/Combat/AM_BasicAttack_Swing` | To create/import |
| `GE_Stagger` | `Content/Game/Combat/GE_Stagger` | To create |
| `ExecCalc_Stagger.h/.cpp` | `Source/Onset/Public/Combat/ExecCalc_Stagger.h` | To create |

---

## 10. Dependencies

- GAS System (`OnsetAttributeSet`, `UOnsetAbilitySystemComponent`) — already implemented
- Targeting System (`UAbilityTargetingLibrary`) — already implemented
- PvP System (`ShouldApplyDamage`) — already implemented
- Tags: `TAG_Ability_Attack`, `TAG_Ability_AoE`, `TAG_Ability_Cone`, `TAG_Ability_Shadowstep`, `TAG_Event_Death`, `TAG_State_Staggered`, `TAG_State_Invulnerable` — defined in `OnsetGameplayTags`

---

## 11. Tuning Parameters (Initial Values)

| Parameter | Ability | Default |
|-----------|---------|---------|
| Radius | AoE | 300 |
| Damage | AoE | 25 |
| Cooldown | AoE | 8s |
| Angle | Cone | 90° |
| Range | Cone | 500 |
| Damage | Cone | 30 |
| Cooldown | Cone | 10s |
| DistanceGate | Shadowstep | 1500 |
| BehindOffset | Shadowstep | 200 |
| InvulnDuration | Shadowstep | 0.5s |
| Cooldown | Shadowstep | 15s |
| StaggerDuration | All | 0.15s |
| TimeDilation | Stagger | 0.1 for 0.1s |
| KnockbackMagnitude | Stagger | 500 |

---

## 12. Verification Checklist (A4.6 Complete Criteria)

- [ ] `GA_OnsetAoE` + `GA_AoE` BP compiles, activates from Slot 1, damages enemies, respects PvP
- [ ] `GA_OnsetCone` + `GA_Cone` BP compiles, activates from Slot 2, damages enemies, respects PvP
- [ ] `GA_OnsetShadowstep` + `GA_Shadowstep` BP compiles, triggers on kill, teleports, cooldown works
- [ ] `GA_BasicAttack` montage plays, damage at correct frame
- [ ] `GE_Stagger` applies hitstop + knockback on all damaging abilities
- [ ] Ability bar stub displays 4 slots with cooldowns
- [ ] All PvP filtering verified per matrix above
- [ ] Multiplayer test: 2 clients + DS, all abilities work, no desyncs