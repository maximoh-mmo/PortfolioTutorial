# Codebase Review vs. Combat System Plan

**Generated:** 2026-08-21 (Updated after deep code audit)
**Project:** Onset (Unreal Engine C++ Action RPG)
**Repository:** origin/master (3 commits: quests, ui, player)
**Follow-up (2026-08-21):** Constants Refactor + Prestige System **IMPLEMENTED** — see commits below.

---

## Executive Summary

The codebase **substantially implements** the planned Phases 1–8 from the combat system plan. The GAS architecture is well-structured, the data-driven pipeline (abilities, equipment, enemies, quests) is solid, and the recently added quest system is a clean addition.

**Key correction from initial review:** Many "critical gaps" flagged in the first pass are **already implemented** in code. The actual remaining work was narrower and focused on:
1. ~~Moving hardcoded tuning constants to library/ini seams~~ ✅ **DONE** (2026-08-21)
2. ~~Implementing the Prestige tracking system~~ ✅ **DONE** (2026-08-21)
3. Optional: Adding `DT_ElementAffinity` DataTable override (code fallback exists)
4. Zone-tier K scaling (stub only, no consumer)

The codebase is **~95% complete** against the plan (core combat 100%, prestige 100%, zone-tier K deferred).

**Recent commits (2026-08-21):**
- Constants refactor: 11 hardcoded values → `EquipmentLibrary`/`LevelingLibrary` + ini
- Prestige system: quest-gated `PrestigeUp()`, replicated `PrestigeLevel`, persisted across all stores with schema migrations

---

## 🔴 Priority 1 — Actual Critical Gaps (What's Truly Missing) — **ALL RESOLVED**

| # | Finding | Plan Reference | Resolution |
|---|---------|----------------|------------|
| **1.1** | **Prestige system: `PrestigeLevel` never tracked/set** | §2.10 | ✅ **DONE** — `AOnsetPlayerCharacter::PrestigeLevel` (replicated), `PrestigeUp()` quest-gated method, `CombatAttributes->PrestigeMultiplier` applied on prestige up. |
| **1.2** | **Enemy prestige scaling missing** | §2.10 | ✅ **DONE** — Zone prestige = max player prestige in zone (design); enemy spawn reads zone prestige and applies multiplier. |
| **1.3** | **Zone-tier K scaling is a stub** | §2.10 | Deferred — `GetZoneTierKScale()` reads from ini, defaults to 1.0; no zone-tier system exists yet. Tuning seam ready. |

---

## 🟠 Priority 2 — Significant Gaps (Quality / Completeness)

| # | Finding | Plan Reference | Code Location | Impact |
|---|---------|----------------|---------------|--------|
| **2.1** | **`DT_ElementAffinity` DataTable not authored (code fallback exists)** | Phase 4 | `UOnsetAbilityLibrary::GetElementAffinityMultiplier()` has full type chart logic (Fire>Ice>Poison>Lightning>Fire, same=0.5, Physical=1.0). Comment says "DT_ElementAffinity is an optional DataTable override." | Type chart works via code constants; DataTable would allow designer tuning without code changes. Low priority. |
| **2.2** | **Loot system: no zone-tier K scaling on drop rates** | §2.9 | `OnsetLootLibrary::RollLoot` takes `FOnsetLootContext` with `Level`/`ZoneTag` but `KZoneTierScale` not applied. | Loot doesn't scale with zone tier (non-blocking, zone system not built). |
| **2.3** | **Enemy `ElementAffinity` only sets one loose tag** | §2.4 | `ApplyEnemyStats` adds single `Element.*` tag. No type chart lookup needed (code handles it). | Works with code fallback; DataTable would allow per-enemy overrides. |

---

## 🟡 Priority 3 — Architecture / Code Quality (Hardcoded Constants)

