# Combat System Plan — GAS Implementation Roadmap

Companion to `Docs/combat-formulas.md`. This is the *implementation* plan: the
systems GAS needs, the decisions behind them, and the phased build order. The
formulas doc is the design authority; this doc is how we get there in code.

---

## 1. Current State (baseline)

| Need (formulas) | In code today |
|---|---|
| STR/INT/VIT/DEF/RES/AGI/LUK | None — only `Health/MaxHealth`, `CooldownMultiplier`, `MovementSpeed` |
| Damage pipeline | `UOnsetDamageExecution` = pass-through `(Physical+Magical)×1.0` + invuln gate |
| Weapon/Shield stats | None — `EquipmentJSON`/`InventoryJSON` are empty persisted blobs |
| Elements | Only `Damage.Physical` / `Damage.Magical` tags |
| Crit / mitigation / type chart / buff aggregation | None |
| Enemy stats | None — everyone has hardcoded `MaxHealth = 100`; `AIProfile` is behaviour-only |
| Skill base | `Effect.Magnitude` in `DT_Abilities` (flat); basic attack uses hardcoded `Damage` |
| Stat acquisition / level-up / prestige | None — `SavedMaxHealth` is a single float |

---

## 2. Locked Design Decisions

### 2.1 Core architecture seam
**Abilities compute `Raw`; the execution computes everything after it.**
```
Ability (GA_BasicAttack / GA_Generic)
  Raw = (WeaponBase | SkillBase) × (1 + Stat/Divisor)   ← the only stat math in the ability
  → SetByCaller "Damage.<Element>"                      ← the SetByCaller seam never changes shape

UOnsetDamageExecution (the whole rest of the pipeline)
  Variance → Block → Mitigation → TypeChart → Crit → Buffs → Health
```
DoTs reuse the execution, so ticks get every stage for free.

### 2.2 Elements
**Physical + Fire + Ice + Lightning + Poison** (5). Status tie-ins:
| Element | Status |
|---|---|
| Fire | Burn (DoT) |
| Ice | Freeze / Slow (CC) |
| Lightning | Stun / Shock (CC) |
| Poison | Poison (DoT) |
| Physical | Bleed (DoT) |

- `Damage.Magical` splits into per-element tags.
- One RES attribute per element (`RES_Fire`, `RES_Ice`, `RES_Lightning`, `RES_Poison`).
- **Physical participates in the type chart**.
- Adding elements later is content expansion, not a formula change.

### 2.3 Ability scaling
Every ability carries `ScalingType`:
```
WeaponScaledDamage = WeaponBase × (1 + STR_total / STR_Divisor)   // STR_Divisor = 100
SkillScaledDamage  = SkillBase  × (1 + INT_total / INT_Divisor)   // INT_Divisor = 100
```
- **Basic attack is always Weapon-scaled (STR)**, even for caster weapons.
- `WeaponBase` comes from the equipped weapon's `WeaponDamage`.
- `SkillBase` = the ability's `Effect.Magnitude`.
- Casters get damage from Skill-scaled (INT) abilities; their basic attack is weak but cooldown-gated like everything else.

### 2.4 Damage pipeline (applied in `UOnsetDamageExecution`)
```
Step 1: Raw = BaseDamage(Weapon or Skill scaled) × VarianceRoll (±15%)
Step 2: Block = 1 - (BlockChance × BlockDamageReduction)   [target, before mitigation]
Step 3: Mitigated = Raw × (1 - Mitigation%)                [DEF or RES_x, K per zone tier]
Step 4: Elemental = Mitigated × TypeMultiplier             [1.5 / 1.0 / 0.5 / 0]
Step 5: Crit = Elemental × (IsCrit ? CritMultiplier : 1)   [LUK curves]
Step 6: Final = Crit × (1 + BuffDebuffTotal)               [additive aggregation]
```
Invulnerability gate (`TAG_State_Invulnerable` → 0) stays at the top.

### 2.5 DoTs
```
TickDamage = SourceStat × DoTCoefficient        // STR for Physical/Bleed, INT for Fire/Poison
```
- `DoTCoefficient` = the ability's `Effect.Magnitude`.
- Ticks **flow through the full pipeline** (mitigation, type chart, crit, buffs).

