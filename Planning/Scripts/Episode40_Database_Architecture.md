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
- SQLite amalgamation integration in UE5
- World subsystem pattern for DS-only services
- Schema versioning and forward-only migrations
- WAL mode for SQLite concurrency safety

---

## **Technical Breakdown**

### **1. Add SQLite Amalgamation**
- Download `sqlite3.h` + `sqlite3.c` from sqlite.org/download.html (amalgamation)
- Place in `Source/Onset/ThirdParty/SQLite/`
- Update `Onset.Build.cs`:
  ```csharp
  PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "ThirdParty", "SQLite"));
  PublicAdditionalLibraries.Add(Path.Combine(ModuleDirectory, "ThirdParty", "SQLite", "sqlite3.c"));
  ```

### **2. Create `IPlayerDataStore` Interface**
**File:** `Source/Onset/Public/Server/IPlayerDataStore.h`
```cpp
#pragma once

#include "CoreMinimal.h"
#include "OnsetAccountData.h" // FOnsetAccountData, FOnsetFullCharacterData

class IPlayerDataStore
{
public:
    virtual ~IPlayerDataStore() = default;

    // Initialize with connection string (SQLite path or PgSQL conn string)
    virtual bool Initialize(const FString& ConnectionString) = 0;
    virtual void Shutdown() = 0;

    // Account operations
    virtual FOnsetAccountData LoadAccount(const FString& Platform, const FString& PlatformID) = 0;
    virtual bool CreateAccount(const FString& Platform, const FString& PlatformID) = 0;

    // Character operations (slot 0-2)
    virtual FOnsetFullCharacterData LoadCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex) = 0;
    virtual bool SaveCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex, const FOnsetFullCharacterData& Data) = 0;
    virtual bool CreateCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex, const FString& CharacterName) = 0;

    // Flush all pending writes (called on DS shutdown)
    virtual void SaveAll() = 0;
};
```

### **3. Implement `FSQLiteStore`**
**File:** `Source/Onset/Public/Server/SQLiteStore.h` + `Private/Server/SQLiteStore.cpp`

Key implementation details:
- Owns `sqlite3* DBHandle`
- `Initialize()`: `sqlite3_open`, enable WAL mode (`PRAGMA journal_mode=WAL`), run migrations
- Migrations: check `_schema_version` table, apply sequential SQL blocks
- All queries use `sqlite3_prepare_v2` + `sqlite3_bind_*` (no string concat)
- Transactions for multi-row operations
- `SaveAll()`: `sqlite3_wal_checkpoint(DBHandle, SQLITE_CHECKPOINT_FULL)`

**Schema (migration v1):**
```sql
CREATE TABLE _schema_version (version INTEGER PRIMARY KEY);
CREATE TABLE accounts (
    platform_id TEXT NOT NULL,
    platform    TEXT NOT NULL,
    created_at  TEXT NOT NULL DEFAULT (datetime('now')),
    last_login  TEXT,
    PRIMARY KEY (platform, platform_id)
);
CREATE TABLE characters (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    platform_id     TEXT NOT NULL,
    platform        TEXT NOT NULL,
    slot_index      INTEGER NOT NULL CHECK(slot_index BETWEEN 0 AND 2),
    name            TEXT NOT NULL DEFAULT 'Adventurer',
    level           INTEGER NOT NULL DEFAULT 1,
    experience      REAL NOT NULL DEFAULT 0,
    max_health      REAL NOT NULL DEFAULT 100.0,
    pos_x           REAL NOT NULL DEFAULT 0,
    pos_y           REAL NOT NULL DEFAULT 0,
    pos_z           REAL NOT NULL DEFAULT 200,
    rot_yaw         REAL NOT NULL DEFAULT 0,
    inventory_json  TEXT NOT NULL DEFAULT '[]',
    equipment_json  TEXT NOT NULL DEFAULT '{}',
    quests_json     TEXT NOT NULL DEFAULT '{}',
    play_time_sec   INTEGER NOT NULL DEFAULT 0,
    created_at      TEXT NOT NULL DEFAULT (datetime('now')),
    saved_at        TEXT NOT NULL DEFAULT (datetime('now')),
    FOREIGN KEY (platform, platform_id) REFERENCES accounts(platform, platform_id),
    UNIQUE(platform, platform_id, slot_index)
);
```

### **4. Create `FPgSQLStore` Stub**
**File:** `Source/Onset/Public/Server/PgSQLStore.h` + `.cpp`
- Same interface, uses `libpq` (`PQconnectdb`, `PQexecParams`)
- Returns `false` / empty data with log warning for demo
- Configurable via `DataStore=Postgres` + connection string

