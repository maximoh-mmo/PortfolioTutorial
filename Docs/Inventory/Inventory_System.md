# 📘 **INVENTORY & LOOT SYSTEM**

**File:** `Docs/Inventory/Inventory_System.md`

---

## **Purpose**

Provide data-driven items (equipment, quest items, junk, scrolls), a shared stacked-bag inventory component used by both player pawns and corpses, and reusable loot tables that roll item drops on NPC death. The player collects drops via **click-to-loot** on corpses, and the looted items auto-inventory immediately (no manual pick-up).

---

## **Responsibilities**

- Define a common item base (`FOnsetItemDefinition`) with per-category derived row structs
- Own per-category DataTables: `DT_Equipment`, `DT_QuestItems`, `DT_Junk`, `DT_Scrolls`
- Provide a shared `UOnsetInventoryComponent` for bag + equipped loadout + persistence
- Roll loot from reusable tables (`DT_Loot`) with level/zone gating, sub-table composition, and quantity ranges
- Populate corpse loot server-side and replicate it to all clients
- Click-to-loot: transfer loot on corpse interact, guard against double-loot, auto-path when out of range
- Show a loot-overlay popup with the just-looted items
- Apply equipped gear stats to the character (defense, stat bonuses, weapon base)

---

## **Non‑Responsibilities**

- The quest system (quest-item rows exist as data; quest wiring is a future pass)
- Scroll *granting/upgrade* — scrolls are dropped/looted now; the learn/upgrade pipeline is a future pass (the editor can already author scroll rows)
- Vendors / shops (sell/buy values are authored but no shop flow exists)
- Trade between players

---

## **Key Classes & Data**

### Item model — `Data/OnsetItemTypes.h`

- `EOnsetItemCategory` — `Equipment`, `QuestItem`, `Junk`, `Scroll`. **A row's table determines its category** (1:1 with a DataTable).
- `EOnsetItemRarity` — `Common` → `Legendary` (drives UI tinting).
- `FOnsetItemDefinition : FTableRowBase` — common base (`DisplayName`, `Rarity`, `LevelRequirement`, `MaxStackSize`, `SellValue`, `BuyValue`, `Icon`, `Description`).
- Derived rows per table:
  - `FOnsetQuestItemDefinition` — adds `QuestHint` (wiring arrives with the quest system).
  - `FOnsetJunkItemDefinition` — vendable filler, explicit type for future flavor fields.
  - `FOnsetScrollDefinition` — adds `GrantedAbility` (`FDataTableRowHandle` into `DT_Abilities`) and `UpgradeLevel` (1 = learn, >1 = upgrade target level).
- `FOnsetInventoryEntry` — one stacked bag slot: `Category` + `RowName` + `Count`. This is the canonical item reference used everywhere (bags, loot rolls, serialization).

### Equipment model — `Data/OnsetEquipmentTypes.h`

- `FOnsetEquipmentDefinition : FOnsetItemDefinition` — adds `Slot`, `Archetype` (`EOnsetWeaponArchetype`), `WeaponDamage`, `DamageElement`, `BlockChance`, `DefenseBonus`, and flat stat bonuses (Strength/Intellect/Vitality/Agility/Luck).
- `EOnsetEquipmentSlot` — Weapon, Shield, Head, Chest, Hands, Legs, Feet, Amulet, Ring1, Ring2, Trinket.
- `FOnsetEquippedEntry` — `Slot` + `RowName` (replicated as an array; TMap replication is unsupported).
- `FOnsetClassBaseStats` — fallback base stat block when the `DT_ClassInfo` row is missing.
- `FOnsetEnemyStats : FTableRowBase` — baseline enemy stats with a `LootTable` handle (`FDataTableRowHandle` into `DT_Loot`) rolled on death.

### Inventory component — `Inventory/UOnsetInventoryComponent.h`

Server-authoritative `UActorComponent` shared by player pawns and corpses.

