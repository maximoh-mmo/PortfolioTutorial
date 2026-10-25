# Equipment Lifecycle System: Durability, Quality Grades, Salvage & Infusion

## Executive Overview
The **Equipment Lifecycle System** replaces traditional vendor-trash loot dumping with an evergreen, multi-layered progression and sink loop. 

Rather than selling surplus weapons and armor to NPC vendors for nominal cash, every dropped piece of equipment feeds into one of two critical game systems:
1. **Material Salvage (Dismantle):** Broken down into modular scrap and crafting components that fuel equipment maintenance and endgame upgrades.
2. **Duplicate Infusion (Combining):** Sacrificed into duplicate copies to steadily advance stat quality grades toward maximum **Grade S (Apex)** potential.

Equipment features a perpetual **Durability** degradation and repair model that creates an evergreen material sink, while safeguarding player investment by ensuring items are never permanently destroyed.

---

## 1. Quality Grades (Tier D through Tier S)

Every piece of equipment dropped from loot or spawned in the world carries mutable instance state represented by `FOnsetEquipmentInstanceData`. Its stats are determined by two separate quality grade rolls:
- **`PrimaryStatGrade`**: Governs weapon damage for weapons and flat defense bonus for armor/shields.
- **`SecondaryStatGrade`**: Governs flat attribute bonuses (`Strength`, `Intellect`, `Vitality`, `Agility`, `Luck`).

### Grade Scale & Multipliers

| Quality Grade | Designation | Stat Multiplier | Base Drop Weight | Visual Color |
| :--- | :--- | :--- | :--- | :--- |
| **Grade D** | Surplus / Defective | **$0.85\times$** | 25% | Muted Silver-Grey (`#8C8C8C`) |
| **Grade C** | Standard Issue | **$1.00\times$** | 50% | Pristine Off-White (`#E6E6E6`) |
| **Grade B** | Refined / Mil-Spec | **$1.15\times$** | 20% | Vibrant Mint Green (`#1FD173`) |
| **Grade A** | Overclocked | **$1.30\times$** | 4.5% | Radiant Violet (`#A640F2`) |
| **Grade S** | Apex / Masterwork | **$1.50\times$** | 0.5% | Brilliant Solar Gold (`#FFD126`) |

### Roll Mechanics & Player Luck
When equipment drops from enemies via `UOnsetLootLibrary::RollLoot`:
- Base roll distribution guarantees that **Grade C** is the baseline standard issue drop.
- The killer's `Luck` attribute (and the item's inherent rarity tier: Uncommon $+5$, Rare $+15$, Epic $+30$, Legendary $+50$) adds positive skew to the roll weights, expanding high-grade drop chances.
- Grade S remains an ultra-rare natural drop ($0.5\%$), designed primarily as an aspirational cap achieved through Infusion.

---

## 2. Durability & Malfunction State

### Degradation Rules
Equipment suffers wear and tear during active gameplay:
1. **Weapons**: Degrades $0.05$ durability per damage impact applied (approximately 1 point per 20 hits).
2. **Armor / Shields**: Degrades $0.10$ durability per damage impact absorbed (approximately 1 point per 10 hits received).
3. **Player Death**: Incurs an immediate, flat **$-15\%$** durability loss across all currently equipped weapons, shields, and armor pieces.

### Zero-Destruction "Malfunctioning" State
> [!IMPORTANT]
> Equipment is **never destroyed or deleted** upon reaching 0 durability. Players never lose their investment or hard-earned loot.

When an item hits $0.0$ durability (`CurrentDurability <= 0.0f`):
- It enters the **`Malfunctioning`** state.
- **Primary Combat Stats**: Weapon damage and shield/armor defense are reduced by a flat **$-50\%$**.
- **Secondary Stat Bonuses**: Stat bonuses (`Strength`, `Intellect`, `Vitality`, `Agility`, `Luck`) are halved ($50\%$ effectiveness).
- Shield block rate is halved.
- Repairing the item immediately restores $100\%$ stat function.

