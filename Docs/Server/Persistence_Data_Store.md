# 📘 **Persistence Data Store**

**File:** `Docs/Server/Persistence_Data_Store.md`

---

## **Purpose**

Provide a clean, swappable persistence backend for the dedicated server. The DS reads a config key at startup and instantiates the appropriate store implementation — SQLite for dev/demo, PostgreSQL or HTTP API (Lambda + DynamoDB) for production.

---

## **Responsibilities**

- Abstract all database operations behind a pure virtual interface  
- Implement SQLite storage with WAL mode and migration support  
- Implement PostgreSQL storage with the same queries and schema  
- Implement HTTP API store (`FHttpStore`) — proxies all calls to a remote REST API (Lambda + DynamoDB)  
- Provide safe concurrent access (thread-safe reads/writes)  
- Handle schema versioning and forward-only migrations  

---

## **Non‑Responsibilities**

- Gameplay logic (account/character flow is in the Account System)  
- Network transport (RPCs are in PlayerController)  
- Cache invalidation (all reads go to the store; no write-through cache in v1)  

---

## **Key Concepts**

### **Store Abstraction**

```
IPlayerDataStore  ← abstract interface (in Source/OnsetDataStore/Public/)
      │
      ├── FSQLiteStore   ← file-based, zero dependencies, dev/demo
      │
      ├── FPgSQLStore    ← libpq-based, production
      │
      └── FHttpStore     ← REST API-backed, serverless production (Lambda + DynamoDB)
```

**Module structure:** All data store code lives in `Source/OnsetDataStore/`, a separate module from `Onset`. The `OnsetDataStore` module conditionally links `SQLiteCore` — only for non-client targets. Client builds compile only the interface (`IPlayerDataStore`), factory (`DataStoreFactory`), and data types (`OnsetPlayerDataTypes`). Store implementations (`FSQLiteStore`, `FPgSQLStore`) are excluded via the `ONSETDATASTORE_CLIENT_ONLY` define.

The `Onset` module accesses stores only through the factory function:

```cpp
// DataStoreFactory.h — single entry point
TUniquePtr<IPlayerDataStore> CreateDataStore(const FString& Type, const FString& ConnectionString, bool& bOutSuccess);
```

The DS selects the implementation via config:

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

**Secrets never ship to clients.** `DefaultEngine.ini` is deliberately kept secret-free (it is baked into packaged clients). Real store credentials live only in server-only config layers (`Config/WindowsServer/WindowsServerEngine.ini`, `Config/LinuxServer/LinuxServerEngine.ini`) or are injected at launch:

```
-OnsetDataStoreType=HttpApi
-OnsetDataStoreURL=<host>
-OnsetDataStoreAPIKey=<key>
-OnsetAuthTokenSecret=<secret>
```

`Test_All.ps1` passes these via command line so local `UnrealEditor -server` dev keeps working after the strip.

Note: For `HttpApi`, the `ConnectionString` stores only the host (no `https://` prefix) — the store prepends it at runtime to avoid INI parser truncation.

**Per-request authorization:** every `FHttpStore` request also carries an `X-Store-Token` header — a short-lived HMAC-SHA256 token signed with `[Onset.Auth] AuthTokenSecret` and bound to the account (`Platform` + `PlatformID`) being accessed. The Lambda verifies it, so a leaked API key alone cannot read arbitrary accounts. See [Account API](../Server/Account_Api.md).

### **Schema**

Two tables plus a version tracker. SQLite DDL (v1 migration):

```sql
CREATE TABLE _schema_version (
    version INTEGER PRIMARY KEY,
    applied_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE accounts (
    platform      TEXT NOT NULL,
    platform_id   TEXT NOT NULL,
    created_at    TEXT NOT NULL DEFAULT (datetime('now')),
    last_login    TEXT NOT NULL DEFAULT (datetime('now')),
    PRIMARY KEY (platform, platform_id)
);

CREATE TABLE characters (
    platform          TEXT NOT NULL,
    platform_id       TEXT NOT NULL,
    slot_index        INTEGER NOT NULL CHECK(slot_index >= 0 AND slot_index <= 2),
    character_name    TEXT NOT NULL DEFAULT '',
    level             INTEGER NOT NULL DEFAULT 1,
    experience        INTEGER NOT NULL DEFAULT 0,
    saved_max_health  REAL NOT NULL DEFAULT 100.0,
    saved_position_x  REAL NOT NULL DEFAULT 0.0,
    saved_position_y  REAL NOT NULL DEFAULT 0.0,
    saved_position_z  REAL NOT NULL DEFAULT 0.0,
    saved_rotation_yaw REAL NOT NULL DEFAULT 0.0,
    inventory_json    TEXT NOT NULL DEFAULT '{}',
    equipment_json    TEXT NOT NULL DEFAULT '{}',
    quests_json       TEXT NOT NULL DEFAULT '{}',
    created_at        TEXT NOT NULL DEFAULT (datetime('now')),
    updated_at        TEXT NOT NULL DEFAULT (datetime('now')),
    PRIMARY KEY (platform, platform_id, slot_index),
    FOREIGN KEY (platform, platform_id) REFERENCES accounts(platform, platform_id) ON DELETE CASCADE
);
```

