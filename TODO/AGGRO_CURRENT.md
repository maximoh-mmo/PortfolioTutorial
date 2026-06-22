# 🗓 AGGRO SYSTEM — PRODUCTION PLAN

**Source design doc:** [Aggro System](../Docs/AI/Aggro_System.md)
**Estimate:** ~5.5 days
**Dependencies:** StateTree, GAS damage pipeline, object pooling

---

## Step 1 — UOnsetAggroSubsystem (day 1, ~1d)

### New files
- `Source/Onset/Public/AI/AggroSubsystem.h`
- `Source/Onset/Private/AI/AggroSubsystem.cpp`

### Implementation
- [ ] Create `UOnsetAggroSubsystem` — inherits `UWorldSubsystem`
- [ ] Storage: `TMap<APlayerState*, TMap<TWeakObjectPtr<AOnsetEnemy>, float>> ThreatTable`
- [ ] `AddThreat(APlayerState* Player, AOnsetEnemy* Enemy, float Amount)` — add/subtract, clamp at 0, auto-remove at 0
- [ ] `RemovePlayer(APlayerState* Player)` — erase outer key (on player death)
- [ ] `RemoveEnemy(AOnsetEnemy* Enemy)` — iterate all players, erase inner key (on NPC death/pool return)
- [ ] `GetPrimaryTarget(AOnsetEnemy* Enemy)` — find player with highest threat to this enemy, return their pawn
- [ ] `GetTargetRank(AOnsetEnemy* Enemy, APlayerState* Player)` — sorted threat position (0 = highest)
- [ ] `GetTargetCount(AOnsetEnemy* Enemy)` — number of players with positive threat
- [ ] `ClearAll()` — empty table (level transition)

### Verification
- [ ] Subsystem creates on world begin
- [ ] AddThreat / RemovePlayer / RemoveEnemy work correctly
- [ ] GetPrimaryTarget returns correct highest-threat player

---

## Step 2 — Wire Damage Feed (day 2, ~0.5d)

### Modified files
- `Source/Onset/Private/GAS/OnsetAttributeSet.cpp`

### Implementation
- [ ] In `PostGameplayEffectExecute`, when `Data.EvaluatedData.Magnitude < 0` (damage taken) AND victim is `AOnsetEnemy`:
  - [ ] Get instigator pawn via `Data.EffectSpec.GetContext().GetInstigator()`
  - [ ] Get `PlayerState` via `InstigatorPawn->GetPlayerState()`
  - [ ] Get subsystem: `GetWorld()->GetSubsystem<UOnsetAggroSubsystem>()`
  - [ ] Call `Subsystem->AddThreat(PlayerState, VictimEnemy, FMath::Abs(Damage))`

### Verification
- [ ] Damage dealt to NPC generates threat entry
- [ ] Zero-damage effects (heals) don't generate threat
- [ ] Player death clears threat for that player
- [ ] No crash when instigator is not a player (NPC-vs-NPC death)

---

## Step 3 — Wire Pool & Death Cleanup (day 2, ~0.25d)

### Modified files
- `Source/Onset/Private/Enemy/OnsetEnemy.cpp`
- `Source/Onset/Private/Spawning/OnsetPoolSubsystem.cpp`

### Implementation
- [ ] `OnsetEnemy::DeferredDeathCleanup()` — call `Subsystem->RemoveEnemy(this)` before `OwningSpawner->OnNPCDeath(this)`
- [ ] `PoolSubsystem::ReturnToPool()` — call `Subsystem->RemoveEnemy(Enemy)` after existing clean-up

### Verification
- [ ] NPC death removes its threat entries
- [ ] Pool return removes threat entries
- [ ] Player re-spawn works (new NPCs fresh from pool = clean threat table)

---

## Step 4 — Add Base Helpers (day 2, ~0.25d)

### Modified files
- `Source/Onset/Public/StateTree/Tasks/OnsetStateTreeTask.h`
- `Source/Onset/Private/StateTree/Tasks/OnsetStateTreeTask.cpp`

### Implementation
- [ ] Add `GetAggroSubsystem()` — returns `UOnsetAggroSubsystem*` via `World->GetSubsystem`
- [ ] Add `GetAggroAngularOffset(int32 Count, int32 Rank, float Radius)` — returns `FVector(Cos(Rank/Count angle), Sin, 0) * Radius`
  - Static helper, doesn't need context
  - Wrap angle math in `FMath::Sin` / `FMath::Cos`
  - `Count = 0` guard (return `FVector::ZeroVector`)

### Verification
- [ ] Helper compiles
- [ ] `GetAggroAngularOffset(4, 0, 250)` returns `(250, 0, 0)` (in front)
- [ ] `GetAggroAngularOffset(4, 1, 250)` returns `(0, 250, 0)` (right)
- [ ] `GetAggroAngularOffset(0, 0, 250)` returns zero vector

---

## Step 5 — Modify AgroTask (day 3, ~0.5d)

### Modified files
- `Source/Onset/Public/StateTree/Tasks/Enemy/AgroTask.h`
- `Source/Onset/Private/StateTree/Tasks/Enemy/AgroTask.cpp`

### Implementation
- [ ] `Tick`: before checking `GetTarget()`, call `GetAggroSubsystem()->GetPrimaryTarget(SelfEnemy)`
  - [ ] If non-null: use aggro target (set as focus, set via `SetTarget()`)
  - [ ] If null: fall back to `GetTarget()` (perception target, current behaviour)
- [ ] `EnterState`: same logic — set focus on aggro target if available

### Verification
- [ ] NPC with threat focuses highest-threat player
- [ ] NPC with no threat uses perception target (unchanged)
- [ ] NPC transitions to Chase as normal after facing target