### Evergreen Repair Sink
Repair costs scale with missing durability and item rarity:
$$\text{MissingRatio} = 1.0 - \frac{\text{CurrentDurability}}{\text{MaxDurability}}$$
$$\text{CreditsCost} = \max\left(5, \lceil\text{MissingRatio} \times (\text{BuyValue} \times 0.25) \times \text{RarityMultiplier}\rceil\right)$$
$$\text{ScrapCost} = \max\left(1, \lceil\text{MissingRatio} \times 5.0 \times \text{RarityMultiplier}\rceil\right)$$

- Repairs consume **`CompositeScrap`**, guaranteed from salvaging low-tier gear, ensuring low-level loot remains consistently valuable throughout the entire game lifecycle.

---

## 3. Item Salvage (Dismantle)

Unwanted gear can be dismantled at any time via `UOnsetInventoryComponent::SalvageItem` (or `AOnsetBaseCharacter::SalvageItem`).

### Salvage Material Tiers
Materials belong to category `EOnsetItemCategory::Material` and stack up to 999:
1. **`CompositeScrap`**: Foundation scrap used for perpetual gear repairs.
2. **`ReinforcedAlloy`**: Refined alloys used for mid-tier repairs and crafting.
3. **`NaniteFilament`**: High-grade filaments for advanced gear.
4. **`QuantumCore`**: Top-tier quantum substrate cores.
5. **`WeaponChassis`**: Specialized component salvaged exclusively from weapons.
6. **`ArmorPlating`**: Specialized plate component salvaged from shields and armor.

### Salvage Yield Table
| Item Rarity | Primary Salvage Yield | Secondary / Bonus Yield |
| :--- | :--- | :--- |
| **Common** | $2-4\times$ `CompositeScrap` | $+1-2\times$ Chassis / Plating |
| **Uncommon** | $3-5\times$ `CompositeScrap` | $1-2\times$ `ReinforcedAlloy` $+ 1-2\times$ Chassis / Plating |
| **Rare** | $3-5\times$ `ReinforcedAlloy` | $1-2\times$ `NaniteFilament` $+ 1-2\times$ Chassis / Plating |
| **Epic** | $3-5\times$ `NaniteFilament` | $1-2\times$ `QuantumCore` $+ 1-2\times$ Chassis / Plating |
| **Legendary** | $5-8\times$ `NaniteFilament` | $2-4\times$ `QuantumCore` $+ 1-2\times$ Chassis / Plating |

*Grade A and S items yield additional bonus `ReinforcedAlloy` when dismantled.*

---

## 4. Duplicate Infusion (Combining)

Players can combine duplicate copies of the same equipment (matching `RowName`) using `UOnsetInventoryComponent::InfuseItem`:

```
[Target Item (Grade C)]  +  [Duplicate Catalyst (Destroyed)]  ==>  [Target Item Progress: +50%]
```

### Infusion Progression Steps
| Transition | Duplicates Required | Progress Gain Per Catalyst |
| :--- | :--- | :--- |
| **Grade D $\rightarrow$ Grade C** | 1 Duplicate | $+100.0\%$ |
| **Grade C $\rightarrow$ Grade B** | 2 Duplicates | $+50.0\%$ |
| **Grade B $\rightarrow$ Grade A** | 3 Duplicates | $+33.4\%$ |
| **Grade A $\rightarrow$ Grade S** | 5 Duplicates | $+20.0\%$ |

### Critical Resonance (Lucky Proc)
- Every infusion attempt has a flat **$15\%$ random chance** to trigger **Critical Resonance**.
- On proc, the target immediately promotes to the next grade regardless of current progress!
- **Promotion Perks**: Each grade promotion resets infusion progress to 0, increments `InfusionLevel`, and grants a **free $100\%$ durability restoration**.