### 2.6 Cooldown-gating + haste (formulas §9)
```
Haste%             = AGI / (AGI + K_haste)              // K_haste = 200
TotalCDR%          = Haste% + DualWieldCDR + MasteryCDR  // capped 80%
EffectiveCooldown  = BaseCooldown × (1 - TotalCDR%) × CooldownMultiplier(slow)
AttacksPerSecond   = 1 / EffectiveCooldown
```
- **Every skill including the basic attack is cooldown-gated** (already true in GAS; the change is the *value* becomes archetype + haste driven).
- Weapon archetype base-cooldown table (§9): Dagger 0.8 / Wand 0.9 / Sword·Bow 1.0 / Axe·Mace 1.1 / Staff 1.2 / Tome 1.3 / Greatsword 1.8.
- The existing `CooldownMultiplier` (Slow debuff) **multiplies on top** of `(1 − TotalCDR%)`.
- The auto-attack 1.5s poll timer becomes a cooldown-echo poller; the GAS cooldown governs rate.
- **Enemies share this model** (archetype from their stats row).

### 2.7 Buffs & debuffs (formulas §10) + Support potency
- All buffs/debuffs sum into one modifier: `BuffDebuffTotal = Σ(buff %) − Σ(debuff %)`.
- Implemented as two additive-stacking attributes: source `OutgoingDamageMod`, target `IncomingDamageMod`; execution uses `(1 + Outgoing + Incoming)`.
- **Support mastery potency**: buff/debuff magnitudes the Support casts are scaled:
  `EffectiveBuffValue = BaseBuffValue × (1 + SupportMasteryPotency)` (potency = +20%).

### 2.8 Class mastery, dual-wield, shields, bows (formulas §11)
Universal equip access; mastery applies only when class matches weapon:

| Existing class | Mastery role | Bonus |
|---|---|---|
| Tank | Tank | Block +DEF (TankMasteryBlock +20%, TankMasteryDEF +20 flat) |
| DPS | MeleeDPS | Dual-wield CDR (+15% on top of DualWieldBaseCDR 20%) |
| Ranged | RangedDPS | Bow crit +10%, bow damage +15% |
| Support | Support | Buff/debuff potency +20% |

- Shields: `BlockChance = BaseBlockChance(10%) + TankMasteryBlock`; `ShieldDEFBonus = BaseShieldDEF(+10) + TankMasteryDEF`; `BlockDamageReduction = 50%`, **applied before DEF/RES mitigation**.
- Dual-wield: `DualWieldCDRBonus = DualWieldBaseCDR(20%) + (MeleeDPS ? +15% : 0)` — reuses the haste math, no new subsystem.

### 2.9 Enemy stats
- New `DT_EnemyStats` DataTable (`MaxHealth, DamageBase, DEF, RES per element, LUK, WeaponArchetype`) — the `Future_Ideas.md` "DataTable Balance Pass".
- Enemies use the **same cooldown/haste attack-rate model** as players.

### 2.10 Prestige + enemy scaling + zone-tier K
- `PrestigeMultiplier = (1 + r)^N` (r ≈ 10%) → global outgoing damage.
- `EnemyStats_loopN = base × (1 + d)^N` (d ≈ 15%, d > r) → applied at spawn.
- `K_DEF` / `K_elem` scale per zone tier alongside expected stat ranges.

### 2.11 Deferred / not in this pass
- **Dodge** (AGI) — no formula/constant yet; defer until pipeline + gear are tuned.
- Level-up stat allocation UX and per-level `+3 points` progression wiring is part of stat aggregation (Phase 2) but the spending UI is out of scope.

---

## 3. Roadmap (7 phases; each ends in a build + PIE check)

