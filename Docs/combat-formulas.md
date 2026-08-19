# ARPG Combat Formula System

**Design pillars:** Real-time ARPG · Power fantasy & satisfying growth · Fast, forgiving fights · Reincarnation/prestige loop · Elemental depth · All skills (including basic attacks) are cooldown-gated

---

## 1. Core Stats (7 primary)

| Stat | Abbrev. | Role |
|---|---|---|
| Strength | STR | Scales weapon-based (physical) damage |
| Intelligence | INT | Scales skill-based (elemental) damage |
| Vitality | VIT | Scales max HP |
| Defense | DEF | Mitigates physical damage |
| Resistance | RES (per element) | Mitigates elemental damage, one value per element |
| Agility | AGI | Universal Haste (cooldown reduction) + dodge |
| Luck | LUK | Crit chance + crit multiplier |

**Acquisition (hybrid model):**
```
Stat_total = Stat_base + Stat_allocated (points spent on level-up) + Stat_gear (from equipment)
```
Level-up points grow **linearly**. Gear provides the explosive, non-linear part of power growth — this is where "50x stronger" mostly comes from.

---

## 2. Elements

**Physical + Fire + Ice + Lightning + Poison** (5 total). Each has a natural status-effect identity that ties into §8:

| Element | Status tie-in |
|---|---|
| Fire | Burn (DoT) |
| Ice | Freeze / Slow (CC) |
| Lightning | Stun / Shock (CC) |
| Poison | Poison (DoT) |
| Physical | Bleed (DoT) |

This list drives the RES stat list, the ability tags, and the type chart (§5). Adding more elements later is a content expansion, not a formula change — nothing here assumes exactly five.

---

## 3. Weapon-Scaled vs. Skill-Scaled Abilities

Every ability carries a **ScalingType** flag: `Weapon` (uses `WeaponBase × STR`) or `Skill` (uses `SkillBase × INT`). This is what actually separates melee and caster identity — without it, every ability pulls from the same stat and the classes collapse into one.

```
WeaponScaledDamage = WeaponBaseDamage × (1 + STR_total / STR_Divisor)
SkillScaledDamage  = SkillBaseDamage  × (1 + INT_total / INT_Divisor)
```

Both use the same divisor-based shape, so the *math* is symmetric between STR and INT — balance between melee and caster is a content/itemization/resource problem, not a formula problem (see §9 for how cooldown-gating specifically affects this).

---

## 4. Damage Pipeline

```
Step 1: Raw = BaseDamage(Weapon or Skill scaled, per §3) × VarianceRoll (±10–20%, default ±15%)
Step 2: Mitigated = Raw × (1 - Mitigation%)               (§6)
Step 3: Elemental = Mitigated × TypeMultiplier             (§5)
Step 4: Crit = Elemental × (IsCrit ? CritMultiplier : 1)   (§7)
Step 5: Final = Crit × (1 + BuffDebuffTotal)                (§10)
```

---

## 5. Elemental Weakness / Resistance (Type Chart)

Flat multiplier per enemy-type/element relationship, layered on top of the RES **stat** (§6):

| Relationship | TypeMultiplier |
|---|---|
| Weak | 1.5× |
| Neutral | 1.0× |
| Resistant | 0.5× |
| Immune | 0× (use sparingly) |

Assign 1–2 defined relationships per enemy; default the rest to Neutral.

---

## 6. Mitigation (Defense & Resistance)

Percentage mitigation with a built-in soft cap (armor-formula style):

```
Mitigation% = DEF / (DEF + K_DEF)
ElementalMitigation%_x = RES_x / (RES_x + K_elem)
```

**Starting constants:** `K_DEF ≈ 100`, `K_elem ≈ 80` (elemental pierces mitigation slightly more, reinforcing that elemental damage rewards exploiting weaknesses). Scale `K` per zone tier alongside expected stat ranges.

---

## 7. Critical Hits