---

## Step 6 — AttackPositionTask (day 3-4, ~1d)

### New files
- `Source/Onset/Public/StateTree/Tasks/Enemy/AttackPositionTask.h`
- `Source/Onset/Private/StateTree/Tasks/Enemy/AttackPositionTask.cpp`

### Implementation
- [ ] `FAttackPositionTask` — inherits `FOnsetStateTreeTask`
- [ ] Instance data: `AttackRange`, `ReevaluateInterval` (3s), `MoveThreshold` (200), `AbilityClass`, timer
- [ ] `EnterState`:
  - [ ] Get aggro primary target via subsystem
  - [ ] Get rank + count via subsystem
  - [ ] Compute offset via `GetAggroAngularOffset(Count, Rank, AttackRange)`
  - [ ] Nav-project via `UNavigationSystemV1::ProjectPointToNavigation`
  - [ ] `MoveToLocation(ProjectedLocation, AcceptanceRadius)`
  - [ ] Cache target location for distance check
- [ ] `Tick`:
  - [ ] If target moved > `MoveThreshold` or elapsed > `ReevaluateInterval`:
    - [ ] Recompute offset + re-path
    - [ ] Reset timer
  - [ ] Fire ability at throttle (same 0.25s as PlayerEngageTask)
  - [ ] On ability fired, filter by `TAG_Cooldown_BasicAttack`
  - [ ] If target lost → `Succeeded`
- [ ] `ExitState`:
  - [ ] `StopMovement()`
  - [ ] `ClearFocus()`

### Verification
- [ ] NPC moves to angular offset position
- [ ] 2 NPCs → 180° apart
- [ ] 3 NPCs → 120° apart
- [ ] 1 NPC → directly in front at AttackRange
- [ ] NPC re-positions when player moves
- [ ] NPC re-positions when an ally dies (count changes)
- [ ] Abilities fire correctly at the offset position
- [ ] Navmesh obstruction → NPC projects to nearest valid point

---

## Step 7 — Modify ChaseTask (day 4-5, ~0.5d)

### Modified files
- `Source/Onset/Public/StateTree/Tasks/Enemy/ChaseTask.h`
- `Source/Onset/Private/StateTree/Tasks/Enemy/ChaseTask.cpp`

### Implementation
- [ ] `EnterState`: replace current random-lateral-offset computation
  - [ ] Get aggro primary target
  - [ ] Get rank + count
  - [ ] Compute angular offset at `ChaseRange` (or `AttackRange * 1.5` if no chase-range threshold)
  - [ ] Nav-project → `MoveToLocation`

### Verification
- [ ] Chase movement goes to angular offset position, not random
- [ ] Multiple chasers spread around target during approach
- [ ] Fallback to random offset if no aggro data (perception-only NPC)

---

## Step 8 — AI LOD + Staggered Ticks (day 5, ~1d)

### Modified files
- `Source/Onset/Public/Enemy/OnsetAIController.h`
- `Source/Onset/Private/Enemy/OnsetAIController.cpp`

### Implementation
- [ ] Add `AOnsetAIController::Tick(float DeltaTime)` override
- [ ] On tick, compute distance to nearest player:
  - [ ] If < `SightRange`: `SetComponentTickInterval(0.0f)` (normal)
  - [ ] If < `ChaseRange * 2`: `SetComponentTickInterval(0.2f)` (throttled)
  - [ ] If < `PerceptionRange * 2`: `SetComponentTickInterval(0.5f)` (half-rate)
  - [ ] If > `PerceptionRange * 2`: pause StateTree + disable perception tick
  - [ ] Re-evaluate tier every 30 ticks (avoid per-frame distance check spam)

### Verification
- [ ] Far NPCs visibly lag behind (throttled ticks)
- [ ] Very far NPCs stop moving entirely
- [ ] NPCs resume normal behaviour on player approach
- [ ] No crash on LOD tier transition
- [ ] No visible pop when tier changes

---

## Step 9 — StateTree Asset Update (day 5, ~0.25d)

### Modified asset
- `Content/AI/ST_NPC_Base` (StateTree BP)

### Implementation
- [ ] Wire `AttackPositionTask` between Chase and Attack:
  - `Chase → [OnCompleted, distance ≤ AttackRange] → AttackPosition → [Tick, if InAttackRange] → Attack → [Tick, cooldown expired] → AttackPosition`
- [ ] Remove direct Chase → Attack transition (now goes through AttackPosition)
- [ ] Add `HasNoTarget` guard on AttackPosition → if target lost mid-positioning, transition to Idle/Lost

### Verification
- [ ] StateTree compiles
- [ ] NPC flows: Agro → Chase → AttackPosition → Attack → AttackPosition (loop)
- [ ] NPC flows: AttackPosition → LostTarget on target death
- [ ] NPC flows: AttackPosition → Chase on target leaving attack range

---

## Step 10 — Verify & Tune (day 5.5, ~0.5d)

- [ ] PIE test: 1 player, 5 NPCs spawning in wave → all spread around player during combat
- [ ] PIE test: 1 NPC attacks player → stands at AttackRange directly in front → correct
- [ ] PIE test: NPC dies mid-combat → remaining NPCs re-rank and re-position
- [ ] PIE test: player runs away → NPCs chase using angular offset → re-acquire on arrival
- [ ] PIE test: 2 players, 5 NPCs → NPCs distribute across both players by who dealt most damage
- [ ] PIE test: Aggro LOD → far NPCs visibly throttle, resume on approach
- [ ] Profile: no per-tick allocations, no TSet allocations, no subsystem O(n) scans
