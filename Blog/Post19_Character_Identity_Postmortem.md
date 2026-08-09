# 🎬 Post 19 — The Bug That Ate the Character Names: A Postmortem

*1 commit · Aug 9, 2026 · covering INSERT OR REPLACE Semantics, Partial Saves, SaveCharacterPreservingIdentity*

---

## Introduction

Every persistence system has a moment where the player's data vanishes and you have to figure out how. Mine happened between login sessions: I created a character, played, logged out, logged back in — and the character select screen showed an *occupied* slot with a blank name and a reset level. The character still existed. It had just lost its identity. Its name, class, and level were gone, silently, replaced by defaults.

This post is a postmortem of that bug. It's not a glamorous feature post — it's a deep dive into `INSERT OR REPLACE` semantics, a store abstraction that let a whole class of data-loss bugs hide in plain sight, and the small fix that makes the save path honest about what it knows and what it doesn't.

---

## Technical Exposition

### How the save system was supposed to work

The persistence layer (Post 12, A5b) sits behind an `IPlayerDataStore` interface with three implementations: SQLite, PostgreSQL, and an HTTP stub. The write path is `SaveCharacter(Platform, PlatformID, CharData)` — one call, one struct, "persist this character." The schema is a single `characters` row per platform+slot:

| Column | Purpose |
|---|---|
| `character_name`, `level`, `experience` | identity + progression |
| `character_class`, `appearance_json` | class & visuals |
| `saved_max_health`, `saved_position_x/y/z`, `saved_rotation_yaw`, `current_zone` | runtime state |
| `inventory_json`, `equipment_json`, `quests_json` | player data blobs |

When you created a character, `Server_CreateCharacter` built a full `FOnsetFullCharacterData` with the name, class, level 1, and appearance, and saved it. The row was complete.

### The trap: `INSERT OR REPLACE`

The SQLite store upserts with `INSERT OR REPLACE`:

```sql
INSERT OR REPLACE INTO characters
  (platform, platform_id, slot_index, character_name, level, experience,
   saved_max_health, saved_position_x, saved_position_y, saved_position_z, saved_rotation_yaw,
   inventory_json, equipment_json, quests_json, current_zone, character_class, appearance_json, updated_at)
  VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, datetime('now'));
```

Here's the semantic trap: **`INSERT OR REPLACE` deletes the existing row and inserts a new one with whatever columns you supplied.** Any column you *don't* supply falls back to its column default — `character_name` → `''`, `level` → `1`, `experience` → `0`. It is not a partial update; it is a full-row replacement wearing a friendly name.

The PostgreSQL store had the same disease in a different costume — its upsert set every column from the provided value:

```sql
INSERT INTO characters (platform, platform_id, slot_index, character_name, ...)
VALUES ($1, $2, $3, $4, ...)
ON CONFLICT (platform, platform_id, slot_index) DO UPDATE SET
  character_name = EXCLUDED.character_name,
  level = EXCLUDED.level,
  ...
```

Same effect: whatever the caller passes in wins, and an empty name in the struct means an empty name in the database.

### Where the empty structs came from

`SaveCharacter` was called from five places, and most of them were **partial saves** — runtime state only. They built a fresh `FOnsetFullCharacterData`, filled in position/rotation/zone/health, and left everything else at its default:

```cpp
FOnsetFullCharacterData CharData;
CharData.SlotIndex = PS->SelectedCharacterSlot;
CharData.SavedPosition = PlayerChar->GetActorLocation();
CharData.SavedRotationYaw = PlayerChar->GetActorRotation().Yaw;
CharData.CurrentZone = GetWorld()->GetMapName();
CharData.SavedMaxHealth = ...;
CharData.InventoryJSON = TEXT("{}");
CharData.EquipmentJSON = TEXT("{}");
CharData.QuestsJSON = TEXT("{}");
```

Every one of these, running after character creation, wiped the name. The offender list:

- `SaveCurrentCharacter` — called from `EndPlay` and `PawnLeavingGame` (disconnect)
- `HandleLogout` in the auth subsystem
- `TravelToZone` in the game mode (zone travel!)
- `OnAbandonedTimeout` in the player AI controller (autoplay despawn)
- `Server_SaveCharacter` — which additionally overwrote the name with `PS->GetPlayerName()` and forced `Level = 1`

So the bug wasn't one broken save. It was a **systemic mismatch**: the store API pretended a partial save was a full save, and the store implementations happily destroyed the columns the callers never bothered to read.

---

## Implementation Details

### 1. The fix: make the merge explicit

The fix lives in `UOnsetPlayerDataSubsystem` — the layer that sits between the callers and the store. I added `SaveCharacterPreservingIdentity`, which reads the existing row *first* and carries the identity fields forward before saving:

```cpp
bool UOnsetPlayerDataSubsystem::SaveCharacterPreservingIdentity(
    const FString& Platform, const FString& PlatformID, FOnsetFullCharacterData& Data)
{
    FOnsetFullCharacterData Existing;
    if (LoadCharacter(Platform, PlatformID, Data.SlotIndex, Existing))
    {
        Data.CharacterName = Existing.CharacterName;
        Data.Level = Existing.Level;
        Data.Experience = Existing.Experience;
        Data.CharacterClass = Existing.CharacterClass;
        Data.AppearanceJSON = Existing.AppearanceJSON;
    }
    return SaveCharacter(Platform, PlatformID, Data);
}
```

