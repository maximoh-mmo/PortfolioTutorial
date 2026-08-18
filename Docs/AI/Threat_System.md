

## 📘 Threat System — `/Docs/AI/Threat_System.md`

# **Threat System**

## Purpose
Provide a **server-side threat table** that drives NPC target selection and positional distribution, enabling:
- Threat-driven targeting (highest-damage dealer is targeted)
- Angular spread around the target to prevent bunching
- Taunt/tank mechanics via threat multipliers
- Clean integration with existing perception and StateTree pipeline

## Design Decision: World Subsystem over Component
`UOnsetThreatSubsystem` (world subsystem) rather than a per-NPC component:
- Zero per-NPC overhead
- Single map clean-up on pool return / player death
- Server-only data — no replication needed

## Key Class: `UOnsetThreatSubsystem`

### Data Storage

| Name | Type | Purpose |
|------|------|---------|
| `ThreatTable` | `TMap<TWeakObjectPtr<AOnsetEnemy>, TMap<TWeakObjectPtr<AOnsetBaseCharacter>, float>>` | Threat per enemy per player. Outer key = NPC, inner key = player pawn, value = accumulated threat. `TWeakObjectPtr` prevents keeping dead actors alive. |
| `EngagementTable` | `TMap<TWeakObjectPtr<AOnsetBaseCharacter>, TArray<TWeakObjectPtr<AOnsetEnemy>>>` | Tracks which NPCs are currently in combat with each player. Used for angular spread computation. |

### API

```cpp
/** Add threat to an enemy for a specific player. Returns the new total. */
void AddThreat(AOnsetBaseCharacter* PlayerCharacter, AOnsetEnemy* Enemy, float ThreatAmount);

/** Remove a player from all threat tables (on player death/disconnect). */
void RemovePlayer(const AOnsetBaseCharacter* PlayerCharacter);

/** Remove an enemy from all threat tables (on NPC death/pool return). */
void RemoveEnemy(AOnsetEnemy* Enemy);

/** Get the highest-threat player pawn for an NPC. */
APawn* GetPrimaryTarget(AOnsetEnemy* Enemy);

/** Get best target weighted by threat × distance. Full weight within AttackRange,
    0.5× within ChaseRange, 0.1× beyond. */
AOnsetBaseCharacter* GetBestTarget(AOnsetEnemy* Enemy, float AttackRange, float ChaseRange);

/** Get the threat-sorted rank for a specific player targeting this NPC. 0 = highest threat. */
int32 GetTargetRank(AOnsetEnemy* Enemy, AOnsetBaseCharacter* PlayerCharacter);

/** Get total number of players with positive threat toward this NPC. */
int32 GetTargetCount(AOnsetEnemy* Enemy) const;

/** Register an NPC as engaged with a player (for angular spread tracking). */
void RegisterEngaged(AOnsetBaseCharacter* PlayerCharacter, AOnsetEnemy* Enemy);

/** Unregister an NPC from all engagement lists. */
void UnregisterEngaged(AOnsetEnemy* Enemy);

/** Check if an NPC is currently engaged with a specific player. */
bool IsEnemyEngagedWithPlayer(AOnsetEnemy* Enemy, AOnsetBaseCharacter* PlayerCharacter) const;

/** Get number of NPCs engaged with a player. */
int32 GetEngagedCount(AOnsetBaseCharacter* PlayerCharacter);

/** Get this NPC's rank among all NPCs engaged with a player (for angular spread). */
int32 GetEngagedIndex(AOnsetEnemy* Enemy, AOnsetBaseCharacter* PlayerCharacter);

/** Switch this Enemy's engagement from its current player(s) to NewPlayer. */
void SwitchTarget(AOnsetEnemy* Enemy, AOnsetBaseCharacter* NewPlayer);

/** Clear all tables (on level transition). */
void ClearAll();
```

## Integration Points

### Damage Pipeline

`UOnsetAttributeSet::PostGameplayEffectExecute` calls `Subsystem->AddThreat(Instigator, Victim, Threat)` when damage > 0 and victim is `AOnsetEnemy`. Also emits `ReportNoiseEvent` for AI hearing sense (group assist).

**Threat = |damage| × ability multiplier × class multiplier**

