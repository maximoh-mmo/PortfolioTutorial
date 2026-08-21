# Implementation Plan: Constants Refactor + Prestige System

**Generated:** 2026-08-21
**Status:** Ready for execution

---

## Part 1: Hardcoded Tuning Constants → Library/Ini (Priority 3)

### Goal
Move all magic numbers from `UOnsetDamageExecution.cpp` and `OnsetBaseCharacter.cpp` into `UOnsetEquipmentLibrary` / `UOnsetLevelingLibrary` with `DefaultEngine.ini` seams, matching the existing pattern for `KZoneTierScale`, `XpBase`, `XpGrowth`, etc.

### Constants to Move

| Constant | Current Location | Target Library | Ini Key | Default |
|----------|------------------|----------------|---------|---------|
| `HealthPerVitality = 10.0f` | `OnsetBaseCharacter.cpp:267` | `OnsetEquipmentLibrary` | `HealthPerVitality` | 10.0 |
| `K_Defense = 100.0f` | `UOnsetDamageExecution.cpp:55` | `OnsetEquipmentLibrary` | `KDefense` | 100.0 |
| `K_Elemental = 80.0f` | `UOnsetDamageExecution.cpp:56` | `OnsetEquipmentLibrary` | `KElemental` | 80.0 |
| `BaseCritChance = 0.05f` | `UOnsetDamageExecution.cpp:61` | `OnsetLevelingLibrary` | `BaseCritChance` | 0.05 |
| `MaxCritChance = 0.70f` | `UOnsetDamageExecution.cpp:62` | `OnsetLevelingLibrary` | `MaxCritChance` | 0.70 |
| `BaseCritMultiplier = 1.50f` | `UOnsetDamageExecution.cpp:63` | `OnsetLevelingLibrary` | `BaseCritMultiplier` | 1.50 |
| `MaxCritMultiplier = 4.00f` | `UOnsetDamageExecution.cpp:64` | `OnsetLevelingLibrary` | `MaxCritMultiplier` | 4.00 |
| `K_Crit = 200.0f` | `UOnsetDamageExecution.cpp:65` | `OnsetLevelingLibrary` | `KCrit` | 200.0 |
| `K_CritMultiplier = 400.0f` | `UOnsetDamageExecution.cpp:66` | `OnsetLevelingLibrary` | `KCritMultiplier` | 400.0 |
| `BlockDamageReduction = 0.50f` | `UOnsetDamageExecution.cpp:52` | `OnsetEquipmentLibrary` | `BlockDamageReduction` | 0.50 |
| `VarianceRange = 0.15f` (±15%) | `UOnsetDamageExecution.cpp:81` | `OnsetLevelingLibrary` | `DamageVariance` | 0.15 |

### Implementation Steps

#### Step 1.1: Add getters to `UOnsetEquipmentLibrary.h/cpp`
```cpp
// In header (add to class):
static float GetHealthPerVitality();
static float GetKDefense();
static float GetKElemental();
static float GetBlockDamageReduction();

// In cpp (add to internal namespace + getters):
float CachedHealthPerVitality = -1.0f;
float CachedKDefense = -1.0f;
float CachedKElemental = -1.0f;
float CachedBlockDamageReduction = -1.0f;

float UOnsetEquipmentLibrary::GetHealthPerVitality() { ... }
float UOnsetEquipmentLibrary::GetKDefense() { ... }
float UOnsetEquipmentLibrary::GetKElemental() { ... }
float UOnsetEquipmentLibrary::GetBlockDamageReduction() { ... }
```

#### Step 1.2: Add getters to `UOnsetLevelingLibrary.h/cpp`
```cpp
// In header (add to class):
static float GetBaseCritChance();
static float GetMaxCritChance();
static float GetBaseCritMultiplier();
static float GetMaxCritMultiplier();
static float GetKCrit();
static float GetKCritMultiplier();
static float GetDamageVariance();

// In cpp (add to internal namespace + getters):
float CachedBaseCritChance = -1.0f;
// ... etc
```

#### Step 1.3: Update `DefaultEngine.ini`
```ini
[Onset.Gameplay]
; Vitality → Health conversion
HealthPerVitality=10.0

; Mitigation K values (scaled by KZoneTierScale in damage execution)
KDefense=100.0
KElemental=80.0

; Block
BlockDamageReduction=0.50

; Crit curves
BaseCritChance=0.05
MaxCritChance=0.70
BaseCritMultiplier=1.50
MaxCritMultiplier=4.00
KCrit=200.0
KCritMultiplier=400.0

; Damage variance (±)
DamageVariance=0.15
```