The name matters. `SaveCharacterPreservingIdentity` is not `SaveCharacter` with extra steps — it's a **different contract**: "save the runtime state I give you, but never clobber the identity I'm not in charge of." The identity columns are loaded from the store, merged into the data, and only then written.

The `LoadCharacter` call has a graceful fallback: if there's no existing row yet (first save of a brand-new character), the identity fields stay whatever the caller set — which is correct, because the caller is `Server_CreateCharacter` and it set them deliberately.

### 2. Converting the call sites

All five partial-save sites changed from `SaveCharacter(...)` to `SaveCharacterPreservingIdentity(...)`. The argument changed from `const` to non-const because the method mutates the struct — a good sign it's doing real work rather than a wrapper.

`Server_SaveCharacter` got an extra fix. It was the only site that *tried* to write identity, but it did it wrong — it took the name from `PS->GetPlayerName()` (which is frequently empty or a generic login name, not the character name) and force-set `Level = 1` every save. With the new helper, those lines are deleted entirely: the character's real name and level come from the store, and the manual save just reports position/health. The manual save can no longer contradict the character's identity, because it no longer pretends to know it.

### 3. Why this fix beats changing the store

The tempting fix is to change the store layer — make SQLite do a partial `UPDATE` that only touches non-default columns, or make the interface take a mask of "which fields are valid." I rejected that for three reasons:

1. **It touches three backends.** SQLite, Postgres, and the HTTP stub would each need the same partial-update semantics, tripling the surface area for bugs and drift.
2. **"Non-default" is ambiguous.** `Level = 1` is a valid value that happens to equal the default. A partial-update heuristic can't distinguish "caller didn't set it" from "caller genuinely saved level 1."
3. **The real bug was at the callers.** They had data they could have loaded but chose not to. Fixing the layer above the store, where the load already exists, is one function instead of three backends.

The merge-in-the-subsystem approach works identically for every store implementation, because by the time the data reaches the store it's a complete row.

---

## Results & Validation

- **Create → logout → login:** slot shows the entered name, level, and class
- **Zone travel no longer wipes identity** (`TravelToZone` was the sneakiest offender — a normal gameplay action, not just logout)
- **Manual save preserves stored name/level** instead of overwriting from PlayerState
- **Autoplay despawn** (`OnAbandonedTimeout`) saves final state without touching identity
- **New characters still work:** first save with no existing row keeps the caller's full data
- Editor build compiles clean across both the Onset and OnsetDataStore modules

The reproduced case — the one that started this investigation — now passes: create a character with a name, disconnect, log back in, and the name is still there.

---

## Challenges & Solutions

### The store interface lied about its semantics

**Problem:** `SaveCharacter(const Data&)` reads like "save this character," so every caller assumed it was safe to pass a partial struct. The implementations silently replaced the whole row.

**Solution:** The interface contract now has two explicit flavors — full save (`SaveCharacter`) and state-preserving save (`SaveCharacterPreservingIdentity`). Callers choose based on what they actually know. A method name that documents its guarantees is worth a hundred comments.

### The bug was invisible without a full login cycle

**Problem:** Everything *looked* fine in a single session. The name was set at creation, the save at logout "succeeded" (the store returned true), and nothing broke until the next `LoadAccount`. Bugs that only appear across process boundaries are the hardest to catch because no single frame is obviously wrong.

**Solution:** The test had to be a real two-session round trip: create → disconnect → reconnect → read. That's now the canonical regression check for this fix, and it's the kind of test that should be automated for the persistence layer — a note I'm carrying forward.

### `Server_SaveCharacter` was making it worse

**Problem:** The one save site that *did* write identity was writing the wrong values — the PlayerState login name instead of the character name, and a hardcoded level.

**Solution:** Deleting those lines, not fixing them. The manual save doesn't own the character's identity; the store does. Removing the wrong code was strictly better than trying to correct it.

---

## Reflection & Lessons Learned

This bug was embarrassing in the best possible way. It exposed a design flaw I'd have sworn I'd never make: **an API whose name overpromised and whose implementations quietly destroyed data.** The whole class of bug was enabled by one decision back in Post 12 — a single `SaveCharacter` entry point — and it stayed hidden for exactly as long as no one logged out and back in.

The fix reframes how I think about save APIs. A save function should either:
1. receive a **complete** representation of what it writes, or
2. explicitly declare what it's *not* responsible for.

`SaveCharacterPreservingIdentity` is option 2 made visible in the name. The same principle generalizes beyond databases — any "set" operation that takes a partial object should say what it preserves.

The second lesson is about load-before-save. The fix is trivially simple — read existing, merge, write — and yet it was the right call over a database-level partial update. When a bug lives at the boundary between two layers, fixing it at the layer that already has both kinds of access (load *and* save) is almost always less risky than threading new semantics through the whole stack.

---

## Forward-Looking Content

| Phase | Status |
|---|---|
| A4.6 — Phase 1 Abilities | ✅ Committed (Post 16) |
| A6.1 — HUD Foundation + v2 | ✅ Committed (Posts 17–18) |
| **Persistence — identity-safe saves** | ✅ **Committed** |

With identity-preserving saves in place, the persistence layer finally behaves the way the UI always assumed it did. The natural next phase is the one the whole project has been circling: a proper demo loop — waves, respawn, and a full combat flow stitched together from every system built so far, plus the data-driven ability editor outlined in `Docs/EditorToolPlan.md`.

---

*Arc commit: `627e564`. Related docs: `Docs/Server/Persistence_Data_Store.md`, `Docs/Player/Account_System.md`.*
