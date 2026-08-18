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
| BlockDamageReduction | 50% of the hit |

---

## 12. Reincarnation / Prestige Scaling

```
PrestigeMultiplier = (1 + r) ^ N        (r ≈ 10% per loop)
EnemyStats_loopN = EnemyStats_base × (1 + d) ^ N   (d ≈ 15% per loop, d > r)
```
`d` slightly exceeding `r` ensures each loop is a genuine challenge increase rather than a formality the player instantly out-scales.

---

## 13. Time-to-Kill Validation

With cooldown-gating, **TTK is now measured in seconds (DPS-based), not hit counts** — hit count varies by weapon archetype and haste, so it's no longer a reliable cross-build target. Use:
```
DPS = FinalAverageDamagePerHit × AttacksPerSecond
TimeToKill(sec) = EnemyHP / DPS
```
Hit-count is still useful as a secondary sanity check (original target: 4–8 hits early game, 10+ hits for the player to die), but treat seconds-to-kill as the primary tuning number now that attack rate itself varies by build.

---

## 14. Tuning Constants — Full Summary

| Constant | Starting Value | Purpose |
|---|---|---|
| `K_DEF` | 100 | Physical mitigation soft cap |
| `K_elem` | 80 | Elemental mitigation soft cap |
| `STR_Divisor` / `INT_Divisor` | 100 | Stat scaling soft cap — +100 stat doubles the weapon/skill base (see §3) |
| `SupportMasteryPotencyBonus` | +20% | Buff/debuff magnitude bonus for the Support class (`UOnsetGameplayAbility::GetBuffPotencyMultiplier`) |
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
| Class threat multiplier | 1.0 (Tank 1.5 in content) | Tank identity — see §15 |

---

## 15. Threat Generation

Threat is generated at the single choke point where post-mitigation damage lands on an enemy (`UOnsetAttributeSet::PostGameplayEffectExecute`). It is the damage value scaled by two multiplicative levers — **not** a separate number authored on the ability:

```
Threat = FinalDamage × AbilityThreatMultiplier × ClassThreatMultiplier
```

| Multiplier | Source | Default | Purpose |
|---|---|---|---|
| `AbilityThreatMultiplier` | `DT_Abilities` (`FOnsetAbilityDefinition::ThreatMultiplier`) | 1.0 | Soft-taunt / high-threat abilities; set in the ability creation dialog |
| `ClassThreatMultiplier` | `DT_ClassInfo` (`FOnsetCharacterClassInfo::ThreatMultiplier`) | 1.0 (Tank 1.5) | Class identity — tank holds aggro without out-DPSing DPS |

Both clamp ≥ 0. The ability multiplier rides the GameplayEffect context (`Context.SetAbility(this)` in `UOnsetGA_Generic`), so DoT ticks inherit the ability's multiplier through the captured spec. Enemy-instigated damage falls through to a 1.0 multiplier — the threat table only tracks player → NPC threat. See [Threat System](AI/Threat_System.md) for the full subsystem (target selection, angular spread, taunts).

---

