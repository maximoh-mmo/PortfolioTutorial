
## 📘 Aggro System — `/Docs/AI/Aggro_System.md`

# **Aggro System**

## Purpose
Provide a **server-side threat table** that drives NPC target selection and positional distribution, enabling:
- Threat-driven aggro (highest-damage dealer is targeted)
- Angular spread around the target to prevent bunching
- Taunt/tank mechanics via threat multipliers
- Clean integration with existing perception and StateTree pipeline

## Design Decision: World Subsystem over Component
`UOnsetAggroSubsystem` (world subsystem) rather than a per-NPC component:
- Zero per-NPC overhead
- Single map clean-up on pool return / player death
- Server-only data — no replication needed

## Key Class: `UOnsetAggroSubsystem`

### Data Storage

| Name | Type | Purpose |
|------|------|---------|
| `ThreatTable` | `TMap<APlayerState*, TMap<TWeakObjectPtr<AOnsetEnemy>, float>>` | Threat per player per enemy. Outer key = player, inner key = enemy, value = accumulated threat. `TWeakObjectPtr` prevents keeping dead actors alive. |

### API

```cpp
/** Add threat to an enemy for a specific player. Returns the new total. */
float AddThreat(APlayerState* Player, AOnsetEnemy* Enemy, float Amount);

/** Remove a player from all threat tables (on player death/disconnect). */
void RemovePlayer(APlayerState* Player);

/** Remove an enemy from all threat tables (on NPC death/pool return). */
void RemoveEnemy(AOnsetEnemy* Enemy);

/** Get the primary target (highest-threat player) for an NPC. Returns pawn or null. */
AActor* GetPrimaryTarget(AOnsetEnemy* Enemy) const;

/** Get the threat-sorted rank for a specific player targeting this NPC. 0 = highest threat. */
int32 GetTargetRank(AOnsetEnemy* Enemy, APlayerState* Player) const;

/** Get total number of players with positive threat toward this NPC. */
int32 GetTargetCount(AOnsetEnemy* Enemy) const;

/** Get all NPCs currently aware of a given player (used for damage event routing). */
const TArray<AOnsetEnemy*> GetEnemiesTargeting(APlayerState* Player) const;

/** Clear all tables (on level transition, subsystem cleanup). */
void ClearAll();
```

## Integration Points

### Damage Pipeline

`UOnsetAttributeSet::PostGameplayEffectExecute` already has the instigator from `Data.EffectSpec.GetContext().GetInstigator()`. When `Data.EvaluatedData.Magnitude < 0` (damage taken):

1. Get `Victim` = `Cast<AOnsetEnemy>(GetOwningActor())` — only NPCs track threat
2. Get `InstigatorPawn` = instigator from context
3. Get `PlayerState` = `InstigatorPawn->GetPlayerState()`
4. `Subsystem->AddThreat(PlayerState, Victim, FMath::Abs(Magnitude))`

No change to the death/hit-reaction/hearing flow.

### Perception

No change to how perception sets targets. Perception feeds **initial** awareness (sight → `TargetingComponent->SetTarget`, hearing → noise). Aggro overrides the target when the NPC has threat data.

### NPC Transition from Perception → Aggro

When a perception-detected target is set AND aggro has threat entries, aggro target takes priority. The AgroTask will:

1. Check `GetPrimaryTarget()` — if non-null, use aggro target
2. Fall back to `GetTarget()` (perception target) — if null, clear focus → transition to Idle

### Angular Spread (Bunching Fix)

Attack state computes position using:

```
Count = Subsystem->GetTargetCount(Enemy)
Rank  = Subsystem->GetTargetRank(Enemy, PrimaryTarget->GetPlayerState())
Angle = (Rank / Count) * 360°
Offset = Target->GetActorLocation() 
       + FVector(Cos(Angle), Sin(Angle), 0) * AttackRange
```

Result nav-projected via `UNavigationSystemV1::ProjectPointToNavigation`. This replaces the current random-lateral-offset in ChaseTask and the direct `MoveToActor` in the attack phase.

### Active Enemy Tracking for Taunt

Since threat is stored as `PlayerState → Enemy → Threat`, a taunt ability simply calls `AddThreat(Taunter, Enemy, TauntAmount)` on the subsystem — no special path needed. The highest-threat player automatically becomes the target.

## File Changes Summary

