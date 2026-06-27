# 🗓 THREAT SYSTEM — PRODUCTION PLAN

**Source design doc:** [Threat System](../Docs/AI/Threat_System.md)
**Estimate:** ~5.5 days
**Dependencies:** StateTree, GAS damage pipeline, object pooling
**Status:** ✅ COMPLETE (architecture simplified to EngageTask)

---

## Step 1 — UOnsetThreatSubsystem ✅ COMPLETE

### Actual files
- `Source/Onset/Public/Subsystem/OnsetThreatSubsystem.h`
- `Source/Onset/Private/Subsystem/OnsetThreatSubsystem.cpp`

### Implementation
- [x] Create `UOnsetThreatSubsystem` — inherits `UWorldSubsystem`
- [x] Storage: `TMap<TWeakObjectPtr<AOnsetEnemy>, TMap<TWeakObjectPtr<AOnsetBaseCharacter>, float>> ThreatTable` (pawn-keyed)
- [x] `AddThreat()` — add/subtract, clamp at 0, auto-remove at 0
- [x] `RemovePlayer()` — erase inner key across all enemies
- [x] `RemoveEnemy()` — erase outer key + cleanup engagement
- [x] `GetPrimaryTarget()` — find player with highest threat
- [x] `GetTargetRank()` — sorted threat position
- [x] `GetTargetCount()` — number of players with positive threat
- [x] `ClearAll()` — empty table
- [x] Engagement API: `RegisterEngaged()`, `UnregisterEngaged()`, `GetEngagedCount()`, `GetEngagedIndex()`, `SwitchTarget()`, `IsEnemyEngagedWithPlayer()`
- [x] `GetBestTarget()` — threat × distance scoring (full weight within AttackRange, 0.5× within ChaseRange, 0.1× beyond)

### Verification
- [x] Subsystem creates on world begin
- [x] AddThreat / RemovePlayer / RemoveEnemy work correctly
- [x] GetPrimaryTarget returns correct highest-threat player

---

## Step 2 — Wire Damage Feed ✅ COMPLETE

### Modified files
- `Source/Onset/Private/GAS/OnsetAttributeSet.cpp`

### Implementation
- [x] In `PostGameplayEffectExecute`, when damage taken AND victim is `AOnsetEnemy`: call `AddThreat(Instigator, TargetEnemy, Damage)`
- [x] Also emits `ReportNoiseEvent` for hearing sense (group assist)

### Verification
- [x] Damage dealt to NPC generates threat entry
- [x] Zero-damage effects don't generate threat
- [x] Player death clears threat for that player
- [x] No crash when instigator is not a player

---

## Step 3 — Wire Pool & Death Cleanup ✅ COMPLETE

### Modified files
- `Source/Onset/Private/Enemy/OnsetEnemy.cpp`
- `Source/Onset/Private/Subsystem/OnsetPoolSubsystem.cpp`

### Implementation
- [x] `DeferredDeathCleanup()` — calls `RemoveEnemy(this)`
- [x] `ReturnToPool()` — calls `RemoveEnemy(Enemy)`

### Verification
- [x] NPC death removes its threat entries
- [x] Pool return removes threat entries
- [x] Player re-spawn works with clean threat table

---

## Step 4 — Add Base Helpers ✅ COMPLETE

### Modified files
- `Source/Onset/Public/StateTree/Tasks/OnsetStateTreeTask.h`
- `Source/Onset/Private/StateTree/Tasks/OnsetStateTreeTask.cpp`

### Implementation
- [x] `GetThreatSubsystem()` — returns `UOnsetThreatSubsystem*` via `World->GetSubsystem`
- [x] `GetThreatAngularOffset(Count, Rank, Radius)` — returns `FVector` offset with angle math, wrapped in `FMath::Sin`/`Cos`
- [x] Count=0 guard returns `FVector::ZeroVector`

### Verification
- [x] Helper compiles
- [x] Angular offset produces correct spread (4 enemies → 90° apart)

---

## Step 5–7 — Simplified: Single EngageTask ✅ COMPLETE

**Architecture change:** AgroTask, ChaseTask, AttackTask, AttackPositionTask, IdleTask, RoamTask all deleted. Replaced by two tasks:
- **PatrolTask** — 50/50 idle vs roam in one task
- **EngageTask** — single combat state handling target switching, positioning, ability firing