```
CritChance% = BaseCritChance + (LUK / (LUK + K_crit)) × MaxCritChanceBonus
CritMultiplier = BaseCritMult + (LUK / (LUK + K_critmult)) × MaxCritMultBonus
```
Starting values: `BaseCritChance = 5%`, cap `~70%`; `BaseCritMult = 1.5×`, cap `~4.0×`.

---

## 8. Status Effects (DoTs + Hard CC)

**DoTs (Bleed/Poison/Burn):**
```
TickDamage = SourceStat × DoTCoefficient
TotalDoTDamage = TickDamage × NumberOfTicks
```
**Hard CC (Stun/Freeze/Root)** — diminishing returns on repeat application to the same target:
```
DR_Multiplier: 1st = 100%, 2nd (within window) = 50%, 3rd = 25%, 4th+ = immune until reset
```

---

## 9. Cooldown-Gated Actions: Weapon Speed & Haste

Since **every** skill — including the basic attack — is cooldown-gated, attack rate is a first-class, tunable stat shared by melee and casters alike. Both use weapons (swords/daggers for melee, staves/wands/tomes for casters), and every weapon archetype has its own **base cooldown**:

| Weapon Archetype | Base Cooldown (sec) |
|---|---|
| Dagger | 0.8 |
| Wand | 0.9 |
| Sword / Bow | 1.0 |
| Axe / Mace | 1.1 |
| Staff | 1.2 |
| Tome | 1.3 |
| Greatsword | 1.8 |

**Universal Haste (AGI)** reduces cooldowns on *all* skills, not just basic attacks — this was a deliberate choice to keep AGI valuable across every build rather than melee-only:
```
Haste% = AGI / (AGI + K_haste)
EffectiveCooldown = BaseCooldown × (1 - TotalCDR%)
AttacksPerSecond = 1 / EffectiveCooldown
```
`TotalCDR%` also folds in dual-wield and mastery bonuses (§11) and should be capped (e.g. 80%) so cooldowns never approach zero.

**Why this matters for melee vs. caster balance:** because both classes' basic attacks are governed by the *same* cooldown+Haste formula, and both damage types use the *same* divisor-shaped scaling formula (§3), the system itself doesn't favor either archetype. The actual balance now lives in: weapon archetype cooldown choices (fast weapons = lower per-hit damage, by design), itemization parity between STR and INT gear, and how mastery bonuses (§11) are tuned — not in a resource-cost asymmetry like mana-vs-free-swings would create.

---

## 10. Buffs & Debuffs (Additive Stacking)

```
BuffDebuffTotal = Σ(all buff %) - Σ(all debuff %)
Final = PreBuffDamage × (1 + BuffDebuffTotal)
```
All buffs/debuffs sum into one modifier, applied once — simpler to reason about than multiplicative layering, at a small cost of buffs feeling slightly less individually punchy when many stack.

---

## 11. Dual-Wielding, Shields & Class Mastery

**Access is universal, not gated.** Any class *can* equip a shield, dual-wield, or carry a bow. What makes a weapon choice "optimal" is a **Class Mastery** bonus that only applies when class and weapon match — this keeps itemization open while still making the intended playstyle statistically best.

**Dual-wielding mechanic (chosen: cooldown reduction, not an extra action):**
```
DualWieldCDRBonus = DualWieldBaseCDR + (Class == MeleeDPS ? MeleeMasteryCDR : 0)
```
This reuses the Haste formula's math — no new subsystem — and gives dual-wield a "fast and frantic" identity without competing against the cooldown-gated skill rotation the way an independent second attack timer would.

**Shields** add Block Chance (a variance-reduction defensive layer, distinct from flat DEF) plus a defensive stat bonus, both boosted for Tanks:
```
BlockChance% = BaseBlockChance + (Class == Tank ? TankMasteryBlock : 0)
ShieldDEFBonus = BaseShieldDEF + (Class == Tank ? TankMasteryDEF : 0)
BlockedDamageMultiplier = 1 - (BlockChance% × BlockDamageReduction%)   [applied before DEF/RES mitigation]
```
Block flattens the damage-taken curve (fewer spikes) rather than just increasing average mitigation — a distinct tanking fantasy from "more DEF."

