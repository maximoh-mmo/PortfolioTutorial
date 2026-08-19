# 🎒 **ITEM & LOOT EDITOR**

**File:** `Docs/Tools/Item_Loot_Editor.md`

---

## **Purpose**

Provide a single in-editor surface for authoring the item category tables (`DT_Equipment`, `DT_QuestItems`, `DT_Junk`, `DT_Scrolls`) and the loot tables (`DT_Loot`), with cross-table reference validation and a loot-roll preview for balance tuning.

---

## **How to open**

Menu bar → **Onset Editor Tools** → **Item & Loot Editor** (opens a nomad tab; a level must be open).

---

## **Layout**

```
┌──────────────────────────────────────────────────────────────────────┐
│ [Equipment] [Quest Items] [Junk] [Scrolls] [Loot Tables]  ← mode tabs │
├──────────────────────────────────────────────────────────────────────┤
│ [Name        ] [Rarity] [Level]        (rows of the active table)     │
│  Iron Sword    Common    1                                            │
│ ───────────────────────────────────────────────────────────────────── │
│  UDetailsView bound to the selected row wrapper (mode-specific):      │
│   - common item fields (name, rarity, level req, stack, sell/buy,     │
│     icon, description)                                                │
│   - mode-specific fields (slot/archetype/stats for equipment,         │
│     GrantedAbility for scrolls, Entries/SubTables for loot)           │
│ ───────────────────────────────────────────────────────────────────── │
 │  Status line: validation results / roll-preview output                │
 │ [Add] [Delete] [Roll Preview (loot)]                                  │
└──────────────────────────────────────────────────────────────────────┘
```

- **Mode tabs** — each tab switches to one row struct + one DataTable: `FOnsetEquipmentDefinition`, `FOnsetQuestItemDefinition`, `FOnsetJunkItemDefinition`, `FOnsetScrollDefinition`, `FOnsetLootTableRow`.
- **Left** — row list (item modes: Name / Rarity / LevelRequirement; loot mode: Name / entry count / sub-table count).
- **Right** — `UDetailsView` bound to the mode's transient wrapper.
- **Status line** — persistence-blocked errors (a table with validation errors is skipped on close) and roll-preview output.

---

## **Buttons**

| Button | Behaviour |
|---|---|
| **Add** | Adds a default row named `New<Mode>` (deduped) and selects it. Persisted when the editor closes. |
| **Delete** | Removes the selected row. Persisted when the editor closes. |
| **Roll Preview (loot)** | Rolls the selected loot table 1000 times (`UOnsetLootLibrary::RollLoot`, no level/zone gating) and shows average drop quantity per item — a balance aid for drop rates. |

---

## **Saving — persisted on close (no Save button)**

There is **no Save button**. Edits are written back to the in-memory table immediately as you type (so switching modes / rows never loses a change), and disk persistence happens when the **editor window is closed**:

1. The pending edit of the currently selected row is committed to the table.
2. Every table you touched (add, edit, or delete in that mode) is **validated** — see below — then saved via `UPackageTools::SavePackagesForObjects`.
3. A table with validation errors is **not** saved; the error is shown in the status line, and the invalid edits are discarded (the on-disk table stays intact). Untouched tables are left alone.

## **Save-time validation**

Before persisting, `ValidateTable()` checks **every row** of each dirty table:

- **Scrolls** — `GrantedAbility` must resolve to a real row in `DT_Abilities`.
- **Loot tables** —
  - every `Entry.Item` handle must resolve in its category table;
  - every `SubTables` handle must point at `DT_Loot` (not another table) and be non-empty;
  - recursive cycle detection across sub-tables.

Any error blocks that table from being saved.

---

## **Data model recap**

- **Item base** — `FOnsetItemDefinition` (`Data/OnsetItemTypes.h`): `DisplayName`, `Rarity`, `LevelRequirement`, `MaxStackSize`, `SellValue`, `BuyValue`, `Icon`, `Description`.
- **Derived rows** — `FOnsetEquipmentDefinition` (slot, archetype, weapon/block/defense/stat bonuses), `FOnsetQuestItemDefinition` (`QuestHint`), `FOnsetJunkItemDefinition`, `FOnsetScrollDefinition` (`GrantedAbility` + `UpgradeLevel`).
- **Loot** — `FOnsetLootTableRow` (`Entries` + `SubTables`), `FOnsetLootEntry` (item, drop chance, qty range, zone/level gating), `FOnsetLootSubTableRef` (inclusion chance).

Tables load through the config seams in `[Onset.Gameplay]` (`EquipmentDataTable`, `QuestItemsDataTable`, `JunkDataTable`, `ScrollDataTable`, `LootDataTable`) via `UOnsetItemLibrary::GetTable` and `UOnsetLootLibrary::GetLootTable`.

---

## **Implementation notes**

- Editor-only code in `OnsetEditor` (`Public|Private/UI/OnsetItemEditorWidget.*`), mirroring the ability/enemy editor's list + `UDetailsView` + transient-wrapper + Add/Delete pattern.
- The details panel is a `UOnsetNotifyDetailsView`, which broadcasts `OnPropertyEdited` on every property change; the widget writes the wrapper back into the row immediately and records the mode as dirty (`DirtyModes`).
- Persistence happens in `NativeDestruct()` → `PersistOnClose()`: each dirty mode's table is validated (`ValidateTable`) and saved; a per-mode dirty set (`TSet<EOnsetItemEditorMode>`) avoids touching tables you never edited.
- One transient wrapper class per mode keeps the details panel type-correct; `UOnsetItemModeButton` (a `UButton` subclass) carries its mode for the tab row.
- The roll preview aggregates `FOnsetInventoryEntry` quantities (already stack-capped by `RollLoot`) and resolves item display names across the category tables for readable output.

---

## **Related**

- [Inventory & Loot System](../Inventory/Inventory_System.md) — item/loot data model, loot flow, config seams.
- [Enemy Creation Tool](Enemy_Editor.md) — sibling tool whose `LootTable` handle targets `DT_Loot` rows authored here.
- [Ability System](../GAS/GAS_System.md) / [Abilities](../Abilities/) — `DT_Abilities` rows referenced by scrolls.