---

## 5. Architectural & Data Model Integration

### Data Structures
- **[`OnsetItemTypes.h`](file:///e:/Unreal%20Projects/PortfolioTutorial/Project/Source/Onset/Public/Data/OnsetItemTypes.h)**:
  - `EOnsetItemCategory::Material`
  - `EOnsetQualityGrade` (`GradeD`, `GradeC`, `GradeB`, `GradeA`, `GradeS`)
  - `FOnsetEquipmentInstanceData`: tracks `CurrentDurability`, `MaxDurability`, `PrimaryStatGrade`, `SecondaryStatGrade`, `InfusionProgress`, `InfusionLevel`, `IsMalfunctioning()`.
  - `FOnsetInventoryEntry`: holds `FOnsetEquipmentInstanceData InstanceData;`
  - `EOnsetMaterialTier` & `FOnsetMaterialDefinition`
- **[`OnsetEquipmentTypes.h`](file:///e:/Unreal%20Projects/PortfolioTutorial/Project/Source/Onset/Public/Data/OnsetEquipmentTypes.h)**:
  - `FOnsetEquippedEntry`: holds `FOnsetEquipmentInstanceData InstanceData;`

### Core APIs
- **[`UOnsetEquipmentLibrary`](file:///e:/Unreal%20Projects/PortfolioTutorial/Project/Source/Onset/Public/Combat/OnsetEquipmentLibrary.h)**:
  - `GetGradeMultiplier()`, `GetNextGrade()`, `GetGradeDisplayText()`, `GetGradeColor()`
  - `RollRandomGrade()`, `RollNewEquipmentInstance()`
  - `CalculateEffectiveWeaponDamage()`, `CalculateEffectiveDefense()`, `CalculateEffectiveStatBonus()`
  - `CalculateRepairCost()`, `GetSalvageYield()`, `GetInfusionProgressPerDuplicate()`, `RollCriticalResonance()`
- **[`UOnsetInventoryComponent`](file:///e:/Unreal%20Projects/PortfolioTutorial/Project/Source/Onset/Public/Inventory/UOnsetInventoryComponent.h)**:
  - `DegradeEquippedWeapon(float Amount)`
  - `DegradeEquippedArmor(float Amount)`
  - `ApplyDurabilityLossOnDeath(float Percent = 15.0f)`
  - `RepairEquippedItem(EOnsetEquipmentSlot Slot)`
  - `RepairAllEquipped()`
  - `RepairBagItem(int32 BagSlotIndex)`
  - `SalvageItem(int32 BagSlotIndex, TArray<FOnsetInventoryEntry>& OutYield)`
  - `InfuseItem(int32 TargetSlotIndex, int32 CatalystSlotIndex, bool& bOutGradeUp, bool& bOutCriticalProc)`
- **[`AOnsetBaseCharacter`](file:///e:/Unreal%20Projects/PortfolioTutorial/Project/Source/Onset/Public/Core/OnsetBaseCharacter.h)**:
  - `RecalculateDerivedStats()`: factors in stat grade multipliers and 50% malfunctioning penalty.
  - `GetEquippedWeaponDamage()`: returns effective weapon damage scaled by grade and durability status.
  - `OnDeath()`: applies -15% durability penalty to equipped gear.
- **[`OnsetAttributeSet.cpp`](file:///e:/Unreal%20Projects/PortfolioTutorial/Project/Source/Onset/Private/GAS/OnsetAttributeSet.cpp)**:
  - Hits received degrade equipped armor by $0.10$.
  - Hits dealt degrade equipped weapon by $0.05$.

### JSON Persistence
- Full backwards compatibility with existing character build and save files.
- `SerializeInventoryJSON` and `SerializeEquipmentJSON` serialize instance data fields (`d`, `md`, `pg`, `sg`, `ip`, `il`), while seamlessly loading legacy string/flat entries.
