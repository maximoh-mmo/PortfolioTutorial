# 📘 **GAS SYSTEM DOCUMENT**  

---

# **GAS System**

## **Purpose**
Provide ability execution and attribute modification with a data-driven damage pipeline, PvP‑aware damage filtering, and threat feeding.

---

## **Responsibilities**
- Execute abilities (data-driven via `DT_Abilities` + `UOnsetGA_Generic`)
- Apply GameplayEffects (damage, heal, DoT/HoT, snare, slow, stun, freeze, invulnerable, cooldown)
- Run the full damage pipeline (variance → block → mitigation → type chart → crit → buffs) in `UOnsetDamageExecution`
- Replicate attribute changes
- Enforce PvP damage rules
- Feed damage into the [Threat System](../AI/Threat_System.md) (scaled by ability × class threat multipliers)

---

## **Non‑Responsibilities**
- UI rendering (handled by [UI System](../Gameplay/UI_System.md))  
- AI decision‑making (handled by [NPC AI System](../AI/NPC_AI_System.md))  
- Target selection (handled by [Targeting System](../Gameplay/Targeting_System.md))  
- Items/equipment storage (handled by [Inventory & Loot System](../Inventory/Inventory_System.md))  

---

## **Ability Flow Diagram**

```mermaid
flowchart TD
    InputOrAI[Input or AI Decision] --> ASC[AbilitySystemComponent]
    ASC --> Ability[GameplayAbility<br/>UOnsetGA_Generic resolves DT_Abilities row]
    Ability --> TargetData
    TargetData --> Execution[UOnsetDamageExecution<br/>variance → block → mitigation → type chart → crit → buffs]
    Execution --> Attributes[AttributeSet]
    Attributes --> Threat[AddThreat: |damage| x abilityMult x classMult]
    Attributes --> DeathOrHit{Health <= 0?}
    DeathOrHit -->|No| HitReaction[Hit Reaction via TAG_Event_HitReaction]
    DeathOrHit -->|Yes| DeathFork[Death Fork]
    DeathFork --> PoolReturn[ReturnToPool]
    DeathFork --> CorpseSpawn[Corpse Actor Spawn]
    DeathFork --> RespawnTimer[Spawner Respawn Timer]
```

Transient gameplay events are pulsed via loose gameplay tags (add → remove on the ASC), e.g. `TAG_Event_HitReaction` (hit reaction) and `TAG_Event_LevelUp` (broadcast on a level-up, `Event.LevelUp`), so abilities can react to them without polling. See [Leveling System](../Player/Leveling_System.md).

---

## **PvP Damage Filtering**

### In Damage Execution Calculation:
```
if (Source is Player && Target is Player)
{
    if (!SourcePlayerState->bIsPvPEnabled)
    {
        // Block damage
        OutDamage = 0;
        return;
    }
}
```

### Applies to:
- Single‑target abilities  
- AoE abilities  
- Directional abilities  
- Projectile abilities  

### Does NOT apply to:
- NPC → Player damage  
- Player → NPC damage  

---

## **Targeting Integration**
If [Targeting System](../Gameplay/Targeting_System.md) rejects a player target due to PvP, GAS never receives invalid target data.

---

## **Key Classes**

- **`UAbilitySystemComponent`** — executes abilities, manages tags and effects  
- **`UOnsetGameplayAbility`** — base class for all abilities; owns cooldown application, buff/debuff potency (Support mastery), threat multiplier API  
- **`UOnsetGA_Generic`** — the data-driven runtime ability. Resolves its `DT_Abilities` row from the spec's `AbilityID.<RowName>` dynamic tag, dispatches on `EOnsetAbilityType` (SingleTarget / AoE / PointBlankAoE / Cone / Self), applies each `FOnsetAbilityEffect`, executes optional caster movement (Leap), and stamps `SetAbility(this)` on outgoing effect contexts so the attribute set can resolve the ability's threat multiplier  
- **`UGameplayEffect`** — base; project subclasses: `UOnsetGenericDamageEffect`, `UOnsetGenericDamageOverTimeEffect`, `UOnsetGenericHealEffect`, `UOnsetGenericHealOverTimeEffect`, `UOnsetGenericSnareEffect`, `UOnsetCooldownSlowEffect` (Slow debuff), `UOnsetGenericStunEffect`, `UOnsetGenericFreezeEffect`, `UOnsetGenericInvulnerableEffect`, `UOnsetGenericCooldownEffect`  
- **`UOnsetDamageExecution`** — `UGameplayEffectExecutionCalculation` implementing the full damage pipeline (see below)  
- **`UOnsetAttributeSet`** — combat attributes `Health` / `MaxHealth` — lives at `GAS/`  
- **`UOnsetCombatAttributeSet`** — derived combat stats: `Strength`, `Intellect`, `Vitality`, `Defense`, `ResistanceFire/Ice/Lightning/Poison`, `Agility`, `Luck`, `BlockChance`, `CooldownMultiplier`, `OutgoingDamageMod`, `IncomingDamageMod`, `PrestigeMultiplier`. `RecalculateDerivedStats` (Phase 2) overrides base values from the class row + gear. Lives at `GAS/`  
- **`UOnsetMovementAttributeSet`** — `MovementSpeed` with own `PostAttributeChange` — clamps ≥ 0 and syncs `MaxWalkSpeed` on `GetCharacterMovement()`. All speed modifiers apply via `MultiplyCompound` GEs.  
- **`UOnsetCCDiminishingComponent`** — per-character CC diminishing-returns tracker (`100% → 50% → 25% → immune` within a 6s window) applied to Stun/Freeze durations before the GE is applied. Lives at `Core/`  
- **`UOnsetAbilityLibrary`** — loads/caches `DT_Abilities` (config seam `AbilityDataTable`), resolves definitions by row/ID tag, maps elements to tags, and holds the element-affinity type chart (`GetElementAffinityMultiplier`).  
- **`UOnsetEquipmentLibrary`** — weapon base from equipped gear, class base stats, archetype base cooldowns, mastery constants (Tank/DPS/Ranged/Support), enemy difficulty + prestige growth, `DT_EnemyStats`. See [Inventory & Loot System](../Inventory/Inventory_System.md).  
- **`UOnsetLootLibrary`** — rolls `DT_Loot` on death. See [Inventory & Loot System](../Inventory/Inventory_System.md).  