**Bag:**
- `AddItem(Category, RowName, Count)` — merges into existing stacks up to `MaxStackSize`.
- `AddItems(TArray<FOnsetInventoryEntry>)`, `RemoveItem`, `RemoveAllOfItem`, `ClearItems`, `SetItems` (bulk stamp, used for corpse loot).
- Queries: `HasItem`, `GetItemCount`, `GetItems`, `GetTotalItemCount`.

**Equipped loadout:**
- `EquipItem(Slot, RowName)`, `UnequipSlot(Slot)`, `EquipFromInventory(RowName)` — resolves the slot from `DT_Equipment`, moves one bag instance into the slot, replaces whatever was there.
- `GetEquippedRow`, `GetEquippedItem`, `GetEquippedMap`.

**Persistence:**
- `SerializeEquipmentJSON` / `DeserializeEquipmentJSON`, `SerializeInventoryJSON` / `DeserializeInventoryJSON`. The bag serializes as an array of `{c, r, n}` objects (`Category`, `RowName`, `Count`) with legacy-format guards.

**Replication:**
- Bag `Items` and `EquippedEntries` replicate with `OnRep_` handlers.
- Owner-only by default (`bReplicateToOwnerOnly = true`) so the owning client reads its own bag/loadout.
- **Corpses** set `bReplicateToOwnerOnly = false` via `SetReplicateToOwnerOnly(false)` so every client can see the server-rolled loot.

`OnInventoryChanged` fires on any authority-side mutation (items or equipment).

### Loot model — `Data/OnsetLootTypes.h`

- `FOnsetLootContext` — `Level` (enemy difficulty tier) + `ZoneTag` (spawner-assigned area tag) for gating.
- `FOnsetLootEntry` — `Item` (`FDataTableRowHandle` into any category table), `DropChance`, `MinQty`/`MaxQty`, optional `RequiredZoneTag`, optional inclusive `MinLevel`/`MaxLevel`.
- `FOnsetLootSubTableRef` — references another `DT_Loot` row (`Table` + `InclusionChance`) pulled in recursively.
- `FOnsetLootTableRow : FTableRowBase` — one named loot table: `Entries` + `SubTables`.

### Loot library — `Combat/OnsetLootLibrary.h`

- `GetLootTable()` — loads/caches `DT_Loot` via config seam `[Onset.Gameplay] LootDataTable`.
- `RollLoot(TableRowHandle, Context)` / `RollLoot(TableRowName, Context)` — expands sub-tables recursively (cycle-safe), filters by level + zone, rolls each entry's `DropChance` and quantity range, and merges results into stacked entries capped by `MaxStackSize`. Returns `TArray<FOnsetInventoryEntry>`.

### Item library — `Data/OnsetItemLibrary.h/.cpp`

- Maps each `EOnsetItemCategory` to its config-seam table key (`EquipmentDataTable`, `QuestItemsDataTable`, `JunkDataTable`, `ScrollDataTable`) and resolves rows through the common base.

---

## **Config Seams** (`Project/Config/DefaultEngine.ini`, `[Onset.Gameplay]`)

| Key | Path |
|---|---|
| `EquipmentDataTable` | `/Game/Data/DT_Equipment` |
| `QuestItemsDataTable` | `/Game/Data/DT_QuestItems` |
| `JunkDataTable` | `/Game/Data/DT_Junk` |
| `ScrollDataTable` | `/Game/Data/DT_Scrolls` |
| `LootDataTable` | `/Game/Data/DT_Loot` |
| `ClassDataTable` | `/Game/Data/ClassInfo` |
| `EnemyStatsDataTable` | `/Game/Data/DT_EnemyStats` |
| `AbilityDataTable` | `/Game/Game/Combat/DT_Abilities` |

---

## **Data Flow — NPC Death → Loot Drop**