Migration 2 adds `current_zone TEXT NOT NULL DEFAULT ''`; migration 3 adds `character_class INTEGER NOT NULL DEFAULT 0` and `appearance_json TEXT NOT NULL DEFAULT '{}'`.

Key design choices:
- **Composite PK `(platform, platform_id)` for accounts; `(platform, platform_id, slot_index)` for characters** — no collisions between Steam `"7656119..."` and future Xbox `"XUID..."`; a character's slot is part of its identity  
- **JSON blobs** (`inventory_json`, `equipment_json`, `quests_json`) — extensible without schema changes; version within the blob  
- **All queries are parametrized** — identical across SQLite and PostgreSQL; only the connection code differs  
- **`TEXT` for timestamps** — ISO 8601 strings, portable across DB engines  
- **`character_name` defaults to `''`** (not a placeholder name) — identity is supplied by `Server_CreateCharacter` and preserved by `SaveCharacterPreservingIdentity` (see [Account System](../Player/Account_System.md))

### **Migration System**

On startup, `EnsureSchema()` creates the `_schema_version` table if missing, reads the current version, and runs each missing migration sequentially via `RunMigration(int32 FromVersion)` until `LatestVersion` (currently **3**) is reached:

```cpp
const int32 LatestVersion = 3;

void FSQLiteStore::EnsureSchema()
{
    // CREATE TABLE IF NOT EXISTS _schema_version (version INTEGER PRIMARY KEY, applied_at ...)
    int32 Version = GetSchemaVersion();   // SELECT COALESCE(MAX(version), 0) FROM _schema_version;
    while (Version < LatestVersion)
        RunMigration(Version);            // applies one migration and bumps _schema_version
}

void FSQLiteStore::RunMigration(int32 FromVersion)
{
    if (FromVersion == 0) { /* CREATE TABLE accounts, characters */ }
    if (FromVersion <= 1) { /* ALTER TABLE characters ADD COLUMN current_zone */ }
    if (FromVersion <= 2) { /* ALTER TABLE characters ADD COLUMN character_class + appearance_json */ }
}
```

Migrations run **without an explicit transaction** — each statement is executed individually and `_schema_version` is bumped as a separate insert. On an interruption mid-migration, the already-applied statements persist; on the next startup `EnsureSchema()` re-runs the remaining steps (idempotent for `ALTER TABLE`-style additions).

---

## **Key Classes**

### **`IPlayerDataStore`** (abstract interface)

```cpp
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

Notes:
- There is **no `Shutdown()`** — stores flush via `SaveAll()` (called from the subsystem's `Deinitialize()` and the store destructors).
- `LoadAccount` / `LoadCharacter` return `bool` and fill out-params.
- `SaveCharacter` takes the slot from `Data.SlotIndex` — there is no separate `SlotIndex` parameter.
- There is **no `CreateCharacter`** — characters are created by the `SaveCharacter` upsert (`PUT /character` on the HTTP store). `DeleteCharacter` removes a character row.

### **`FSQLiteStore`**
- Owns `sqlite3*` handle
- Opens file, enables WAL mode (`PRAGMA journal_mode=WAL`), `synchronous=NORMAL`, `foreign_keys=ON`, runs migrations on `Initialize()`
- All queries use `sqlite3_prepare_v2` + `sqlite3_bind_*` (no string concatenation)
- Multi-row reads (e.g. `LoadAccount` slot listing) run as plain sequential queries — no explicit transaction blocks
- `SaveAll()` runs `PRAGMA wal_checkpoint(TRUNCATE)` to flush the WAL

### **`FPgSQLStore`**
- Owns `PGconn*` handle
- Same queries (parametrized with `$1`, `$2`, etc.)  
- `Initialize()` connects and runs migrations
- `SaveAll()` runs `CHECKPOINT;` (PostgreSQL handles most flushing internally)

### **`FHttpStore`**
- No database connection — proxies all calls to a remote REST API via `FHttpModule`
- `Initialize()` constructs the base URL from config (prepends `https://`)
- All requests use `FHttpModule::Get().CreateRequest()` with blocking spin-loop + `GetHttpManager().Tick(0.f)` to pump callbacks on the game thread
- Serializes requests/responses as JSON (FJsonObject/FJsonObjectConverter)
- `SaveAll()` is a no-op (API is transactional per-call)
- API key sent via `X-API-Key` header for all requests
- `SaveCharacter` sends `characterClass` + `appearanceJson` in the request body

