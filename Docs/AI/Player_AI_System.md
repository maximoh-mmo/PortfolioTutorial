
## 📘 Player AI System — `/Docs/AI/Player_AI_System.md`

# **Player AI System**

## Purpose
Provide an **autoplay/testing mode** where the player character is controlled by AI instead of human input, useful for stress tests, demos, and debugging.

## Responsibilities
- Control player character via `AOnsetPlayerAIController`
- Use StateTree to decide targeting, movement, and ability usage
- Select targets based on proximity (navmesh-projected distance)
- Trigger abilities intelligently (tag-filtered, throttle-gated)

## Non‑Responsibilities
- Human input handling
- Ability definitions (handled by [GAS System](../GAS/GAS_System.md))
- UI (handled by [UI System](../Gameplay/UI_System.md))

## Key Classes
- **`AOnsetPlayerAIController`** — AI controller for player pawn, inherits `AAIController`, owns `UStateTreeAIComponent`
- **`UOnsetStateTreeSchema`** — same schema class used by NPCs (SelfActor + TargetActor context)
- **`FOnsetStateTreeTaskBase`** — shared base with helpers: `GetTargetingComponent()`, `GetPlayerController()`, `SetTarget()`, `GetTarget()`

## StateTree Tasks
- **`PlayerAcquireTargetTask`** — `EnterState` finds best target via `BestAvailableTarget()`: iterates `GetActiveEnemies()`, filters by `IsAlive()` and leash/acquire range, navmesh-projects, picks nearest. Throttle-gated.
- **`PlayerEngageTask`** — combined approach + attack. `EnterState` validates `IsAlive()` and clears dead targets. `Tick` checks distance: within attack range → `StopMovement` + `SetFocus` + fire abilities (throttled 0.25s, filtered by `TAG_Ability_Attack`); outside → `MoveToActor`. `ExitState` clears focus + stops movement.

## Ability Selection (expected-damage heuristic)
When 2+ abilities are ready, `PlayerEngageTask` scores each candidate and casts the best:

- **Score = `UOnsetGA_Generic::GetComparisonDamage(row)` × hit count**
  - `GetComparisonDamage` mirrors the runtime damage math per DT_Abilities row: direct effects = `Base × (1 + STR|INT/100)` (Base = equipped WeaponBase for Weapon scaling, row Magnitude for Skill), DoT effects = `(STR|INT) × Magnitude × Duration/Period` ticks. Non-damage effects contribute 0.
  - Cached on the ability instance; recomputed only when STR, INT, or WeaponBase drifts (equipment swaps and stat buffs refresh it automatically).
- **Hit count by shape** from one shared overlap query around the primary target:
  - `AoE` → enemies within the row's `Radius` of the target
  - `PointBlankAoE` → enemies within `Radius` of self
  - `Cone` → same set filtered to the row's `Radius` + `ConeHalfAngle` toward the target
  - `SingleTarget` / `Self` → 1
- **Refresh gate (per-caster):** a hostile periodic-damage ability whose authored `FOnsetAbilityDefinition::RefreshTag` is already active on the target — **from this caster** (`UOnsetGA_Generic::HasActivePeriodicInstanceFrom` matches tag + effect instigator) — is dropped from candidacy (a caster's own DoT refreshes rather than stacks; re-casting wastes the cooldown). Another player's live stack of the same DoT does **not** block application, since periodic GEs stack per source (`AggregateBySource`). The tag is granted dynamically on every periodic spec in `ApplyPeriodicEffectSpecToTarget`.
- **Tie-break:** equal score → **longest `CooldownSeconds`** wins, so expensive casts go on cooldown before fast fillers.
- **Fallback:** non-data-driven attacks (`GA_BasicAttack`) enter as zero-score fillers; with a single ready candidate no overlap query runs at all.
- Instantaneous comparison only — no DPS/time averaging, no defense/element mitigation modeling.

## Key Functions
- `EnableAutoCombat()` — UnPossess from player controller, Possess with AI controller, start StateTree
- `DisableAutoCombat()` — UnPossess AI controller, re-Possess with player controller, swap camera via `DelayedSetViewTarget`
- `BestAvailableTarget()` — choose best living enemy target
- `ResetIdleTimer()` / `CancelIdleTimer()` — idle timer on player controller auto-disables autoplay after timeout if no input received

## StateTree Overview
- **Idle** → **AcquireTarget** → **Engage** (combined move + attack)
- No separate Idle → Seek → Move → Attack chain — combined EngageTask handles both movement and ability usage

## Performance Notes
- `PlayerAcquireTargetTask` runs on tick interval (throttle), not every frame
- `PlayerEngageTask` throttles ability activation at `AttackTickInterval = 0.25s`
- Overlap query (`OverlapMultiByChannel`) runs only when 2+ abilities with matching tag are ready; hit counts are filtered from that single query (no per-shape re-queries)
- Comparison-damage cache makes selection O(candidates) float math after warm-up
- No `TSet` allocations per tick
- `GetTargetingComponent()` fast path: NPC cast ~3ns, Player `FindComponentByClass` ~20ns

## bIsAlive Integration
- Player AI targets filtered by `IsAlive()` (`bIsAlive && Health > 0`)
- `EnterState` in EngageTask checks alive status and clears dead targets
- On target death, re-acquisition flows naturally through parent state → AcquireTarget

## Interactions
- **[Player System](../Player/Player_System.md):** possession switching between `AOnsetPlayerController` and `AOnsetPlayerAIController`
- **[GAS](../GAS/GAS_System.md):** triggers abilities via `TryActivateAbilityByClass`, filters by `TAG_Ability_Attack`
- **[UI](../Gameplay/UI_System.md):** debug toggle for autoplay
- **[Targeting System](../Gameplay/Targeting_System.md):** target selection for AI

## Replication
- Player AI logic runs **server‑side** in multiplayer
- Inputs are simulated on server; movement/abilities replicate normally

## Edge Cases
- No valid targets (`IsAlive()` all false) → remains in AcquireTarget, no crash
- Target dies mid‑engage → `EnterState` re-validation catches it next evaluation
- Autoplay toggled mid‑state → `OnUnPossess` stops StateTree, clears focus, stops movement
- Pool-returned NPCs (`bIsAlive = false`) never selected as targets
- Camera handover race condition → `DelayedSetViewTarget` defers by one frame

## Testing Checklist
- [ ] Autoplay toggles cleanly (camera follows, no black frame)
- [ ] Player AI picks reasonable targets (nearest alive enemy)
- [ ] Abilities fire correctly (`TAG_Ability_Attack` filtered)
- [ ] Target re-acquisition on death
- [ ] AI disabled on any input (movement, ability, primary)
- [ ] Works in multiplayer (server‑side AI)