### EngageTask files
- `Source/Onset/Public/StateTree/Tasks/Enemy/EnemyEngageTask.h`
- `Source/Onset/Private/StateTree/Tasks/Enemy/EnemyEngageTask.cpp`

### Implementation
- [x] `EnterState`: reads `AttackRange`/`ChaseRange` from `UAIProfile` at runtime, gets best target from threat subsystem, registers engagement, sets focus, computes initial angular offset position, paths to it
- [x] `Tick`:
  - [x] Target re-evaluation every 1s via `GetBestTarget()` (threat × distance scoring)
  - [x] Position re-evaluation every 3s or when target moves > 200 units
  - [x] Angular offset at `SpreadRadius` (chase) or `AttackRange` (attack) with dead zone (50u)
  - [x] Crowd avoidance via `UCrowdFollowingComponent`
  - [x] Target lost → 2s timeout then `Succeeded`
  - [x] SwitchTarget called only on actual target change (not on EnterState)
- [x] `ExitState`: `StopMovement()`
- [x] `SetFocus` on EnterState — AI faces target immediately on combat entry
- [x] `ComputeOffsetPosition`: nav-project via `UNavigationSystemV1::ProjectPointToNavigation`

### PatrolTask files
- `Source/Onset/Public/StateTree/Tasks/Enemy/EnemyPatrolTask.h`
- `Source/Onset/Private/StateTree/Tasks/Enemy/EnemyPatrolTask.cpp`

### Verification
- [x] NPC moves to angular offset position with multiple enemies
- [x] 2 NPCs → 180° apart, 3 → 120° apart, 1 → directly in front
- [x] NPC re-positions when player moves or ally dies
- [x] Abilities fire correctly at the offset position
- [x] Navmesh obstruction projects to nearest valid point
- [x] Patrol: 50/50 idle vs roam

---

## Step 8 — AI LOD ✅ COMPLETE

### Modified files
- `Source/Onset/Public/AI/OnsetAIController.h`
- `Source/Onset/Private/AI/OnsetAIController.cpp`

### Implementation
- [x] `UpdateLodTier()` — called every 30 ticks
- [x] Tier 1 (full): within sight range — normal tick
- [x] Tier 2 (throttled): within hearing range — `SetActorTickInterval(0.2f)`
- [x] Tier 3 (paused): beyond — StateTree paused

### Verification
- [x] Far NPCs visibly lag behind
- [x] Very far NPCs stop moving entirely
- [x] NPCs resume on player approach
- [x] No crash on LOD tier transition

---

## Step 9 — StateTree Asset Update ✅ COMPLETE

### Modified asset
- `Content/AI/NewStateTree.uasset`

### Implementation
- [x] 6 top-level subtrees (Patrol, Engage, Investigate, Search, Flee, Lost) — no Selector
- [x] Event-driven transitions between subtrees

### Verification
- [x] StateTree compiles
- [x] Full behaviour loop verified

---

## Step 10 — Verify & Tune ✅ COMPLETE

- [x] PIE test: 1 player, multiple NPCs spread around player during combat
- [x] PIE test: NPC attacks player — stands at AttackRange directly in front
- [x] PIE test: NPC dies mid-combat — remaining NPCs re-rank and re-position
- [x] PIE test: player runs away — NPCs chase using angular offset
- [x] PIE test: Debug logging stripped from EngageTask and ThreatSubsystem
- [x] PIE test: Sight perception adds base threat (1.0) via OnsetAIController
- [x] PIE test: Corpses ignore pawn collision (ECC_Pawn → Ignore)

## Additional Work

- [x] `IsEnemyEngagedWithPlayer()` — query method for engagement safety checks
- [x] `SwitchTarget()` — guards against redundant engagement swaps
- [x] `ClearFocus()` on lost target — AI stops staring at null
- [x] `RegisterEngaged` — fixed duplicate detection with `HasSameIndexAndSerialNumber`
- [x] All debug `UE_LOG` calls stripped from both EnemyEngageTask and OnsetThreatSubsystem
- [x] Subsystem directory migrated from `Subsystems/` (plural) to `Subsystem/` (singular)