### **5. Create `UOnsetPlayerDataSubsystem`**
**File:** `Source/Onset/Public/Multiplayer/OnsetPlayerDataSubsystem.h` + `.cpp`
```cpp
UCLASS()
class ONSET_API UOnsetPlayerDataSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    FOnsetAccountData GetAccountData(const FString& Platform, const FString& PlatformID);
    FOnsetFullCharacterData GetCharacterData(const FString& Platform, const FString& PlatformID, int32 SlotIndex);
    bool SaveCharacterData(const FString& Platform, const FString& PlatformID, int32 SlotIndex, const FOnsetFullCharacterData& Data);

private:
    TUniquePtr<IPlayerDataStore> Store;
};
```
- `Initialize()`: reads `DefaultEngine.ini` `[Onset.DataStore]` config, creates `FSQLiteStore` or `FPgSQLStore`, calls `Initialize()`
- `Deinitialize()`: calls `Store->SaveAll()`, `Store->Shutdown()`

### **6. Config**
`Config/DefaultEngine.ini`:
```ini
[Onset.DataStore]
DataStore=SQLite
SQLitePath=../../OnsetDB/playerdata.db
; DataStore=Postgres
; PgSQLConnectionString=host=localhost dbname=onset user=onset password=...
```

---

## **How to Test**
1. Add SQLite amalgamation, update Build.cs, compile
2. Launch DS: `Onset.exe ... -server -log`
3. Check log for:
   - `SQLite store initialized at ...`
   - `WAL mode enabled`
   - `Schema version: 1`
4. Open `OnsetDB/playerdata.db` in DB Browser for SQLite → verify tables exist
5. Stop DS → verify clean shutdown log: `SQLite store saved all, WAL checkpoint done`

---

## **Code Snippets**

```cpp
// IPlayerDataStore.h
virtual FOnsetAccountData LoadAccount(const FString& Platform, const FString& PlatformID) = 0;
virtual bool SaveCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex, const FOnsetFullCharacterData& Data) = 0;

// FSQLiteStore.cpp — RunMigrations
void FSQLiteStore::RunMigrations()
{
    int32 Version = 0;
    sqlite3_stmt* Stmt;
    if (sqlite3_prepare_v2(DBHandle, "SELECT version FROM _schema_version", -1, &Stmt, nullptr) == SQLITE_OK)
    {
        if (sqlite3_step(Stmt) == SQLITE_ROW) Version = sqlite3_column_int(Stmt, 0);
        sqlite3_finalize(Stmt);
    }
    if (Version < 1) { Exec("CREATE TABLE ..."); /* set version = 1 */ }
}

// UOnsetPlayerDataSubsystem.cpp
void UOnsetPlayerDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (!GetWorld()->IsServer()) return; // DS only

    FString StoreType = GConfig->GetStr(TEXT("/Script/Onset.OnsetSettings"), TEXT("DataStore"), TEXT("SQLite"), GEngineIni);
    if (StoreType == TEXT("SQLite"))
    {
        FString Path = GConfig->GetStr(TEXT("/Script/Onset.OnsetSettings"), TEXT("SQLitePath"), TEXT("../../OnsetDB/playerdata.db"), GEngineIni);
        Store = MakeUnique<FSQLiteStore>();
        Store->Initialize(Path);
    }
    // ... Postgres branch
}
```

---

## **Common Pitfalls**
- Forgetting to link `sqlite3.c` in Build.cs → unresolved symbols
- Not enabling WAL mode → DB locks under concurrent access
- String concatenation in SQL → injection bugs + broken migrations
- Forgetting `SaveAll()` in `Deinitialize()` → lost writes on DS shutdown

---

## **Dependencies**
- Requires Episode 39 (Steam auth ticket flow)
- Requires dedicated server build (Episodes 37-38)

---

## **Next Episode Preview**
Next time we extract the numeric SteamID from the auth ticket, store it on PlayerState, and implement the full save/load RPC flow — account auto-create on first login, character load on select, spawn from saved state.

---

## **Episode Checklist**
- [ ] SQLite amalgamation added and compiling
- [ ] `IPlayerDataStore` interface defined
- [ ] `FSQLiteStore` implements all methods with WAL + migrations
- [ ] `FPgSQLStore` stub compiles
- [ ] `UOnsetPlayerDataSubsystem` initializes store from config
- [ ] Schema v1 creates accounts/characters tables
- [ ] DS starts, opens DB, runs migrations, shuts down cleanly