**Bows** give Ranged DPS extra crit chance and/or damage, on top of the shared physical weapon-scaled formula:
```
RangedBonusCritChance% = (Class == RangedDPS AND Weapon == Bow) ? RangedMasteryCrit : 0
RangedBonusDamage%     = (Class == RangedDPS AND Weapon == Bow) ? RangedMasteryDamage : 0
```

**Caster DPS (Staff)** gets all three offensive axes at once — damage%, crit%, and a distinct mechanic none of the other classes touch: **Elemental Penetration**, which reduces the *enemy's* effective RES rather than boosting the caster's own output:
```
CasterBonusDamage% / CasterBonusCrit%  → added the same way Ranged DPS's bow bonuses are
EffectiveEnemyRES = EnemyRES × (1 - CasterBonusPenetration%)
```
Penetration gives Caster DPS a build-around fantasy of "shredding resistances" rather than just bigger numbers — distinct from Ranged DPS's flat damage/crit stacking.

**Support (Tome)** doesn't deal damage, so it doesn't get a damage/crit/penetration axis at all — it gets **Buff Potency**, which strengthens the buffs/heals *this character casts* by folding a bonus into the additive stacking formula from §10:
```
SupportBuffPotency% = (Class == Support AND Weapon == Tome) ? SupportMasteryPotency : 0
EffectiveBuffContribution% = BaseBuffPercent% × (1 + SupportBuffPotency%)
```
This reuses §10's existing math rather than inventing a new subsystem — the same "Vulnerable +20%" debuff cast by a Support with a Tome contributes more to `BuffDebuffTotal` than the identical debuff cast by anyone else. Note this only affects buff/heal-type skills, not raw damage, so it won't show up in the damage/TTK calculator — it needs its own healing/buff-throughput sheet if you want to tune it numerically.

**Starting mastery values to playtest:**

| Bonus | Value |
|---|---|
| DualWieldBaseCDR | 20% |
| MeleeDPS mastery CDR bonus | +15% |
| BaseBlockChance (shield equipped) | 10% |
| Tank mastery block bonus | +20% |
| BaseShieldDEF | +10 flat |
| Tank mastery DEF bonus | +20 flat |
| RangedDPS mastery crit bonus | +10% |
| RangedDPS mastery damage bonus | +15% |
| CasterDPS mastery damage bonus | +15% |
| CasterDPS mastery crit bonus | +10% |
| CasterDPS mastery penetration bonus | +20% |
| Support mastery buff potency bonus | +25% |
| BlockDamageReduction | 50% of the hit |

**Five classes total:** Melee DPS (dual-wield → CDR), Tank (shield → block+DEF), Ranged DPS (bow → crit+damage), Caster DPS (staff → damage+crit+penetration), Support (tome → buff potency). Each has exactly one weapon that unlocks its mastery — everyone can still equip anything, they just won't get the bonus outside their archetype's match.

---

## 12. Experience & Leveling

> **Status: implemented** (2026-08-18) — formulas below are wired into `UOnsetLevelingLibrary` + `AOnsetPlayerCharacter::GrantXPFromEnemy`/`AddExperience`, config-driven via `[Onset.Gameplay]` (`XPBase`, `XPGrowth`, `LevelCap`, `KillsPerLevel`, `GreyThreshold`, `YellowThreshold`, `BonusXPPerOverLevel`, `MaxBonusXP`, `StatPointsPerLevel`). Kills grant XP via the grey/yellow/green multiplier; crossing a threshold levels the player up (full heal + `StatPointsPerLevel`, persisted to the identity cache write-through). Content still needed: author `Level`/`XpReward` on `DT_EnemyStats` rows. See [Leveling System](../Docs/Player/Leveling_System.md).

**Per-level XP requirement (exponential curve):**
```
XPRequired(Level, Loop N) = XPBase × (1 + XPGrowth)^(Level-1) × (1 + d)^N
```
`XPGrowth` is the per-level compounding rate — this is what drives the "big numbers" feel late-game (by level 300 at the starting constants, a single level requires well over a billion XP). Reusing `d` (the same enemy-difficulty constant from §13 below) means required XP scales with each rebirth loop exactly as enemy toughness does, without a second constant to keep in sync.