### Phase 1 — Stat foundation + elements + pipeline v2
- Extend `UOnsetCombatAttributeSet` with STR, INT, VIT, DEF, RES_Fire/Ice/Lightning/Poison, AGI, LUK + derived (MaxHealth from VIT, block chance, crit inputs).
- Add element tags; split `Damage.Magical` into per-element tags; wire into `OnsetGameplayTags`, the ability editor's `DamageTypeTag`, and `LoadTable`.
- Add `ScalingType` (Weapon/Skill) to `FOnsetAbilityEffect`/`UAbilityCreationData`; divisor math in `GA_Generic` and `GA_BasicAttack`.
- Pipeline v2 in `UOnsetDamageExecution`: variance → block → mitigation → type chart → crit → buffs; keep invuln gate.
- Basic attack: `Damage` hardcode → `WeaponBase` from equipped weapon (Phase 2 dependency; stub with a default weapon first).

### Phase 2 — Equipment model + weapon/shield + stat aggregation
- `FOnsetEquipmentItem`: slot, archetype, WeaponDamage, DamageElement, stat bonuses, block, shield DEF; loadout serialization into the existing `EquipmentJSON`.
- `RecalculateDerivedStats()`: class base + allocated points + gear → attribute set; called on spawn, level-up, equipment change.
- Extend `FOnsetCharacterClassInfo` with base stat values; add stat-allocation persistence to character data.
- Basic attack reads `WeaponBase`; `DT_Abilities` skills get `SkillBase = Effect.Magnitude`.

### Phase 3 — Cooldown-gating + haste
- Weapon archetype base-cooldown table; `Haste%`/`TotalCDR%` (cap 80%).
- `ApplyCooldown` computes `BaseCooldown × (1 − TotalCDR%) × CooldownMultiplier`.
- Basic attack cooldown from equipped weapon; auto-attack timer becomes a cooldown-echo poller.
- Enemies get cooldowns from their archetype.

### Phase 4 — Type chart + status tie-ins + CC diminishing returns
- `DT_ElementAffinity` (enemy type × element → 1.5/1.0/0.5/0); enemy type tags.
- New CC: Freeze (Ice) alongside existing Stun (Lightning) and Slow (Ice); DoTs: Burn/Poison/Bleed (already ride the DOT work).
- CC diminishing-returns tracker (100 → 50 → 25 → immune within a window) applied to durations.

### Phase 5 — Crit + buffs/debuffs aggregation + Support potency
- LUK curves in the execution (`BaseCritChance 5%` / cap ~70%; `BaseCritMult 1.5×` / cap ~4.0×).
- `OutgoingDamageMod` + `IncomingDamageMod` additive attributes; Support potency multiplies buff magnitudes.

### Phase 6 — Mastery + dual-wield + shields + bows
- Class-mastery map (Tank/DPS/Ranged/Support); mastery bonus constants (§11 table).
- Shield block-before-mitigation; dual-wield CDR; bow crit/damage.

### Phase 7 — Enemy stats + prestige + zone-tier K + DPS-TTK
- `DT_EnemyStats` + `(1+d)^N` spawn scaling; `(1+r)^N` outgoing multiplier.
- Zone-tier K scaling for `K_DEF`/`K_elem`.
- Seconds-based TTK validation vs `Docs/combat-tuning-calculator.xlsx` (`TTK = EnemyHP / DPS`).

---

## 4. New tuning constants (add to combat-formulas §14)

| Constant | Starting Value |
|---|---|
| `STR_Divisor` | 100 |
| `INT_Divisor` | 100 |
| `SupportMasteryPotencyBonus` | 0.20 (+20% buff/debuff magnitude) |

Existing §14 constants reused as-is: `K_DEF 100`, `K_elem 80`, variance ±15%, crit 5%/65% and 1.5×/2.5×, type chart 1.5/1.0/0.5/0, prestige `r 10%`, difficulty `d 15%`, CC-DR 100→50→25→immune, `K_haste 200`, max CDR 80%, weapon base cooldowns 0.8–1.8, mastery/dual-wield/block values from §11.

---

## 5. Assumptions
- Basic attacks are always Weapon-scaled (STR), even for caster weapons.
- Physical participates in the type chart.
- DoT source stat: STR for Physical/Bleed, INT for Fire/Poison.
- `Effect.Magnitude` = `SkillBase` for skill-scaled abilities and `DoTCoefficient` for DoTs.