### **`UOnsetPlayerDataSubsystem`**
- World subsystem, DS only
- `OnWorldBeginPlay()` reads config, calls `CreateDataStore()` (factory function in `DataStoreFactory.h`); on failure or a null result it retries with the fallback store type before giving up
- Caches the `IPlayerDataStore*` for the lifetime of the DS
- `Deinitialize()` calls `StopAutoSaveTimer()`, `SaveAll()`, then `Store.Reset()`
- Runs a periodic auto-save timer (default 300s) that calls `SaveAll()`

---

## **Data Flow**

```
DS Startup
    │
    ├── UOnsetPlayerDataSubsystem::OnWorldBeginPlay()
    │       │
    │       ├── Read Config (Type=SQLite|Postgres|HttpApi, ConnectionString)
    │       │
    │       ├── CreateDataStore(Type, ConnectionString, bOutSuccess)  ← from OnsetDataStore module
    │       │       │
    │       │       ├── [Server build]: new FSQLiteStore → Initialize(Path)
    │       │       │       ├── sqlite3_open(path)
    │       │       │       ├── PRAGMA journal_mode=WAL
    │       │       │       ├── EnsureSchema() / RunMigration()
    │       │       │       └── Ready
    │       │       │
    │       │       └── (or) new FPgSQLStore / FHttpStore → Initialize(...)
    │       │
    │       ├── [Client build]: returns nullptr (stores compiled out via ONSETDATASTORE_CLIENT_ONLY)
    │       │
    │       └── Caches IPlayerDataStore* for subsystem lifetime
    │
    └── Ready for player connections
```

---

## **Interactions With Other Systems**

- **[Account System](../Player/Account_System.md)** — consumer of all store methods; translates between USTRUCTs and DB rows  
- **[Account API](../Server/Account_Api.md)** — the Lambda + DynamoDB backend proxied by `FHttpStore` when `Type=HttpApi`  
- **[Multiplayer](../Multiplayer/Multiplayer_System.md)** — runs server-only on the DS, never on clients  
- **[Steam Integration](../Steam/Steam_Integration_System.md)** — provides the platform ID used as the primary key  

---

## **Replication Rules**

- The data store is **DS-only**. Clients never connect to the database directly.  
- All data travels to/from clients via RPCs on `AOnsetPlayerController`.  
- The `OnsetDataStore` module compiles store implementations out on client targets (`ONSETDATASTORE_CLIENT_ONLY`). Client builds link only the interface + data types.  
- `DataStoreFactory` on a client build returns `nullptr` — `UOnsetPlayerDataSubsystem` validates the pointer before use.  

---

## **Edge Cases**

- **DB file missing on DS start** — SQLite creates it automatically  
- **DB file read-only** — `Initialize()` fails, DS logs error and refuses connections  
- **Migration from unknown version** — version 0 (no `_schema_version` table) treated as fresh install  
- **Concurrent writes (same SteamID)** — only one DS instance per account; serialized by the store  
- **DS crashes mid-migration** — partial statements persist (no transaction); `EnsureSchema()` re-runs the remaining steps on next startup  
- **Store creation failure** — `CreateDataStore` returns false/null; the subsystem logs the error and falls back to the default store type with a warning (any type, not just Postgres)

---

## **Testing Checklist**

- [ ] SQLite store creates DB file on first Initialize()
- [ ] WAL mode enabled after Initialize()
- [ ] Schema migrations run in order
- [ ] LoadAccount returns correct slots
- [ ] SaveCharacter persists and can be read back
- [ ] Same queries pass unchanged for SQLite and PostgreSQL
- [ ] Migrations idempotent (re-running same version does nothing)
- [ ] FPgSQLStore connects (with valid connection string)
- [ ] Config switch between stores works at startup
- [ ] Deinitialize() flushes all pending writes via SaveAll()
- [ ] DB file survives DS restart (read-after-reboot)

---

## **Future Extensions**

- **Read replica** — separate connection for reads vs writes  
- **Connection pooling** — for PgSQL, maintain a pool of connections  
- **Async writes** — queue save requests, batch-write on timer  
- **Backup** — periodic `VACUUM INTO` (SQLite) or `pg_dump` (PostgreSQL)  
- **Migration CI** — test that migration from every prior version produces correct schema  
