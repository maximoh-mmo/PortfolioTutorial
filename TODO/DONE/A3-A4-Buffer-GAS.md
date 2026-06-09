# 🗓 FRI 12 JUN — A3 Buffer / Overflow + A4.1 GAS Foundation

## ✅ Done

### A3.2 AI Perception
- [x] Verify perception triggers on player proximity — **confirmed working** (all NPCs acquire and track player target via `OnPerceptionUpdated`)
- [x] Hearing/assist deferred to A4 (requires GAS noise events)

### A3.3 Behaviour States
- [x] **Idle** — timer-based, `FOnsetStateTreeIdleTask`
- [x] **Roam** — nav-reachable territory patrol, `FOnsetStateTreeRoamTask`
- [x] **Agro** — face target via `SetFocus`, facing-angle check, `FOnsetStateTreeAgroTask`
- [x] **Lost** — clear focus, random pause, `FOnsetStateTreeLostTargetTask`
- [x] **Chase** — `MoveToActor`, returns `Succeeded` on arrival, `FOnsetStateTreeChaseTask`
- [x] **Marooned** — leash-broken chase (same `ChaseTask` in asset, no leash transition)
- [x] **Attack stub** — timer-based cooldown, `StopMovement`, placeholder for GAS activation — `FOnsetStateTreeAttackTask`

### Task Base Struct (migration)
- [x] **`FOnsetStateTreeTaskBase`** — static helpers: `GetController`, `GetTarget`, `GetSelfBaseCharacter`, `GetPathFollowingComponent`, `HasMoveCompleted`
- [x] All 5 task structs migrated: Agro, Chase, Idle, LostTarget, Roam

### Transition Condition
- [x] **`FOnsetStateTreeDistanceCondition`** — reusable condition with `DistSquared` + squared threshold, `EComparisonOperator`, `bAllowNoTarget`

### Data Model
- [x] `HomeLocation` moved from `AOnsetEnemy` → `AOnsetBaseCharacter`

### A4.1 GAS Foundation
- [x] **ASC** — `UAbilitySystemComponent` on `AOnsetBaseCharacter` via `CreateDefaultSubobject` (pre-existing)
- [x] **`UOnsetAttributeSet`** — `Health`/`MaxHealth`, clamp in `PostGameplayEffectExecute`, `DOREPLIFETIME_CONDITION_NOTIFY` + `GAMEPLAYATTRIBUTE_REPNOTIFY`, `ATTRIBUTE_ACCESSORS` macro
- [x] **Native GameplayTags** — `UE_DEFINE_GAMEPLAY_TAG` macros for: `Damage.Physical`, `Damage.Magical`, `State.Dead`, `State.Staggered`, `State.Invulnerable`, `Cooldown.Melee`
- [x] **Ability init** — `PossessedBy` override on `AOnsetBaseCharacter` calls `InitAbilityActorInfo(this, this)`
- [x] **`AbilityTargetingLibrary`** — `FAbilityTargetData` renamed to `FOnsetTargetData` (collision with UE5.8 engine type)

## Remaining A3.3
- [x] **Flee** — low health retreat
- [ ] **RoamWander** — no-home stray patrol with timed despawn
- [ ] A3.4 Group Assist (deferred until hearing integration)
- [ ] A3.5 Player AI Autoplay