| # | Finding | Location | Recommendation |
|---|---------|----------|----------------|
| **3.1** | **`HealthPerVitality = 10.0f` magic number** | `OnsetBaseCharacter.cpp:267` | Move to `OnsetLevelingLibrary` or `OnsetEquipmentLibrary` as tuning constant with ini seam. |
| **3.2** | **`K_DEF = 100`, `K_Elem = 80` hardcoded** | `UOnsetDamageExecution.cpp:55-56` | Move to `OnsetEquipmentLibrary` with ini seam (like `KZoneTierScale`). |
| **3.3** | **Crit constants hardcoded** | `UOnsetDamageExecution.cpp:61-66` | `BaseCritChance=5%`, `MaxCritChance=70%`, `BaseCritMult=1.5`, `MaxCritMult=4.0`, `K_Crit=200`, `K_CritMult=400` → move to library. |
| **3.4** | **`BlockDamageReduction = 0.50f` hardcoded** | `UOnsetDamageExecution.cpp:52` | Move to `OnsetEquipmentLibrary` as tuning constant. |
| **3.5** | **Variance `±15%` hardcoded** | `UOnsetDamageExecution.cpp:81` | Move to library/ini. |
| **3.6** | **`FMath::FRand()` for crit — non-deterministic for replays** | `UOnsetDamageExecution.cpp:151` | Consider seeded RNG or `FMath::RandHelper` for replay consistency. |
| **3.7** | **Dual-wield / mastery CDR constants only in `OnsetEquipmentLibrary`** | `OnsetEquipmentLibrary.h:76-88` | Already in library (good), but verify `GetTotalCooldownReduction()` caps at 80% correctly (line 191 in ability). |

---

## ✅ Corrected: Already Implemented (Previously Flagged as Missing)

The following were flagged as "critical gaps" in the initial review but **are fully implemented**:

| Item | Where Implemented | Notes |
|------|-------------------|-------|
| **Type chart / elemental affinity multiplier** | `UOnsetAbilityLibrary::GetElementAffinityMultiplier()` lines 73-125 | Full cycle: Fire>Ice>Poison>Lightning>Fire; same=0.5; Physical=1.0. Optional DataTable override supported. |
| **Dual-wield CDR** | `UOnsetGameplayAbility::GetTotalCooldownReduction()` lines 174-188 | Melee weapon + empty off-hand → +20% base CDR; DPS class → +15% extra; capped at 80% total. |
| **Bow mastery damage (+15%)** | `UOnsetGameplayAbility::GetSourceWeaponBase()` lines 134-140 | Applied for Ranged class with Bow weapon. |
| **Bow mastery crit (+10%)** | `UOnsetDamageExecution.cpp:146` | Applied in damage execution for Ranged+Bow. |
| **Support mastery potency (+20% buff magnitude)** | `UOnsetGameplayAbility::GetBuffPotency()` lines 145-157; used in `OnsetGA_Generic::ApplyEffect` for Heal (683), Snare (701), Slow (710) | Support class gets 1.2x multiplier on buff/debuff magnitudes. |
| **CC diminishing returns (100%→50%→25%→immune)** | `UOnsetCCDiminishingComponent::GetDiminishedDuration()`; called from `OnsetGA_Generic::GetDiminishedCCDuration()` for Stun/Freeze | Fully functional; 6-second window, per-CC-tag stacks. |
| **Freeze / Slow CC effects** | `OnsetGA_Generic::ApplyEffect` cases for Freeze (729-738) and Slow (707-713) | GE templates exist (`UOnsetGenericFreezeEffect`, `UOnsetCooldownSlowEffect`); applied via data-driven ability rows. |
| **DoT source stat enforcement (STR for Physical, INT for elemental)** | `OnsetGA_Generic::ApplyEffect` lines 650-666 | `SourceStat = STR` for Physical, `INT` for Fire/Ice/Lightning/Poison. |
| **Shield block (50% reduction, before mitigation)** | `UOnsetDamageExecution.cpp:88-95` | `BlockChance` from shield + Tank mastery; `BlockDamageReduction=0.5` applied pre-mitigation. |
| **Enemy cooldown/haste shared with players** | `AOnsetBaseCharacter::GetBaseWeaponArchetype()`; `OnsetGA_BasicAttack::GetCooldownBaseDuration()` | Enemy `EnemyWeaponArchetype` drives same cooldown table. |

---

## 🟢 Priority 4 — Well-Implemented (Matches Plan)

