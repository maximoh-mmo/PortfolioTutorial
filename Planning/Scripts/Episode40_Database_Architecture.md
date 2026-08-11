# 🎬 **Episode 40 — Database Architecture & Player Data Subsystem**

## **Episode Goal**
Design the `IPlayerDataStore` abstraction, implement the SQLite store with schema migrations, and create the `UOnsetPlayerDataSubsystem` world subsystem on the dedicated server.

---

## **Context & Dependencies**
- Requires Episode 39 (Steam Integration — auth ticket flow working)
- Dedicated server build configured and tested (Episodes 37-38)
- Project compiles and runs in PIE

---

## **High‑Level Summary**
This episode establishes the persistence backbone. We introduce a clean interface (`IPlayerDataStore`) that the DS uses for all reads/writes, implement SQLite as the development backend, and wire it into a world subsystem so the rest of the codebase never talks to the database directly. We also set up a versioned migration system so schema changes are safe and repeatable.

---

## **Key Concepts Introduced**
- Interface-based storage abstraction (`IPlayerDataStore`)
- SQLite via the engine `SQLiteCore` module (no bundled amalgamation)
- World subsystem pattern for DS-only services
- Schema versioning and forward-only migrations
- WAL mode for SQLite concurrency safety

---

## **Technical Breakdown**

