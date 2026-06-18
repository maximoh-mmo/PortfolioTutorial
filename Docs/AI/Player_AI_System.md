
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
- **`PlayerEngageTask`** — combined approach + attack. `EnterState` validates `IsAlive()` and clears dead targets. `Tick` checks distance: within attack range → `StopMovement` + `SetFocus` + fire abilities (throttled 0.25s, filtered by `TAG_Ability_Attack`); outside → `MoveToActor`. On multi-ability readiness, uses overlap query for AoE targeting. `ExitState` clears focus + stops movement.

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
- Overlap query (`OverlapMultiByChannel`) runs only when 2+ abilities with matching tag are ready
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