| System | Status | Evidence |
|--------|--------|----------|
| **Core attributes (STR/INT/VIT/DEF/RES/AGI/LUK)** | ✅ Complete | `OnsetCombatAttributeSet.h` — all 12 attributes + derived, replicated |
| **Element tags (Physical/Fire/Ice/Lightning/Poison)** | ✅ Complete | `OnsetGameplayTags.cpp` — tags registered; damage execution iterates all 5 |
| **Damage pipeline v2 (variance → block → mitigation → type chart → crit → buffs)** | ✅ Complete | `UOnsetDamageExecution.cpp` — full pipeline including type chart |
| **Weapon-scaled (STR) vs Skill-scaled (INT) ability math** | ✅ Complete | `GA_BasicAttack` uses Weapon; `GA_Generic` resolves `ScalingType` from definition |
| **Weapon archetype base cooldown table** | ✅ Complete | `GetArchetypeBaseCooldown` matches plan table (0.8–1.8s) |
| **Haste formula (AGI → CDR%)** | ✅ Complete | `GetTotalCooldownReduction()` implements `AGI/(AGI+200)`, capped 80% |
| **Enemy stats via `DT_EnemyStats`** | ✅ Complete | `FOnsetEnemyStats` matches plan; `ApplyEnemyStats` scales by `(1+d)^Tier` |
| **XP / Leveling system (Phase 8)** | ✅ Complete | `OnsetLevelingLibrary` formulas match plan; `GrantXPFromEnemy` with level-diff multiplier; level-up full heal + stat points + `TAG_Event_LevelUp` |
| **Persistence (Inventory, Equipment, Quests, Progression)** | ✅ Complete | JSON serialization, SQLite/HTTP/PgSQL stores, autoplay-safe identity tracking |
| **Quest system (new)** | ✅ Complete | Stages → objectives → auto-advance, generic progress signals, Collect from inventory, Kill from enemy death, ReachLocation, JSON persistence |
| **Equipment model (slots, stat bonuses, shield block/DEF)** | ✅ Complete | `FOnsetEquipmentDefinition`, `RecalculateDerivedStats` sums bonuses, shield adds BlockChance + Tank mastery |

---

## 📋 Summary: What "Landed" vs. What's Missing (Corrected)

| Phase | Plan Status | Actual Status |
|-------|-------------|---------------|
| **1** — Stat foundation + elements + pipeline v2 | ✅ Landed | ✅ **Complete** |
| **2** — Equipment model + weapon/shield + stat aggregation | ✅ Landed | ✅ **Complete** |
| **3** — Cooldown-gating + haste + dual-wield + bow mastery | ✅ Landed | ✅ **Complete** |
| **4** — Type chart + status tie-ins + CC diminishing | ✅ Landed | ✅ **Complete** (type chart in code; DataTable optional) |
| **5** — Crit + buffs/debuffs + Support potency | ✅ Landed | ✅ **Complete** |
| **6** — Mastery + dual-wield + shields + bows | ✅ Landed | ✅ **Complete** |
| **7** — Enemy stats + prestige + zone-tier K | ✅ Landed | ✅ **Complete** — prestige done; zone-tier K stub (deferred) |
| **8** — Experience & leveling | ✅ Landed | ✅ **Complete** |

---

## 🎯 Recommended Next Steps — **ALL DONE (2026-08-21)**

1. ✅ **Move all hardcoded tuning constants to libraries/ini** — DONE (11 constants).
2. ✅ **Implement Prestige tracking system** — DONE (quest-gated `PrestigeUp()`, replicated, persisted).
3. **Optional: Author `DT_ElementAffinity` DataTable** — code fallback complete; only if designers need tuning without code.
4. **Zone-tier K scaling** — deferred; stub harmless.

---

## 🏁 Verdict

The codebase is **~95% complete** against the plan. The core GAS architecture, data-driven equipment/enemy/ability systems, damage pipeline (including type chart, CC-DR, masteries), prestige progression, and persistence are **production-quality**.

Remaining:
- **Zone-tier K scaling** — only when zone-tier system is designed.
- **Optional DataTable** for type chart if designers need it.

**0 new system designs required.**

The quest system is a clean, well-architected addition.

---

*End of corrected review*