**Enemy XP reward — built from the same curve, not a separate one:**
```
EnemyXP(EnemyLevel, Loop N) = XPRequired(EnemyLevel, N) / KillsPerLevel
```
`KillsPerLevel` is the single pacing knob — "how many on-level kills should it take to gain a level." Because both the requirement and the reward scale off the same curve, kills-per-level stays roughly constant across the whole level range regardless of how steep `XPGrowth` is. **This is what decouples "how big the numbers look" from "how long leveling actually takes"** — tune `XPGrowth` for display magnitude, tune `KillsPerLevel` for pacing, independently.

**Level-difference scaling (grey/yellow/green mob mechanic):**
```
LevelDiff = EnemyLevel - PlayerLevel
XPMultiplier =
  0%                                                  if LevelDiff ≤ -GreyThreshold
  ramp 0%→100%                                        if -GreyThreshold < LevelDiff ≤ -YellowThreshold
  100%                                                if -YellowThreshold < LevelDiff ≤ 0
  100% + min(LevelDiff × BonusXPPerOverLevel%, MaxBonusXP%)   if LevelDiff > 0
```
Prevents trivial-content grinding (killing far-under-level enemies for free XP) while rewarding players who fight above their level, capped so it can't be exploited into an infinite-scaling strategy.

**Why rebirth loops naturally get slower — no extra lever needed:**

