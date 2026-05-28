## 📘 Ability Targeting System — `/Docs/Gameplay/Ability_Targeting_System.md`

# **Ability Targeting System** *(planned — not yet implemented)*

## Purpose
Provide a unified way to produce targeting data for abilities — single‑target, AoE, and directional — by reading from the **TargetingComponent's current target** rather than re‑raycasting from cursor.

## Responsibilities *(planned)*
- Read current target from `UTargetingComponent`
- Populate `FAbilityTargetData` with actor, world location, and direction
- Provide target data to GAS for ability execution

## Non‑Responsibilities
- Target selection (handled by `UTargetingComponent` via `IsActorTargetValid`)
- Per-ability parameters (radius, range — set per `UGameplayAbility` subclass in A4)
- Ability execution (handled by [GAS System](../GAS/GAS_System.md))
- AI decision making
- [UI System](UI_System.md) outside of targeting indicators

## Key Types *(planned)*
- **`FAbilityTargetData`** — struct holding `TargetActor`, `TargetLocation`, `TargetDirection`
- **`UAbilityTargetingLibrary`** — static utility class with `GetTargetData()`

## Key Functions *(planned)*
- `UAbilityTargetingLibrary::GetTargetData(TargetingComponent, SourceActor)` — returns `FAbilityTargetData` for current target
  - `TargetActor` → `TargetingComponent->GetTarget()`
  - `TargetLocation` → target actor's location
  - `TargetDirection` → `(TargetLocation - SourceActorLocation).GetSafeNormal()`

## Data Flow *(planned)*
```
UTargetingComponent (CurrentTarget)
        │
        ▼
UAbilityTargetingLibrary::GetTargetData()
        │
        ▼
FAbilityTargetData → GAS Ability → Execution
```

## Interactions *(planned)*
- **[Targeting System](Targeting_System.md):** source of the current target
- **[Player System](../Player/Player_System.md):** ability input triggers `GetTargetData()` call
- **[NPC AI System](../AI/NPC_AI_System.md):** NPCs use same library with their `UTargetingComponent` + pawn
- **[GAS System](../GAS/GAS_System.md):** consumes `FAbilityTargetData` for execution

## Replication *(planned)*
- Targeting data production is **client‑side** (reads local `CurrentTarget`)
- `FAbilityTargetData` is sent with ability activation RPC
- Server re-validates target data before applying effects

## Edge Cases *(planned)*
- No current target (`GetTarget()` returns null) — empty struct returned
- SourceActor is null — direction cannot be computed, falls back to `ForwardVector`
- Target moves between selection and activation — ability re-validates on server

## Testing Checklist *(planned)*
- [ ] `GetTargetData()` returns correct data when target is set
- [ ] `GetTargetData()` returns empty data when no target is set
- [ ] Direction is correctly computed from source to target  