---

## **Damage Pipeline** (`UOnsetDamageExecution`)

Comprehensive pipeline per combat-formulas §4–§7. SetByCaller magnitudes per element (`Damage.Physical/Fire/Ice/Lightning/Poison`); abilities never bake damage values into GE assets.

```
Step 1  Raw       = Σ(element SetByCaller magnitudes) × ±15% variance
Step 2  Block     = Raw × (1 - BlockChance × 50%)             [shield, before mitigation]
Step 3  Mitigate  = per element: value × (1 - MitigationStat/(MitigationStat + K))
                        K_DEF = 100 × KZoneTierScale (physical), K_elem = 80 × KZoneTierScale (elemental)
Step 4  TypeChart = per element: × GetElementAffinityMultiplier(Source, TargetAffinity)
Step 5  Crit      = × (CritChance ≤ BaseCritChance + LUK/(LUK+200) [capped 70%] ? CritMult : 1)
                        CritMult = 1.5 + LUK/(LUK+400) × 2.5 (capped 4.0); Ranged + Bow adds +10% crit
Step 6  Buffs     = × (1 + OutgoingDamageMod) × (1 + IncomingDamageMod) × PrestigeMultiplier
Output  Health    = additive -FinalDamage
```

- **Invulnerability gate** — `TAG_State_Invulnerable` on the target short-circuits the whole pipeline (0 damage, no threat/noise/effect).
- **Type chart** — structural default cycle `Fire > Ice > Poison > Lightning > Fire`; same-element resists 0.5, opposites 1.5. `DT_ElementAffinity` is an optional DataTable override. Physical is always neutral (1.0). Target affinity comes from the target's `Element.*` tag (owned by `AOnsetEnemy::ApplyEnemyStats` from `DT_EnemyStats`).
- **DoTs/HoTs** reuse the same execution per tick, so every stage applies.

---

## **Threat Feed**

In `UOnsetAttributeSet::PostGameplayEffectExecute`, when damage lands on an `AOnsetEnemy`:

```
Threat = |magnitude| × AbilityThreatMultiplier × ClassThreatMultiplier
```

- Ability multiplier from the effect context's ability (`UOnsetGA_Generic` caches `DT_Abilities` row `ThreatMultiplier` at activation). DoT ticks inherit via the captured spec.
- Class multiplier from the instigator's `DT_ClassInfo` row (`Tank` default 1.5 in content).
- Also emits `ReportNoiseEvent` for group assist. See [Threat System](../AI/Threat_System.md).

---

## **Elements & CC**

- `EOnsetDamageElement` — Physical, Fire, Ice, Lightning, Poison. Each maps to a `Damage.*` tag and a `RES_*` attribute (mitigation stat).
- **DoTs** (Bleed/Poison/Burn) ride `UOnsetGenericDamageOverTimeEffect`; source stat is STR for Physical, INT for elemental.
- **Hard CC** — Stun (`State.Stunned`), Freeze (`State.Frozen`); durations pass through `UOnsetCCDiminishingComponent` (`100% → 50% → 25% → immune` within 6s window).
- **Soft CC** — Snare (movement speed), Slow (cooldown multiplier).

---

## **Data-Driven Abilities (`DT_Abilities`)**