Because `d > r` (§13 below — enemies scale 15%/loop, the player's permanent multiplier only 10%/loop), real kill-rate slows every loop even though the *formula* pacing (kills-per-level) stays constant. First-order estimate:
```
RelativeSlowdown(N) = ((1+d)/(1+r))^N
DaysToCompleteLoop(N) ≈ DaysToCompleteLoop(0) × RelativeSlowdown(N)
```
This compounds geometrically loop-over-loop — exactly the "slower and slower" feel you want, and it emerges from constants you already have rather than a hand-authored decay curve. Treat it as a planning-stage estimate; validate against real combat-calculator TTK once zone-specific enemy stats exist.

**Pacing calibration (factoring in active + idle/autoplay time):**
```
DailyKillCapacity = (ActiveHours/day × ActiveKillRate) + (IdleHours/day × IdleKillRate)
TotalKillsToCap ≈ LevelCap × KillsPerLevel
ImpliedDaysToCap(Loop 0) = TotalKillsToCap / DailyKillCapacity
```
At the starting constants (2 active hrs/day, 8 idle hrs/day, 200/100 kills/hr respectively, LevelCap 200), `KillsPerLevel = 180` lands loop 0 at exactly 30 days — the companion spreadsheet's Leveling tab lets you re-tune any of these live.

**Starting constants to playtest:**

| Constant | Starting Value | Purpose |
|---|---|---|
| `XPBase` | 50 | XP for Level 1→2 at Loop 0 — sets absolute magnitude |
| `XPGrowth` | 6%/level | Compounding growth — controls "big number" feel |
| `LevelCap` | 200 | Levels per loop |
| `KillsPerLevel` | 180 | Primary pacing knob |
| `GreyThreshold` / `YellowThreshold` | 10 / 5 levels | Grey/yellow mob XP ramp |
| `BonusXPPerOverLevel` / `MaxBonusXP` | 5% / 50% | Over-level risk/reward, capped |
| Active/Idle hours & kill rates | 2hr@200/hr, 8hr@100/hr | Pacing calibration inputs |
| `TargetDaysToRebirth` | 30 | Design goal for Loop 0 |

---

## 13. Reincarnation / Prestige Scaling

```
PrestigeMultiplier = (1 + r) ^ N        (r ≈ 10% per loop)
EnemyStats_loopN = EnemyStats_base × (1 + d) ^ N   (d ≈ 15% per loop, d > r)
```
`d` slightly exceeding `r` ensures each loop is a genuine challenge increase rather than a formality the player instantly out-scales.

---

## 14. Time-to-Kill Validation

With cooldown-gating, **TTK is now measured in seconds (DPS-based), not hit counts** — hit count varies by weapon archetype and haste, so it's no longer a reliable cross-build target. Use:
```
DPS = FinalAverageDamagePerHit × AttacksPerSecond
TimeToKill(sec) = EnemyHP / DPS
```
Hit-count is still useful as a secondary sanity check (original target: 4–8 hits early game, 10+ hits for the player to die), but treat seconds-to-kill as the primary tuning number now that attack rate itself varies by build.

---

## 15. Threat Generation
    Threat is generated at the single choke point where post-mitigation damage lands on an enemy (UOnsetAttributeSet::PostGameplayEffectExecute). It is the damage value scaled by two multiplicative levers — not a separate number authored on the ability:

Threat = FinalDamage × AbilityThreatMultiplier × ClassThreatMultiplier
Multiplier	Source	Default	Purpose
AbilityThreatMultiplier	DT_Abilities (FOnsetAbilityDefinition::ThreatMultiplier)	1.0	Soft-taunt / high-threat abilities; set in the ability creation dialog
ClassThreatMultiplier	DT_ClassInfo (FOnsetCharacterClassInfo::ThreatMultiplier)	1.0 (Tank 1.5)	Class identity — tank holds aggro without out-DPSing DPS

Both clamp ≥ 0. The ability multiplier rides the GameplayEffect context (Context.SetAbility(this) in UOnsetGA_Generic), so DoT ticks inherit the ability's multiplier through the captured spec. Enemy-instigated damage falls through to a 1.0 multiplier — the threat table only tracks player → NPC threat. See Threat System for the full subsystem (target selection, angular spread, taunts).

---

## 16. Tuning Constants — Full Summary

| Constant | Starting Value | Purpose |
|---|---|---|
| `K_DEF` | 100 | Physical mitigation soft cap |
| `K_elem` | 80 | Elemental mitigation soft cap |
| Damage variance | ±15% | Per-hit randomness |
| `BaseCritChance` / `MaxCritChanceBonus` | 5% / 65% | Crit chance curve (cap ~70%) |
| `BaseCritMult` / `MaxCritMultBonus` | 1.5× / 2.5× | Crit damage curve (cap ~4.0×) |
| TypeMultiplier (weak/neutral/resist/immune) | 1.5× / 1.0× / 0.5× / 0× | Elemental type chart |
| Prestige bonus `r` | +10%/loop | Permanent power growth |
| Difficulty growth `d` | +15%/loop | Enemy scaling per loop |
| CC diminishing returns | 100→50→25→immune | Prevents stun-lock |
| `K_haste` | 200 | Universal cooldown-reduction soft cap |
| Max total CDR | 80% | Prevents near-zero cooldowns |
| Weapon base cooldowns | 0.8–1.8 sec | Per weapon archetype, see §9 |
| Dual-wield / mastery bonuses | see §11 table | Class identity layer |
| XP curve / pacing constants | see §12 table | Leveling — XPBase, XPGrowth, KillsPerLevel, etc. |

---

## 17. Suggested Next Steps

1. Use the companion tuning spreadsheet to plug in a build (class, weapon, stats) and check DPS/TTK against targets before writing game code.
2. Playtest all four class archetypes (Melee DPS, Tank, Ranged DPS, Caster) at the same gear/stat budget to confirm mastery bonuses actually differentiate them without making off-archetype weapon choices unviable.
3. Confirm dual-wield's CDR-based identity feels distinct enough from two-handed weapons' higher per-hit damage — if not, consider nudging `DualWieldBaseCDR` or the base cooldowns in §9.
4. Playtest a full reincarnation cycle to confirm `r` vs `d` balance.
5. Use the Leveling tab's pacing calculator to validate `KillsPerLevel` against real playtest kill-rates (active and idle/autoplay) — the 30-day Loop 0 target is a planning estimate until measured against actual player behavior.
