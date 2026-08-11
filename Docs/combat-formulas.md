# ARPG Combat Formula System

**Design pillars:** Real-time ARPG · Power fantasy & satisfying growth · Fast, forgiving fights · Reincarnation/prestige loop · Elemental depth

---

## 1. Core Stats (7 primary)

| Stat | Abbrev. | Role |
|---|---|---|
| Strength | STR | Scales physical damage |
| Intelligence | INT | Scales elemental/magic damage |
| Vitality | VIT | Scales max HP |
| Defense | DEF | Mitigates physical damage |
| Resistance | RES (per element) | Mitigates elemental damage, one value per element |
| Agility | AGI | Attack speed + dodge chance |
| Luck | LUK | Crit chance + crit multiplier |

**Acquisition (hybrid model):**
```
Stat_total = Stat_base + Stat_allocated (points spent on level-up) + Stat_gear (from equipment)
```
Level-up points grow **linearly** (e.g. +3 points/level to distribute). Gear provides the explosive, non-linear part of power growth — this is where "50x stronger" mostly comes from, since gear stat budgets scale steeply with item level/tier and rarity.

---

## 2. Damage Formula

### Physical
```
RawDamage = WeaponBase × (1 + STR / 100)
```

### Elemental
```
RawDamage = SkillBase × (1 + INT / 100)
```

### Variance (±10–20%, recommend ±15% to start)
```
VarianceRoll = Random(0.85, 1.15)
```

### Full damage pipeline (applied in this order)
```
Step 1: Raw = RawDamage × VarianceRoll
Step 2: Mitigated = Raw × (1 - Mitigation%)
Step 3: Elemental = Mitigated × TypeMultiplier   (weakness/resistance table, see §4)
Step 4: Crit = Elemental × (IsCrit ? CritMultiplier : 1)
Step 5: Final = Crit × (1 + BuffDebuffTotal)     (see §6)
```

Keeping these as **discrete multiplicative stages** (mitigation → type → crit → buffs) rather than one giant formula makes each stage independently tunable and easy to debug/log.

---

## 3. Mitigation (Defense & Resistance)

Percentage mitigation with a **built-in soft cap** using the classic "armor formula" (League of Legends / PoE-style):

```
Mitigation% = DEF / (DEF + K)
```

Where `K` is a tuning constant representing "the DEF value at which you hit 50% reduction." This curve is beautiful for your use case because:
- It's naturally percentage-based ✅
- It naturally soft-caps — each additional point of DEF gives less than the last ✅
- It never mathematically reaches 100%, so nothing is ever unkillable

**Same formula per element**, using each element's own RES stat:
```
ElementalMitigation%_fire = RES_fire / (RES_fire + K_elem)
```

**Starting constants to playtest:**
- `K_DEF ≈ 100` at early game scale (tune alongside your DEF point budget per level)
- `K_elem ≈ 80` (slightly lower — elemental should feel a bit more piercing than physical, reinforcing the "elemental system has weaknesses to exploit" fantasy)
- Since stats will reach extreme values (50x+ growth), scale `K` alongside expected DEF ranges per zone tier, or you'll trivialize mitigation late-game. Recommend `K` increasing per zone/tier band (e.g. `K = K_base × ZoneTierMultiplier`).

---

## 4. Elemental Weakness / Resistance (Type Chart)

Separate from the RES **stat** (§3) — this is a **flat multiplier per enemy type**, layered on top:

| Enemy relationship to element | TypeMultiplier |
|---|---|
| Weak to it | 1.5× |
| Neutral | 1.0× |
| Resistant to it | 0.5× |
| Immune | 0× (rare, use sparingly) |

Store this as a simple lookup table: `EnemyType × Element → Multiplier`. Doesn't need to be a full matrix for every enemy — assign 1-2 defined relationships per enemy, default the rest to Neutral (1.0×) to keep content creation manageable.

---

## 5. Critical Hits (Scalable Chance + Multiplier)

Both chance and multiplier scale with LUK, and **both soft-cap** using the same diminishing-returns shape as mitigation:

```
CritChance% = BaseCritChance + (LUK / (LUK + K_crit)) × MaxCritChanceBonus
CritMultiplier = BaseCritMult + (LUK / (LUK + K_critmult)) × MaxCritMultBonus
```

**Recommended starting values:**
- `BaseCritChance = 5%`, `MaxCritChanceBonus = 65%` → hard ceiling ~70% crit chance
- `BaseCritMult = 1.5×`, `MaxCritMultBonus = 2.5×` → hard ceiling ~4.0× crit damage
- Tune `K_crit` / `K_critmult` so mid-game LUK investment feels rewarding but doesn't hit the ceiling until deep into a reincarnation loop

This gives crit-focused builds a real power fantasy (up to 4x burst damage, hitting most of the time) without becoming mathematically guaranteed/broken.

---

## 6. Buffs & Debuffs (Additive Stacking)

All active buffs and debuffs sum into **one combined modifier**, applied once at the end of the damage pipeline:

```
BuffDebuffTotal = Σ(all buff %) - Σ(all debuff %)
Final = PreBuffDamage × (1 + BuffDebuffTotal)
```

Example: +20% damage buff, +15% "Vulnerable" debuff on target, -10% self-debuff → `BuffDebuffTotal = 0.20 + 0.15 - 0.10 = 0.25` → **+25% total**, applied as one multiplication.

This is simpler to reason about and easier to debug than layered multiplicative stacking, at the cost of buffs feeling slightly less individually impactful when many are active — a fair tradeoff given your "easy to tune" and "readable" priorities.

---

## 7. Status Effects (DoTs + Hard CC)

### Damage-over-time (bleed, poison, burn)
```
TickDamage = SourceStat × DoTCoefficient
TotalDoTDamage = TickDamage × NumberOfTicks
```
Each DoT type gets its own coefficient (tunable independently of direct-hit damage) so bleed/poison/burn can have distinct identities (e.g. bleed = many small ticks, burn = fewer big ticks).

### Hard CC (stun, freeze, root)
Binary state flags with a duration, not a formula — but **do** apply diminishing returns on repeated CC to the same target to avoid stun-locking:
```
EffectiveDuration = BaseDuration × (DR_Multiplier)
DR_Multiplier: 1st application = 100%, 2nd within window = 50%, 3rd = 25%, 4th+ = immune until window resets
```
(Borrowed from WoW's diminishing returns system — strongly recommended for any real-time game with hard CC to keep it from feeling unfair.)

---

## 8. Reincarnation / Prestige Scaling

**In-run growth:** linear, as defined in §1 (flat points per level).

**Permanent reincarnation bonus:** compounding multiplier applied globally to all outgoing damage (and optionally HP):

```
PrestigeMultiplier = (1 + r) ^ N
```
Where `r` = bonus per reincarnation (e.g. 0.10 = +10%) and `N` = number of completed reincarnations.

**Harder base difficulty per loop:**
```
EnemyStats_loopN = EnemyStats_base × (1 + d) ^ N
```
Where `d` = difficulty growth per loop (recommend `d` slightly higher than `r`, e.g. `d = 0.15`, so each loop is a genuine challenge increase, not just a formality before the player out-scales it again).

**Tuning target:** by the time a player reaches max level within a single loop, total effective power (base × allocated stats × gear × prestige multiplier) should land around **50×** starting power, per your target. Reaching this is mostly a product of the linear+gear stacking in §1 — the prestige multiplier is a smaller compounding layer on top that makes each *loop* feel meaningfully stronger than the last, not the primary driver within a single loop.

---

## 9. Time-to-Kill Sanity Check

Target: **4–8 hits** to kill a normal enemy early game; **player survives 10+ hits** from a normal enemy.

Quick validation once you have starting numbers, e.g.:
- Player weapon base damage: 20, STR: 10 → RawDamage ≈ 22
- Enemy DEF: 20 → Mitigation% = 20/(20+100) ≈ 16.7% → Mitigated ≈ 18.3
- Enemy HP: ~130 → **≈ 7 hits to kill** ✅ within target range

- Enemy attack: 15 base → Player DEF: 20 → Mitigation ≈ 16.7% → ≈ 12.5 dmg/hit
- Player HP: ~150 → **12 hits to die** ✅ within "10+" forgiving target

Run this kind of spreadsheet check at each major level bracket (start, mid, pre-reincarnation cap) to confirm the curve holds as stats scale.

---

## 10. Tuning Constants — Starting Values Summary

| Constant | Starting Value | Purpose |
|---|---|---|
| `K_DEF` | 100 (scale per zone tier) | Physical mitigation soft cap |
| `K_elem` | 80 (scale per zone tier) | Elemental mitigation soft cap |
| Damage variance | ±15% | Per-hit randomness |
| `BaseCritChance` / `MaxCritChanceBonus` | 5% / 65% | Crit chance curve (cap ~70%) |
| `BaseCritMult` / `MaxCritMultBonus` | 1.5× / 2.5× | Crit damage curve (cap ~4.0×) |
| TypeMultiplier (weak/neutral/resist) | 1.5× / 1.0× / 0.5× | Elemental type chart |
| Prestige bonus `r` | +10% per reincarnation | Permanent power growth |
| Difficulty growth `d` | +15% per reincarnation | Enemy scaling per loop |
| CC diminishing returns | 100% → 50% → 25% → immune | Prevents stun-lock |

---

## 11. Suggested Next Steps

1. Build a spreadsheet implementing §2–§5 so you can plug in stat values and see resulting damage/TTK instantly — this is the fastest way to feel out whether `K` constants and crit curves feel right before writing any game code.
2. Playtest the §9 sanity check at 3–4 points across a single reincarnation loop (start, quarter, mid, near-cap) to confirm TTK targets hold as stats scale non-linearly through gear.
3. Once base loop feels good, playtest a full reincarnation cycle to confirm the `r` vs `d` balance actually makes loop 2 feel harder *and* the permanent multiplier feel earned.