- `FOnsetAbilityDefinition` rows hold identity, type, ranges, effects (`TArray<FOnsetAbilityEffect>`), optional montage, caster movement (Leap), cooldown tag/seconds, and `ThreatMultiplier`.
- `FOnsetAbilityEffect` — `Type` (Damage/Heal/Snare/Slow/Stun/Freeze/Invulnerable), `bFriendly` gate, `Magnitude`, `Duration`, `Period` (0 = instant, >0 = DoT/HoT), `ScalingType` (Weapon = STR, Skill = INT), `DamageTypeTag`.
- Authored with the ability creation editor tool (`UOnsetAbilityEditorWidget`); the editor can also emit a matching `DT_Scrolls` row. See [Inventory & Loot System](../Inventory/Inventory_System.md).

---

## **Source Location**
- All GAS files migrated from `Combat/` to `GAS/` directory: `OnsetAttributeSet.h/.cpp`, `OnsetMovementAttributeSet.h/.cpp`, `OnsetCombatAttributeSet.h/.cpp`, `OnsetGameplayTags.h/.cpp`, generic effect classes.
- `UOnsetDamageExecution`, `UOnsetAbilityLibrary`, `UOnsetEquipmentLibrary`, `UOnsetGA_Generic` live at `Combat/`.

## **MovementSpeed Attribute (E22)**
- `UOnsetMovementAttributeSet` owns `MovementSpeed` as a replicated attribute
- `PostAttributeChange` clamps ≥ 0 and writes to `CharacterMovement->MaxWalkSpeed`
- Base value initialises from CDO default (`InitMovementSpeed(600.0f)`), overridable per BP Class Defaults
- All StateTree tasks apply speed modifiers via `ApplyMovementSpeedModifier` helper (creates infinite GE with `MultiplyCompound` op, returns `FActiveGameplayEffectHandle`)
- Speed effects stack multiplicatively by default (flee × stagger × search = compound)
- Pool return clears all active GEs via `RemoveActiveEffects`, preventing speed leaks  

## **Cooldowns & Haste**
- Every ability (including the basic attack) is cooldown-gated via `GE_GenericCooldown` + a per-row `Cooldown.<RowName>` tag.
- `UOnsetGameplayAbility::ApplyCooldown` computes `BaseCooldown × (1 - TotalCDR%) × CooldownMultiplier`; `CooldownMultiplier` (>1) is the Slow debuff.
- Weapon archetype base-cooldown table (Dagger 0.8 … Greatsword 1.8) from `UOnsetEquipmentLibrary::GetArchetypeBaseCooldown`. Haste/mastery CDR from class + dual-wield + mastery bonuses. See [combat-formulas](../combat-formulas.md) §9.

---

## **Replication**
- Damage filtering occurs **server‑side only** via the [Multiplayer System](../Multiplayer/Multiplayer_System.md)  
- Clients receive replicated attribute changes  
- No client‑side prediction of [PvP System](../Gameplay/PVP_System.md) rules  

---

## **Testing Checklist**
- [ ] Abilities execute on input/AI trigger  
- [ ] Damage applies correctly  
- [ ] PvP filtering blocks player→player damage when OFF  
- [ ] AoE abilities respect PvP rules  
- [ ] Cooldowns work and replicate  
- [ ] Death triggers correctly (health ≤ 0)  
- [ ] Death fires both pool return and corpse spawn (two parallel paths)  
- [ ] MovementSpeed attribute initialises correctly at CDO default (600)  
- [ ] Flee/Investigate/Search tasks apply MovementSpeed GE correctly  
- [ ] Flee speed is dynamic — varies by health ratio each tick  
- [ ] MovementSpeed GEs stack multiplicatively (flee × stagger)  
- [ ] No speed leak on pool return — `RemoveActiveEffects` clears GEs  
- [ ] Damage pipeline stages run in order (variance → block → mitigation → type chart → crit → buffs)
- [ ] Type chart applies (Fire > Ice; same-element resists)
- [ ] Crit curves scale with LUK and cap at 70% / 4.0×
- [ ] Ranged + Bow grants the +10% crit mastery bonus
- [ ] CC diminishing returns (100 → 50 → 25 → immune) apply to Stun/Freeze durations
- [ ] Data-driven abilities resolve from `DT_Abilities` via `AbilityID.*` dynamic tag
- [ ] Enemy stats scale by `(1 + d)^Tier` from `DT_EnemyStats`
- [ ] Threat feed applies ability × class multipliers

---

## **Edge Cases**
- AoE overlaps players when PvP disabled  
- Player toggles PvP mid‑ability  
- Projectile fired before PvP toggle hits a player  
- Invulnerable target receives 0 damage and no threat/noise  
- CC-immune target (4th in-window application) — caller skips the effect
- DoT on a target that dies mid-window — effect removed on death/pool return  