# 🏗 Episode 22 — GAS Movement Speed Attribute

**Phase:** 3.5 — Architecture Cleanup
**Prerequisites:** Episode 15 (Basic Attack), Episode 21 (Architecture Cleanup)
**Core teach:** Replace scattered direct `MaxWalkSpeed` writes with a dedicated `UOnsetMovementAttributeSet` and stackable GEs.

---

## The Problem

Three StateTree tasks wrote directly to `UCharacterMovementComponent::MaxWalkSpeed`:

- `FOnsetStateTreeFleeTask` — caches original, multiplier per tick, restores on exit
- `FOnsetStateTreeInvestigateTask` — same cache/restore pattern
- `FOnsetStateTreeSearchTask` — caches but **never restores** (speed leak bug)

Issues:
1. **No stacking** — stagger slow + flee slow clobber each other, last write wins
2. **Scattered responsibility** — every task duplicates cache/restore logic
3. **Bypasses GAS** — effects can't interact with NPC movement speed
4. **Speed leak** — SearchTask's `ExitState` called base method that did nothing, leaving speed permanently multiplied

---

## Solution: `UOnsetMovementAttributeSet` + GE-based Modifiers

Separate dedicated attribute set (industry pattern — Fortnite, Paragon). All speed changes flow through GEs with `MultiplyCompound` operation — they stack multiplicatively by default.

### Stacking

```
   Base MovementSpeed (600)
        │
        ├── Flee GE (×0.35) ─────────────────────────┐
        │                                              │
        ├── Search GE (×0.5) ─────────────────────────┤
        │                                              │
        └── Stagger GE (×0.5) ────────────────────────┤
                                                       │
                    MovementSpeed = 600 × 0.35 × 0.5 × 0.5 = 52.5
                                                       │
                                                       ▼
                                             MaxWalkSpeed = 52.5
```

---

## Implementation

### Files Created

| File | Purpose |
|------|---------|
| `GAS/OnsetMovementAttributeSet.h/.cpp` | New dedicated set with `MovementSpeed` attribute + `PostGameplayEffectExecute` |

### Files Modified

| File | Change |
|------|--------|
| `Player/OnsetBaseCharacter.h/.cpp` | Add `UOnsetMovementAttributeSet* MovementAttributes` subobject |
| `AI/Tasks/OnsetStateTreeTaskBase.h/.cpp` | Add `ApplyMovementSpeedModifier` helper; moved all helpers from inline header to `.cpp` |
| `AI/Tasks/OnsetStateTreeFleeTask.h/.cpp` | Remove `CachedOriginalWalkSpeed`, apply/remove GE by handle |
| `AI/Tasks/OnsetStateTreeInvestigateTask.h/.cpp` | Same refactor as FleeTask |
| `AI/Tasks/OnsetStateTreeSearchTask.h/.cpp` | Same refactor + **fixes speed leak bug** (ExitState now removes GE) |
| `Spawning/OnsetPoolSubsystem.cpp` | `RemoveActiveEffects` in `ReturnToPool` clears leftover GEs |
| `Spawning/OnsetSpawner.cpp` | Added missing `ApplyPerceptionProfile` call |

### Directory Migration

All GAS files moved from `Combat/` to `GAS/`: `OnsetAttributeSet`, `OnsetMovementAttributeSet`, `OnsetGameplayTags`.

### Architecture

- **`UOnsetMovementAttributeSet`** — owns `MovementSpeed` as a replicated attribute
- **`PostAttributeChange`** — clamps ≥ 0, writes to `CharacterMovement->MaxWalkSpeed`
- **Base value** — initialises from CDO default (`InitMovementSpeed(600.0f)`), overridable per BP Class Defaults
- **`ApplyMovementSpeedModifier(Self, Magnitude)`** — shared helper on `FOnsetStateTreeTaskBase`:
  - Creates infinite GE via `NewObject<UGameplayEffect>` (dynamic, no BP asset needed)
  - Sets `MultiplyCompound` on `MovementSpeed` attribute with given magnitude
  - Applies via ASC, returns `FActiveGameplayEffectHandle`
- **Pool cleanup** — `ReturnToPool` calls `RemoveActiveEffects` to clear all GEs, preventing speed leaks across cycles

### Task Refactors

| Task | EnterState | Tick | ExitState |
|------|-----------|------|-----------|
| Flee | Apply GE with health-ratio lerp | Remove old handle, re-apply with updated magnitude | Remove handle |
| Investigate | Apply GE with group/non-group multiplier | N/A (static) | Remove handle |
| Search | Apply GE with `SearchMovementSpeedMultiplier` | N/A (static) | **Remove handle** (was missing — speed leak fix) |

---

## Key Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Dedicated set | `UOnsetMovementAttributeSet` | Industry pattern; separate concerns from combat attributes |
| GE creation | Dynamic via `NewObject` | No BP asset dependency; helper encapsulates all GE setup |
| No speed tags | `FActiveGameplayEffectHandle` only | Removal-by-handle is simpler and sufficient without tag-based queries |
| Modifier op | `MultiplyCompound` | Correct compounding (×0.5 flee × ×0.5 stagger = ×0.25) |
| Pool GE cleanup | `RemoveActiveEffects` in `ReturnToPool` | Best point — not OnPossess (avoids killing buffs during autoplay swaps) |
| No PossessedBy init | CDO default only | BP Class Defaults override for per-class variation; avoids init-ordering issues |