```
AOnsetEnemy::OnDeath (PostGameplayEffectExecute, Health <= 0)
    │
    ├──> Corpse spawned (AOnsetCorpse) via UOnsetCorpseSubsystem
    │       └──> Corpse->InventoryComponent->SetItems(RollLoot(Stats->LootTable, Context))
    │             Context = { Level: difficulty tier, ZoneTag: spawner area }
    │
    ├──> Empty loot → Corpse->SetLifeSpan(4.0f)  (auto-expire; still despawns on click)
    └──> Corpse loot replicated to all clients (owner-only disabled on corpse component)
```

## **Data Flow — Click-to-Loot**

```
Player clicks corpse (IA_Primary raycast hits AOnsetCorpse)
    │
    └──> UInteractionComponent::TryLootCorpse(Corpse)
            ├── bLooted? → abort
            ├── Within LootRange (250u)?
            │       ├── Yes → LootCorpse
            │       └── No  → set PendingLootCorpse + PendingMoveTarget, path to corpse,
            │                 poll every 0.2s (LootArrivalTimerHandle) until in range
            │
            └──> LootCorpse
                  ├── Guard: valid corpse + not bLooted + both inventory components
                  ├── PawnInventory->AddItems(CorpseInventory->GetItems())
                  ├── OwnerController->Client_ShowLootOverlay(Loot)   [client RPC]
                  ├── Corpse->bLooted = true   (replicated; double-loot guard)
                  └── Corpse->Destroy(); ClearPendingLoot()
```

**Click-to-loot rules:**
- No range check on the *click* — clicking any corpse triggers loot or auto-path.
- Out-of-range clicks auto-path via the existing `PendingMoveTarget` flow and loot on arrival.
- Empty corpses expire after 4s (`SetLifeSpan`) and also despawn on click.
- `bLooted` is replicated and server-authoritative, so loot cannot be double-claimed.

---

## **Data Flow — Equipment Stats**

Equipped gear flows into derived stats via `AOnsetBaseCharacter::RecalculateDerivedStats`:

- **Armor slots (Head/Chest/Hands/Legs/Feet) + Shield** — `DefenseBonus` sums into flat DEF.
- **Weapon** — `WeaponDamage` is the `WeaponBase` for weapon-scaled abilities (`Raw = WeaponDamage × (1 + STR/100)`); `DamageElement` is the basic attack's element.
- **Shield** — `BlockChance` feeds the pre-mitigation block stage.
- **Any slot** — flat stat bonuses (STR/INT/VIT/AGI/LUK) are summed into the character's stats.
- Missing `DT_ClassInfo` rows fall back to `FOnsetClassBaseStats`.

---

## **Ability Editor Integration** (scroll rows + threat multiplier)

The ability creation editor (`UOnsetAbilityEditorWidget`, editor-only) now authors items alongside abilities:

- **`bCreateScroll`** — when creating an ability, also writes a `FOnsetScrollDefinition` row to `DT_Scrolls` whose `GrantedAbility` points at the newly created ability row. Both tables persist.
- **Delete cleanup** — deleting an ability scans `DT_Scrolls` for rows whose `GrantedAbility` references the deleted ability (matched by row handle, not name) and removes them, preventing dangling references.
- **`ThreatMultiplier`** — the creation dialog exposes a per-ability threat multiplier (see [Threat System](../AI/Threat_System.md)).

## **Editor Tools**

Itemisation and enemy data are authored through two editor-only tools:

- **[Item & Loot Editor](../Tools/Item_Loot_Editor.md)** — edits `DT_Equipment` / `DT_QuestItems` / `DT_Junk` / `DT_Scrolls` / `DT_Loot` with cross-table reference validation (granted abilities, item rows, sub-tables, cycle detection) and a 1000-roll drop preview for balance.
- **[Enemy Creation Tool](../Tools/Enemy_Editor.md)** — one `DT_EnemyStats` row = one complete enemy type; the row owns its visual/AI/perception profiles and `LootTable` handle. Spawners fall back to the row's profiles when `FSpawnConfig` doesn't override them.

