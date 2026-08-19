# ⚔️ **ENEMY CREATION TOOL**

**File:** `Docs/Tools/Enemy_Editor.md`

---

## **Purpose**

Provide a single in-editor surface for authoring a complete enemy type. An enemy type is now a **`DT_EnemyStats` row that owns its three profile assets** (visual, AI, perception) plus a loot table — so one row is the whole definition, and the editor builds/edits/saves it without touching five scattered assets by hand.

---

## **How to open**

Menu bar → **Onset Editor Tools** → **Enemy Editor** (opens a nomad tab; a level must be open).

---

## **Layout**

```
┌──────────────────────────────────────────────────────────────────────┐
│ [Name      ] [Level] [XP] [Tier]          (list of DT_EnemyStats rows)│
│  Goblin        3     12    1                                          │
│  OrcBrute      8     40    1                                          │
│ ───────────────────────────────────────────────────────────────────── │
│  UDetailsView bound to the selected row wrapper:                       │
│   - DisplayName / Level / XpReward / MaxHealth / DamageBase / ...     │
│   - LootTable picker                                                   │
│   - VisualProfile / AIProfile / PerceptionProfile asset pickers       │
│ ───────────────────────────────────────────────────────────────────── │
 │ XP at level 8: required 250 | base per kill 40 | on-level grant 40 ...│
 │ [Add] [Delete] [Test in PIE]                                           │
└──────────────────────────────────────────────────────────────────────┘
```

- **Left** — row list from `DT_EnemyStats` (`UOnsetEquipmentLibrary::GetEnemyStatsTable()`), showing Name / Level / XP.
- **Right** — a `UDetailsView` bound to the selected row wrapped in a transient `UOnsetEnemyEditRowWrapper`. The panel auto-generates every field: stats, XP, loot-table picker, and the three profile asset pickers.
- **XP preview line** — live derivation: `XPRequired(Level)`, base XP per kill (`GetEnemyBaseXP`), the on-level grant (`GetGrantedXP`), and kills-per-level, so pacing is visible while editing.
- **Tier column** — informational; the real difficulty tier lives on the spawner (`FSpawnConfig.Tier`), not the row.

---

## **Buttons**

| Button | Behaviour |
|---|---|
| **Add** | Opens the creation dialog (name + `bAutoCreateProfiles`). Creates the row and, when enabled, sibling `VP_<Name>` / `AI_<Name>` / `PP_<Name>` DataAssets under `/Game/AI/`, then links them into the row. Persisted when the editor closes. |
| **Delete** | Removes the selected row. Persisted when the editor closes. |
| **Test in PIE** | Spawns the selected enemy ~300u in front of the local player's camera in the running PIE world, applying the row's profiles + stats (tier 0) and possessing it with a fresh `AOnsetAIController`. |

---

## **Saving — persisted on close (no Save button)**

There is **no Save button**. Edits are written back to the in-memory table immediately as you type (so switching rows / closing never loses a change), and disk persistence happens when the **editor window is closed**:

1. The pending edit of the currently selected row is committed to the table.
2. The table **and** every profile asset referenced by any row (visual / AI / perception) are saved via `UPackageTools::SavePackagesForObjects`.
3. Only rows you actually changed (added, edited, or deleted) trigger a save; untouched tables are left alone.

---

## **Data model — one row = one enemy type**

`FOnsetEnemyStats` (`Onset/Public/Data/OnsetEquipmentTypes.h`) now carries:

- **Stats / identity** — `DisplayName`, `Level` (1–200), `XpReward` (0 = derive), `MaxHealth`, `DamageBase`, `Defense`, `ResistanceFire/Ice/Lightning/Poison`, `Luck`, `WeaponArchetype`, `ElementAffinity`.
- **Loot** — `LootTable` (`FDataTableRowHandle` into `DT_Loot`).
- **Profiles (new)** — `VisualProfile` / `AIProfile` / `PerceptionProfile` (`TObjectPtr` refs, nullable).

### Spawner fallback (`Onset/Public/Spawning/OnsetSpawner.h/.cpp`)

`AOnsetSpawner` gained `ResolveVisualProfile()` / `ResolveAIProfile()` / `ResolvePerceptionProfile()`:

```
Resolve<Profile>() = Config.Enemy<Profile>   // explicit spawner override wins
                     ?: StatsRow.<Profile>    // row-owned profile (the tool's model)
```

All spawn-time and editor-preview code uses the resolved profiles (`SpawnEnemyAtSlot`, `CreateSpawnPoint`, `UpdateAllSpawnPointPreviews`, and the `SpawnGroup` guard), so:

- Existing spawners with explicit `FSpawnConfig` profiles keep working unchanged.
- New spawners can just assign `Config.EnemyStats` and the row drives visuals/AI/perception.

---

## **Implementation notes**

- Editor-only code lives in `OnsetEditor` (`Public|Private/UI/OnsetEnemyEditorWidget.*`, `Private/UI/OnsetEnemyCreationDialog.*`), mirroring the ability editor's list + `UDetailsView` + transient-wrapper + Add/Delete pattern.
- The details panel is a `UOnsetNotifyDetailsView`, which broadcasts `OnPropertyEdited` on every property change; the widget listens and writes the wrapper back into the row immediately (see `HandlePropertyEdited`).
- Persistence happens in `NativeDestruct()` → `PersistOnClose()`: the widget marks the table dirty on Add/Delete/edit and saves the table + linked profiles only if something changed (`bDirty`). A dirty flag avoids touching disk on rows you never edited.
- The creation dialog is a Slate modal (`SEnemyCreationDialog`) bound to a transient `UOnsetEnemyCreationData`; profile auto-creation uses `CreatePackage` + `NewObject<…>(RF_Public | RF_Standalone)` under `/Game/AI/`.
- `Test in PIE` uses `GEditor->GetPIEWorldContext()` and applies the row's stats with tier 0 — for tiered testing, point a spawner at the row instead.

---

## **Related**

- [Spawner System](../AI/Spawner_System.md) — `FSpawnConfig`, resolved-profile fallback, difficulty tiers.
- [Leveling System](../Player/Leveling_System.md) — `Level` / `XpReward` and the XP curves shown in the preview line.
- [Inventory & Loot System](../Inventory/Inventory_System.md) — the `LootTable` handle the tool wires to `DT_Loot`.
- [Item & Loot Editor](Item_Loot_Editor.md) — sibling tool for authoring `DT_Loot` rows the enemy loot table points at.