#### Step 1.4: Update `UOnsetDamageExecution.cpp`
- Remove all `constexpr` constants
- Call library getters:
  - `K_Defense = UOnsetEquipmentLibrary::GetKDefense() * UOnsetEquipmentLibrary::GetZoneTierKScale()`
  - `K_Elemental = UOnsetEquipmentLibrary::GetKElemental() * UOnsetEquipmentLibrary::GetZoneTierKScale()`
  - `BlockDamageReduction = UOnsetEquipmentLibrary::GetBlockDamageReduction()`
  - `Variance: FMath::FRandRange(1.0f - Variance, 1.0f + Variance)` where `Variance = UOnsetLevelingLibrary::GetDamageVariance()`
  - Crit params from `UOnsetLevelingLibrary` getters

#### Step 1.5: Update `OnsetBaseCharacter.cpp:267`
- Replace `constexpr float HealthPerVitality = 10.0f` with `UOnsetEquipmentLibrary::GetHealthPerVitality()`

#### Step 1.6: Verify build & test
- Build `OnsetEditor Win64 Development`
- Run quick playtest: verify damage numbers, crit rates, block mitigation, variance feel correct
- Check ini overrides work (change a value, restart, verify)

---

## Part 2: Prestige Tracking System (Priority 1)

### Goal
Implement the missing Prestige progression: track `PrestigeLevel` on player, apply `(1+r)^N` outgoing damage multiplier, and scale enemy stats at spawn.

### Design (per combat-formulas §2.10 / §14)
- **Prestige growth rate**: `r = 10%` (`GetPrestigeGrowth()` in EquipmentLibrary)
- **Formula**: `PrestigeMultiplier = (1 + r)^PrestigeLevel`
- **Player**: Gains prestige on reaching LevelCap (200) — resets to Level 1, +1 Prestige
- **Enemy**: Spawns with `PrestigeLevel` based on zone/player prestige; stats multiplied by `GetPrestigeMultiplier(EnemyPrestigeLevel)`
- **Damage execution**: Already reads `SourceCombat->GetPrestigeMultiplier()` (line 162) and applies to outgoing damage

### Implementation Steps

#### Step 2.1: Add `PrestigeLevel` to player persistence
**File:** `Source/Onset/Public/Player/OnsetPlayerCharacter.h`
```cpp
// Add to AOnsetPlayerCharacter:
UPROPERTY(ReplicatedUsing = OnRep_PrestigeLevel, BlueprintReadOnly, Category = "Progression")
int32 PrestigeLevel = 0;

UFUNCTION()
void OnRep_PrestigeLevel();
```

**File:** `Source/Onset/Private/Player/OnsetPlayerCharacter.cpp`
- Initialize `PrestigeLevel = 0` in constructor
- Add replication in `GetLifetimeReplicatedProps`
- In `AddExperience()` when level-up would exceed `LevelCap`:
  ```cpp
  if (Level >= LevelCap) {
      // Prestige up!
      Level = 1;
      Experience = 0;
      PrestigeLevel += 1;
      UnspentStatPoints += GetStatPointsPerLevel(); // or bonus points?
      OnProgressionChanged.Broadcast(Level, Experience);
      OnPrestigeChanged.Broadcast(PrestigeLevel);
      PersistProgression(); // Save new prestige
      // Apply prestige multiplier to CombatAttributes
      if (CombatAttributes) {
          CombatAttributes->SetPrestigeMultiplier(UOnsetEquipmentLibrary::GetPrestigeMultiplier(PrestigeLevel));
      }
  }
  ```

#### Step 2.2: Add `PrestigeMultiplier` to `UOnsetCombatAttributeSet`
**File:** `Source/Onset/Public/GAS/OnsetCombatAttributeSet.h`
```cpp
// Add attribute:
UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_PrestigeMultiplier, Category = "Combat|Prestige")
FGameplayAttributeData PrestigeMultiplier;
ATTRIBUTE_ACCESSORS(UOnsetCombatAttributeSet, PrestigeMultiplier)

UFUNCTION()
void OnRep_PrestigeMultiplier(const FGameplayAttributeData& OldValue);
```

**File:** `Source/Onset/Private/GAS/OnsetCombatAttributeSet.cpp`
- Implement `GetPrestigeMultiplier()` returning `PrestigeMultiplier.GetCurrentValue()`
- Default to 1.0 in `PreAttributeChange` / init