```
Threat = |Data.EvaluatedData.Magnitude| × AbilityThreatMultiplier × ClassThreatMultiplier
```

- **Ability multiplier** — `FOnsetAbilityDefinition::ThreatMultiplier` (DT_Abilities). `UOnsetGA_Generic` caches the resolved row's multiplier at activation and stamps the outgoing effect context with `SetAbility(this)`; `UOnsetAttributeSet` reads it via `GetAbility()->GetThreatMultiplier()`. Default 1.0; values > 1 create taunt-style high-threat abilities. DoT ticks inherit the same multiplier through the captured spec/context.
- **Class multiplier** — `FOnsetCharacterClassInfo::ThreatMultiplier` (DT_ClassInfo). The instigator's class row scales all threat it generates (Tank identity: more threat per damage without extra DPS). Default 1.0.
- The two multipliers multiply, so a Tank using a high-threat ability compounds both. Both are clamped ≥ 0.
- **Enemy-instigated damage** falls through to the DPS row (×1.0) — enemy threat toward players is not tracked this way (the table is player→NPC only).

### Perception

Perception feeds **initial** awareness (sight → `TargetingComponent->SetTarget`, hearing → noise/investigate). Threat overrides the target when the NPC engages in combat.

### Sight-Based Threat

`AOnsetAIController::OnPerceptionUpdated` adds a base threat of 1.0 on visual perception of a player if the NPC is not already engaged with them. This ensures NPCs enter combat on sight even before taking damage.

### NPC Engagement Flow

The EngageTask (`FEnemyEngageTask`) handles all combat behaviour in a single state:

1. **EnterState**: reads `AttackRange`/`ChaseRange` from `UAIProfile`, calls `GetBestTarget()`, registers engagement, sets focus, computes angular offset position via `GetThreatAngularOffset()`, paths to it
2. **Tick**: re-evaluates target every 1s via `GetBestTarget()`, re-evaluates position every 3s or when target moves > 200 units, fires ability at throttle when within attack range, 2s timeout if target is lost
3. **ExitState**: `StopMovement()`

### Angular Spread (Bunching Fix)

Computed in `EnemyEngageTask::ComputeOffsetPosition`:

```
Count = Subsystem->GetEngagedCount(TargetPlayer)
Rank  = Subsystem->GetEngagedIndex(SelfEnemy, TargetPlayer)
Angle = (Rank / Count) * 360°
Radius = (Dist > AttackRange) ? SpreadRadius : AttackRange
Offset = Target->GetActorLocation()
       + FVector(Cos(Angle), Sin(Angle), 0) * Radius
```

Result nav-projected via `UNavigationSystemV1::ProjectPointToNavigation`. Single enemy → rank 0, count 1 → angle 0° → directly in front at `AttackRange`.

### Active Enemy Tracking for Taunt

Since threat is stored as `Enemy → Player → Threat`, a taunt ability simply calls `AddThreat(Player, Enemy, TauntAmount)` on the subsystem — no special path needed. The highest-threat player automatically becomes the best target.

### Taunt / High-Threat Abilities

Two levers exist for tanking:

1. **Per-ability `ThreatMultiplier`** (`DT_Abilities` / ability-creation dialog) — an ability dealing normal damage can still generate 2–3× threat, acting as a soft taunt. Set in the ability editor's creation dialog.
2. **Per-class `ThreatMultiplier`** (`DT_ClassInfo`) — the Tank row is the identity lever (default 1.5 in content); every ability the Tank casts generates 1.5× threat without inflating its damage, so tanks hold aggro without out-DPSing damage dealers.

## Architecture Simplification

The original plan called for separate Agro/Chase/Attack/AttackPosition tasks. These were replaced by a single **EngageTask** that handles all combat positioning, targeting, and ability firing. Similarly, IdleTask and RoamTask were replaced by a single **PatrolTask**.

StateTree was reduced from 9 states to 6 top-level subtrees with event-driven transitions (no Selector to avoid flickering):
- Patrol (idle/roam)
- Engage (combat)
- Investigate (noise response)
- Search (post-investigate sweep)
- Flee (low health retreat)
- Lost (target lost pause → Patrol)

## File Changes Summary