### **1. Create the `OnsetDataStore` Module**
- New module `OnsetDataStore` at `Source/OnsetDataStore/` (game module, added to the project's `Build.cs` module list)
- `OnsetDataStore.Build.cs` links engine `SQLiteCore` + `HTTP` (+ optional `libpq` for Postgres) — **no bundled `sqlite3.c`**; the amalgamation shipped with the engine's SQLiteCore is used instead

### **2. Create `IPlayerDataStore` Interface**
**File:** `Source/OnsetDataStore/Public/IPlayerDataStore.h`
```cpp
#pragma once

#include "CoreMinimal.h"
#include "OnsetPlayerDataTypes.h"

struct IPlayerDataStore
{
    virtual ~IPlayerDataStore() = default;

    virtual bool Initialize(const FString& ConnectionString) = 0;

    virtual bool LoadAccount(const FString& Platform, const FString& PlatformID, FOnsetAccountData& OutAccount) = 0;
    virtual bool CreateAccount(const FString& Platform, const FString& PlatformID) = 0;

    virtual bool LoadCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex, FOnsetFullCharacterData& OutData) = 0;
    virtual bool SaveCharacter(const FString& Platform, const FString& PlatformID, const FOnsetFullCharacterData& Data) = 0;
    virtual bool DeleteCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex) = 0;

    virtual void SaveAll() = 0;
};
```

### **3. Implement `FSQLiteStore`**
**File:** `Source/OnsetDataStore/Public/FSQLiteStore.h` + `Private/FSQLiteStore.cpp`

Key implementation details:
- Owns `sqlite3* DB` via the engine's `SQLiteCore` headers
- `Initialize()`: `sqlite3_open`, enable WAL mode (`PRAGMA journal_mode=WAL`), `PRAGMA synchronous=NORMAL`, `PRAGMA foreign_keys=ON`, run migrations. Default path: `FPaths::ProjectSavedDir() / "OnsetPlayerData.db"` (falls back to `:memory:` in PIE)
- Migrations: `EnsureSchema()` checks `_schema_version`, applies sequential migrations 1→3
- All queries use `sqlite3_prepare_v2` + `sqlite3_bind_*` (no string concat)
- Transactions for multi-row operations
- `SaveAll()`: `PRAGMA wal_checkpoint(PASSIVE)`

**Schema (migration 1):**
```sql
CREATE TABLE accounts (
    platform    TEXT NOT NULL,
    platform_id TEXT NOT NULL,
    created_at  TEXT NOT NULL DEFAULT (datetime('now')),
    last_login  TEXT NOT NULL DEFAULT (datetime('now')),
    PRIMARY KEY (platform, platform_id)
);
CREATE TABLE characters (
    platform            TEXT NOT NULL,
    platform_id         TEXT NOT NULL,
    slot_index          INTEGER NOT NULL CHECK(slot_index >= 0 AND slot_index <= 2),
    character_name      TEXT NOT NULL DEFAULT '',
    level               INTEGER NOT NULL DEFAULT 1,
    experience          INTEGER NOT NULL DEFAULT 0,
    saved_max_health    REAL NOT NULL DEFAULT 100.0,
    saved_position_x    REAL NOT NULL DEFAULT 0.0,
    saved_position_y    REAL NOT NULL DEFAULT 0.0,
    saved_position_z    REAL NOT NULL DEFAULT 0.0,
    saved_rotation_yaw  REAL NOT NULL DEFAULT 0.0,
    inventory_json      TEXT NOT NULL DEFAULT '{}',
    equipment_json      TEXT NOT NULL DEFAULT '{}',
    quests_json         TEXT NOT NULL DEFAULT '{}',
    created_at          TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at          TEXT NOT NULL DEFAULT (datetime('now')),
    PRIMARY KEY (platform, platform_id, slot_index),
    FOREIGN KEY (platform, platform_id) REFERENCES accounts(platform, platform_id) ON DELETE CASCADE
);
```
**Migration 2** adds `current_zone` column; **migration 3** adds `character_class` + `appearance_json` columns.

### **4. Create `FPgSQLStore` Stub + `DataStoreFactory`**
**File:** `Source/OnsetDataStore/Public/FPgSQLStore.h` + `.cpp`, `DataStoreFactory.h` + `.cpp`
- Same interface, uses `libpq` (`PQconnectdb`, `PQexecParams`)
- Returns `false` / empty data with log warning for demo
- `CreateDataStore(StoreType, ConnectionString, bInitialized)` factory picks SQLite / PgSQL / HTTP store

### **5. Create `UOnsetPlayerDataSubsystem`**
**File:** `Source/Onset/Public/Subsystem/OnsetPlayerDataSubsystem.h` + `.cpp`
```cpp
UCLASS()
class ONSET_API UOnsetPlayerDataSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override; // server-only

    bool LoadAccount(const FString& Platform, const FString& PlatformID, FOnsetAccountData& OutAccount);
    bool CreateAccount(const FString& Platform, const FString& PlatformID);
    bool LoadCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex, FOnsetFullCharacterData& OutData);
    bool SaveCharacter(const FString& Platform, const FString& PlatformID, const FOnsetFullCharacterData& Data);
    bool SaveCharacterPreservingIdentity(const FString& Platform, const FString& PlatformID, FOnsetFullCharacterData& Data);
    bool DeleteCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex);
    void SaveAll();

private:
    TUniquePtr<IPlayerDataStore> Store;
};
```
- `Initialize()`: reads `DefaultEngine.ini` `[Onset.DataStore]` `Type` / `ConnectionString`, honors `-OnsetDataStoreType=` / `-OnsetDataStoreURL=` command-line overrides, creates the store via the factory
- `Deinitialize()`: `StopAutoSaveTimer()`, `Store->SaveAll()`, reset
- `StartAutoSaveTimer()`: 5-min timer (configurable via `AutoSaveInterval`) → `SaveAll`

### **6. Config**
`Config/DefaultEngine.ini`:
```ini
[Onset.DataStore]
; Type=Postgres
; ConnectionString=host=localhost dbname=onset user=onset password=onset
; Type=SQLite (default)
; ConnectionString=
; Type=HttpApi
; ConnectionString=...
; APIKey=...
```

---

## **How to Test**
1. Compile the `OnsetDataStore` module (no amalgamation to add)
2. Launch DS: `Onset.exe ... -server -log`
3. Check log for:
   - `UOnsetPlayerDataSubsystem: initialized with SQLite (success=1)`
   - `FSQLiteStore: opened <path> (existing=0)` on first run
   - `FSQLiteStore: migration N applied` for each of the 3 migrations
4. Open `Project/Saved/OnsetPlayerData.db` in DB Browser for SQLite → verify tables exist
5. Stop DS → verify clean shutdown (auto-save timer stopped, `SaveAll` flushed)

---

## **Code Snippets**

```cpp
// IPlayerDataStore.h
virtual bool LoadAccount(const FString& Platform, const FString& PlatformID, FOnsetAccountData& OutAccount) = 0;
virtual bool SaveCharacter(const FString& Platform, const FString& PlatformID, const FOnsetFullCharacterData& Data) = 0;

// FSQLiteStore.cpp — EnsureSchema / RunMigration
void FSQLiteStore::EnsureSchema()
{
    // CREATE TABLE IF NOT EXISTS _schema_version ...
    int32 Version = GetSchemaVersion();
    const int32 LatestVersion = 3;
    while (Version < LatestVersion)
    {
        RunMigration(Version);   // each migration wrapped in a transaction
        Version = GetSchemaVersion();
    }
}

// UOnsetPlayerDataSubsystem.cpp
void UOnsetPlayerDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (!GetWorld()->IsServer()) return; // server-only via ShouldCreateSubsystem

    FString DataStoreType, ConnectionString;
    GConfig->GetString(TEXT("Onset.DataStore"), TEXT("Type"), DataStoreType, GEngineIni);
    GConfig->GetString(TEXT("Onset.DataStore"), TEXT("ConnectionString"), ConnectionString, GEngineIni);
    FParse::Value(FCommandLine::Get(), TEXT("OnsetDataStoreType="), DataStoreType);
    FParse::Value(FCommandLine::Get(), TEXT("OnsetDataStoreURL="), ConnectionString);

    bool bInitialized = false;
    Store = CreateDataStore(DataStoreType, ConnectionString, bInitialized);
}
```

---

## **Common Pitfalls**
- Forgetting to add `SQLiteCore` (and `HTTP`) to `OnsetDataStore.Build.cs` → unresolved symbols
- Not enabling WAL mode → DB locks under concurrent access
- String concatenation in SQL → injection bugs + broken migrations
- Forgetting `SaveAll()` in `Deinitialize()` → lost writes on DS shutdown
- Client connections must skip store init — gate with `ShouldCreateSubsystem` returning false on `NM_Client`

---

## **Dependencies**
- Requires Episode 39 (Steam auth ticket flow)
- Requires dedicated server build (Episodes 37-38)

---

## **Next Episode Preview**
Next time we extract the numeric SteamID from the auth ticket, store it on PlayerState, and implement the full save/load RPC flow — account auto-create on first login, character load on select, spawn from saved state.

---

## **Episode Checklist**
- [x] `OnsetDataStore` module created (SQLiteCore + HTTP linked, no amalgamation)
- [x] `IPlayerDataStore` interface defined (bool + out-param signature)
- [x] `FSQLiteStore` implements all methods with WAL + migrations
- [x] `FPgSQLStore` stub + `DataStoreFactory` compile
- [x] `UOnsetPlayerDataSubsystem` initializes store from config, server-only
- [x] Schema migrations 1→3 create accounts/characters tables
- [x] DS starts, opens DB, runs migrations, shuts down cleanly