| File | Action | Change |
|------|--------|--------|
| `Source/Onset/Public/AI/AggroSubsystem.h` | **New** | `UOnsetAggroSubsystem` class declaration |
| `Source/Onset/Private/AI/AggroSubsystem.cpp` | **New** | Implementation |
| `Source/Onset/Onset.Build.cs` | Edit | Add `AIModule` dependency if not present |
| `Source/Onset/Public/StateTree/Tasks/Enemy/AgroTask.h/.cpp` | Modify | Check aggro primary target before perception target |
| `Source/Onset/Public/StateTree/Tasks/Enemy/ChaseTask.h/.cpp` | Modify | Replace random offset with aggro-angular offset |
| `Source/Onset/Public/StateTree/Tasks/Enemy/AttackTask.h/.cpp` | Modify | Add `AttackPositionTask` sub-state for aggro-based positioning |
| `Source/Onset/Private/GAS/OnsetAttributeSet.cpp` | Modify | Add aggro feed in `PostGameplayEffectExecute` |
| `Source/Onset/Private/Enemy/OnsetEnemy.cpp` | Modify | `DeferredDeathCleanup` calls `Subsystem->RemoveEnemy(this)` |
| `Source/Onset/Private/Spawning/OnsetPoolSubsystem.cpp` | Modify | `ReturnToPool` calls `Subsystem->RemoveEnemy(Enemy)` |
| `Source/Onset/Public/StateTree/Tasks/OnsetStateTreeTask.h/.cpp` | Modify | Add `GetAggroSubsystem()` and `GetAggroAngularOffset()` helpers |

## StateTree Changes

Current NPC state flow:
```
Idle → Roam → [perception] → Agro → Chase → Attack → Flee → ...
```

New NPC state flow (aggro-aware):
```
Idle → Roam → [perception/aggro] → Agro → Chase → AttackPosition → [distance check] ↔ Attack
```

| State | Change |
|-------|--------|
| Agro | No structural change. Tick checks `GetPrimaryTarget()` first, then `GetTarget()` |
| Chase | Replace current random-lateral-offset with offset = aggro angular position at `ChaseRange` |
| AttackPosition | New sub-state (or merged into existing Attack). Computes angular offset at `AttackRange`, moves there, fires abilities at throttle. Re-evaluates on timer (3s) or player move > 200 units. |
| Attack (current) | Modified to be a "fire abilities" state only — no movement logic. Position handled by AttackPosition. |

## AI LOD Integration

Add to `AOnsetAIController`:

| Distance from player | Tick Enabled | Tick Interval | StateTree |
|---------------------|--------------|---------------|-----------|
| < SightRange | Yes | Normal (0.016s) | Running |
| < ChaseRange * 2 | Yes | Throttled (0.2s) | Running |
| < PerceptionRange * 2 | Yes | Throttled (0.5s) | Running |
| > PerceptionRange * 2 | No | — | Stopped / Paused |

## Staggered Tick Integration

All NPC AI logic is already event-driven (perception, damage). Heavy work is in StateTree ticks and pathfinding.

- **StateTree Interval**: `UStateTreeAIComponent` supports `SetComponentTickInterval()`. Set per-NPC at pool-retrieve time based on LOD tier.
- **Pathfinding**: NPC path cost is driven by `AOnsetAIController` distance. Far NPCs simply don't path (tickless).
- **Aggro subsystem reads**: O(1) map lookups on damage events only. No per-tick overhead.

## Implementation Order

| Step | Task | Files | Est. |
|------|------|-------|------|
| 1 | Create `UOnsetAggroSubsystem` | 2 new files | 1d |
| 2 | Wire damage → aggro feed in `PostGameplayEffectExecute` | 1 modify | 0.5d |
| 3 | Wire pool return + death cleanup | 2 modify | 0.25d |
| 4 | Add helpers to `FOnsetStateTreeTask` base | 1 modify | 0.25d |
| 5 | Modify AgroTask to check aggro target | 2 modify | 0.5d |
| 6 | Create AttackPositionTask with angular spread | 2 files (new) | 1d |
| 7 | Modify ChaseTask offset to use aggro | 2 modify | 0.5d |
| 8 | AI LOD + stagger tick | 2 modify | 1d |
| 9 | Verify + tune | 0.5d |
| | **Total** | | **~5.5d** |

## Edge Cases

- **No threat entries** → falls through to perception target (current behavior). No change to perception-driven flow.
- **Single enemy attacking** → rank = 0, count = 1 → angle = 0° → offset = directly in front of player at AttackRange. Correct behavior for single attacker.
- **Player dies** → `RemovePlayer(DeadPlayerState)` clears all threat entries for that player. NPCs with no remaining threat targets fall back to perception.
- **NPC dies** → `RemoveEnemy(DeadNPC)` in `DeferredDeathCleanup`. Other NPCs targeting the same player re-rank.
- **Pool return** → `RemoveEnemy(NPC)` in `ReturnToPool`. Not strictly required (map key is weak ptr), but keeps table clean.
- **Multiplayer** → one subsystem per world, one threat entry per `PlayerState`. NPCs naturally distribute across party members by damage output.
- **Taunt** → `AddThreat(Taunter, Enemy, TauntAmount)` with a high value. Next Agro/AttackState tick picks the taunter as primary target.