| File | Action | Change |
|------|--------|--------|
| `Source/Onset/Public/Subsystem/OnsetThreatSubsystem.h` | New | `UOnsetThreatSubsystem` class declaration |
| `Source/Onset/Private/Subsystem/OnsetThreatSubsystem.cpp` | New | Implementation |
| `Source/Onset/Private/GAS/OnsetAttributeSet.cpp` | Modify | Add threat feed + noise event in `PostGameplayEffectExecute` |
| `Source/Onset/Private/Enemy/OnsetEnemy.cpp` | Modify | `DeferredDeathCleanup` calls `Subsystem->RemoveEnemy(this)` |
| `Source/Onset/Private/Subsystem/OnsetPoolSubsystem.cpp` | Modify | `ReturnToPool` calls `Subsystem->RemoveEnemy(Enemy)` |
| `Source/Onset/Public/StateTree/Tasks/OnsetStateTreeTask.h/.cpp` | Modify | Add `GetThreatSubsystem()` and `GetThreatAngularOffset()` helpers |
| `Source/Onset/Public/StateTree/Tasks/Enemy/EnemyEngageTask.h/.cpp` | New | Single combat state replacing Agro/Chase/Attack/AttackPosition |
| `Source/Onset/Public/StateTree/Tasks/Enemy/EnemyPatrolTask.h/.cpp` | New | 50/50 idle vs roam replacing IdleTask + RoamTask |
| `Source/Onset/Private/AI/OnsetAIController.cpp` | Modify | AI LOD tiers, sight-based threat on perception |

Deleted tasks: AgroTask, AttackTask, AttackPositionTask, IdleTask, RoamTask. (`EnemyChaseTask` remains in the source tree but is not part of the current StateTree asset — combat is driven by the single EngageTask.)

## StateTree Changes

Current NPC state flow (simplified):
```
Patrol → [perception/noise] → Investigate → Search → Patrol
Patrol → [sight/threat] → Engage → [flee] → Flee → Patrol
Engage → [target lost 2s] → Lost → Patrol
```

## AI LOD Integration

`AOnsetAIController::UpdateLodTier()` runs every 30 controller ticks (distance measured to the nearest player pawn):

| Distance | Tier | Actor Tick Interval | StateTree |
|----------|------|---------------------|-----------|
| ≤ SightRange | 1 (full) | 0.0 (every frame) | Running |
| ≤ HearingRange | 2 (throttled) | 0.2s | Running |
| > HearingRange | 3 (paused) | 0.5s | Stopped |

## Implementation Order

| Step | Task | Files | Est. |
|------|------|-------|------|
| 1 | Create `UOnsetThreatSubsystem` | 2 new files | 1d |
| 2 | Wire damage → threat feed in `PostGameplayEffectExecute` | 1 modify | 0.5d |
| 3 | Wire pool return + death cleanup | 2 modify | 0.25d |
| 4 | Add helpers to `FOnsetStateTreeTask` base | 1 modify | 0.25d |
| 5-7 | Create EngageTask + PatrolTask (replaces Agro/Chase/Attack/AttackPosition/Idle/Roam) | 4 new files + deletions | 2d |
| 8 | AI LOD + staggered ticks | 1 modify | 1d |
| 9 | StateTree asset update | 1 asset | 0.25d |
| 10 | Verify + tune | | 0.5d |
| | **Total** | | **~5.5d** |

## Edge Cases

- **No threat entries** → `GetBestTarget()` returns null → EngageTask enters 2s timeout → exits to Lost → back to Patrol. No change to perception-driven initial awareness.
- **Single enemy attacking** → rank = 0, count = 1 → angle = 0° → offset directly in front at `AttackRange`. Correct behaviour.
- **Player dies** → `RemovePlayer(DeadPlayer)` clears all threat entries. NPCs with no remaining threat targets exit Engage via timeout.
- **NPC dies** → `RemoveEnemy(DeadNPC)` in `DeferredDeathCleanup`. Other NPCs targeting the same player re-rank and re-position.
- **Pool return** → `RemoveEnemy(NPC)` in `ReturnToPool`. Ensures clean state for next pool lifecycle.
- **Multiplayer** → one subsystem per world. NPCs naturally distribute across players by damage output via `GetBestTarget()` distance-weighted scoring.
- **Taunt** → `AddThreat(Taunter, Enemy, TauntAmount)` with high value. Next target re-evaluation picks the taunter as best target.