---

## **Interactions With Other Systems**

- **[Corpse System](../AI/Corpse_System.md)** — corpse hosts the inventory component; `OnDeath` rolls loot; click-to-loot consumes it.
- **[Player System](../Player/Player_System.md)** — `UInteractionComponent` branches to corpse loot in `ProcessPrimaryInteraction`; `AOnsetPlayerController` fires `Client_ShowLootOverlay`.
- **[UI System](../Gameplay/UI_System.md)** — `ULootOverlayWidget` popup lists just-looted items (rarity-tinted, auto-hides after 4s).
- **[GAS System](../GAS/GAS_System.md)** — weapon base from equipped gear drives weapon-scaled damage; equipped defense feeds mitigation.
- **[Threat System](../AI/Threat_System.md)** — per-ability `ThreatMultiplier` rides the ability's effect context.
- **[Account System](../Player/Account_System.md)** / **[Persistence Data Store](../Server/Persistence_Data_Store.md)** — `inventory_json` / `equipment_json` blobs (bag serialized as `{c, r, n}` array).
- **[Spawner System](../AI/Spawner_System.md)** — provides the `ZoneTag` used by `FOnsetLootContext` for zone-gated loot.

---

## **Replication Rules**

- Inventory is **server-authoritative**. All mutations happen server-side; clients see replicated arrays.
- Player pawns replicate the bag/loadout **owner-only**.
- Corpse loot replicates to **all clients** so every player sees the same dropped items.
- `bLooted` replicates (server-authoritative double-loot guard).
- Loot overlay is **client-only** — `Client_ShowLootOverlay` is a Client RPC.

---

## **Edge Cases**

- **Loot table row missing / table not loaded** — `RollLoot` returns empty; corpse gets no items and uses the 4s expire path.
- **Empty loot on death** — corpse expires after 4s; still despawnable by click.
- **Double-click / two players on one corpse** — `bLooted` guard; first looter wins, corpse destroyed.
- **Player out of range** — auto-path via `PendingMoveTarget`, 0.2s arrival poll.
- **Player dead / no pawn while polling** — `TryLootCorpse` guards on pawn validity; `ClearPendingLoot` on invalid corpse.
- **Sub-table cycles** — `RollLoot` expansion is cycle-safe.
- **Level/zone gated entries** — filtered by `FOnsetLootContext` before `DropChance` is rolled.

---

## **Testing Checklist**

- [ ] `AddItem` stacks within `MaxStackSize` and creates new stacks past it
- [ ] `EquipFromInventory` resolves slot from `DT_Equipment`, moves one instance, replaces slot contents
- [ ] `UnequipSlot` returns the item to the bag
- [ ] Bag + equipment serialize to `{c, r, n}` JSON and deserialize back losslessly (legacy format still parses)
- [ ] NPC death rolls loot into corpse inventory; loot replicates to all clients
- [ ] Empty-loot corpse expires after 4s
- [ ] Click-to-loot in range transfers loot, fires overlay, marks `bLooted`, destroys corpse
- [ ] Click-to-loot out of range auto-paths and loots on arrival
- [ ] Double-click / second player cannot double-loot a corpse
- [ ] Loot overlay shows items with rarity tinting and auto-hides after 4s
- [ ] Equipped armor feeds DEF into derived stats; weapon base feeds weapon-scaled damage
- [ ] Ability editor `bCreateScroll` writes a matching `DT_Scrolls` row; deleting the ability removes the scroll row

---

## **Future Extensions**

- Scroll **grant/upgrade pipeline** (learn/upgrade an ability from a looted scroll)
- Vendors / shops (sell/buy authored values)
- Quest wiring for `DT_QuestItems` rows
- Loot auto-pickup setting (walk-over vs click)
- PvP-influenced corpse loot (see `Docs/Future_Ideas.md`)