#### Step 2.3: Wire prestige on player load/spawn
**File:** `OnsetPlayerCharacter.cpp` - `ApplyCharacterBuild()` or `PossessedBy()`
```cpp
// After loading progression data:
if (CombatAttributes) {
    CombatAttributes->SetPrestigeMultiplier(UOnsetEquipmentLibrary::GetPrestigeMultiplier(PrestigeLevel));
}
```

#### Step 2.4: Enemy prestige at spawn
**File:** `Source/Onset/Private/Enemy/OnsetEnemy.cpp` (or wherever enemies are spawned)
- Determine enemy prestige level (e.g., zone-based, or match player's prestige)
- Apply to `CombatAttributes`:
  ```cpp
  if (EnemyCombatAttributes) {
      const float Mult = UOnsetEquipmentLibrary::GetPrestigeMultiplier(EnemyPrestigeLevel);
      EnemyCombatAttributes->SetPrestigeMultiplier(Mult);
      // Also scale base stats: Health, Damage, etc. by Mult
  }
  ```

#### Step 2.5: Persist `PrestigeLevel` in player data
**File:** `Source/Onset/Public/Subsystem/OnsetPlayerDataSubsystem.h/.cpp`
- Update `FOnsetPlayerRuntimeData` struct to include `PrestigeLevel`
- Update `UpdateRuntimeProgression()` signature to accept prestige
- Update SQL/HTTP/PgSQL stores to persist the new field
- Update `LoadRuntimeProgression()` to return prestige

#### Step 2.6: UI / Feedback (minimal)
- Add prestige display to HUD/character screen (optional for now)
- Log prestige events: `UE_LOG(LogTemp, Log, TEXT("PRESTIGE: Player reached Prestige %d (multiplier %.2fx)"), PrestigeLevel, Multiplier)`

#### Step 2.7: Verify build & test
- Build
- Playtest: level to cap, verify prestige increments, multiplier applies to damage
- Verify enemy scaling if implemented
- Verify persistence across sessions

---

## Execution Order

| Phase | Task | Est. Effort |
|-------|------|-------------|
| 1 | Add EquipmentLibrary getters + ini keys | 30 min |
| 2 | Add LevelingLibrary getters + ini keys | 30 min |
| 3 | Update DamageExecution.cpp | 30 min |
| 4 | Update OnsetBaseCharacter.cpp | 15 min |
| 5 | Update DefaultEngine.ini | 10 min |
| 6 | Build & verify constants refactor | 15 min |
| **Subtotal: Constants Refactor** | | **~2 hours** |
| 7 | Add PrestigeLevel to PlayerCharacter + replication | 30 min |
| 8 | Add PrestigeMultiplier to CombatAttributeSet | 30 min |
| 9 | Wire prestige on level-up (AddExperience) | 30 min |
| 10 | Wire prestige on load/spawn | 20 min |
| 11 | Enemy prestige at spawn | 30 min |
| 12 | Persist prestige in PlayerDataSubsystem + stores | 45 min |
| 13 | Build & verify prestige system | 30 min |
| **Subtotal: Prestige System** | | **~3.5 hours** |
| **Total** | | **~5.5 hours** |

---

## Verification Checklist

### Constants Refactor
- [ ] All 11 constants moved to libraries
- [ ] `DefaultEngine.ini` has all keys with correct defaults
- [ ] Build succeeds
- [ ] Playtest: damage variance feels ±15%, crit curve matches 5%→70%, block reduces 50%
- [ ] Ini override test: change `KDefense=200`, restart, verify mitigation changes

### Prestige System
- [ ] `PrestigeLevel` replicates correctly
- [ ] Level 200 → Prestige 1, Level resets to 1, XP resets
- [ ] `CombatAttributes->PrestigeMultiplier` = 1.1, 1.21, 1.331... for Prestige 1, 2, 3
- [ ] Damage output increases by multiplier (verify in log)
- [ ] Prestige persists across session reload
- [ ] Enemy prestige scales stats (if implemented)
- [ ] No regression in non-prestige gameplay

---

## Notes

- **No new DataTables needed** — all values are scalar tuning constants
- **Backward compatible** — ini defaults match current hardcoded values
- **Prestige is opt-in for enemies** — can start with player-only, add enemy scaling later
- **Existing `GetPrestigeMultiplier(int32)` in EquipmentLibrary is the single source of truth** for the formula

---

*End of plan*