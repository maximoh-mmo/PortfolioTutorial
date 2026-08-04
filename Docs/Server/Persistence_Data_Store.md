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
IPlayerDataStore* CreateDataStore(const FString& StoreType, const FString& ConnectionString);
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

Two tables plus a version tracker:

```sql
CREATE TABLE _schema_version (
    version INTEGER PRIMARY KEY
);

CREATE TABLE accounts (
    platform_id   TEXT NOT NULL,
    platform      TEXT NOT NULL,
    created_at    TEXT NOT NULL DEFAULT (datetime('now')),
    last_login    TEXT,
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

Key design choices:
- **Composite PK `(platform, platform_id)`** — no collisions between Steam `"7656119..."` and future Xbox `"XUID..."`  
- **JSON blobs** (`inventory_json`, `equipment_json`, `quests_json`) — extensible without schema changes; version within the blob  
- **All queries are parametrized** — identical across SQLite and PostgreSQL; only the connection code differs  
- **`TEXT` for timestamps** — ISO 8601 strings, portable across DB engines  

### **Migration System**

On startup, the store reads `_schema_version`. If version < current (hardcoded in code), it runs each missing migration sequentially inside a transaction:

```cpp
int32 CurrentSchemaVersion = 1;

void FSQLiteStore::RunMigrations()
{
    int32 Version = ReadSchemaVersion();
    if (Version < 1) { /* CREATE TABLE accounts, characters, _schema_version */ }
    if (Version < 2) { /* ALTER TABLE characters ADD COLUMN ... */ }
    // Each migration increments _schema_version
}
```

---

## **Key Classes**

### **`IPlayerDataStore`** (abstract interface)

```cpp
class IPlayerDataStore
{
public:
    virtual ~IPlayerDataStore() = default;

    virtual bool Initialize(const FString& ConnectionString) = 0;
    virtual void Shutdown() = 0;

    virtual FOnsetAccountData LoadAccount(const FString& Platform, const FString& PlatformID) = 0;
    virtual FOnsetFullCharacterData LoadCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex) = 0;
    virtual bool SaveCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex, const FOnsetFullCharacterData& Data) = 0;
    virtual bool CreateAccount(const FString& Platform, const FString& PlatformID) = 0;
    virtual bool CreateCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex, const FString& CharacterName) = 0;
    virtual void SaveAll() = 0;
};
```

### **`FSQLiteStore`**
- Owns `sqlite3*` handle
- Opens file, enables WAL mode, runs migrations on `Initialize()`
- All queries use `sqlite3_prepare_v2` + `sqlite3_bind_*` (no string concatenation)
- Transactions on multi-row operations
- `SaveAll()` calls `sqlite3_wal_checkpoint()` to flush WAL

### **`FPgSQLStore`**
- Owns `PGconn*` handle
- Same queries (parametrized with `$1`, `$2`, etc.)  
- `Initialize()` connects and runs migrations
- `SaveAll()` is a no-op (PostgreSQL handles this internally)

### **`FHttpStore`**
- No database connection — proxies all calls to a remote REST API via `FHttpModule`
- `Initialize()` constructs the base URL from config (prepends `https://`)
- All requests use `FHttpModule::Get().CreateRequest()` with blocking spin-loop + `GetHttpManager().Tick(0.f)` to pump callbacks on the game thread
- Serializes requests/responses as JSON (FJsonObject/FJsonObjectConverter)
- `SaveAll()` is a no-op (API is transactional per-call)
- API key sent via `X-API-Key` header for all requests

### **`UOnsetPlayerDataSubsystem`**
- World subsystem, DS only
- `OnWorldBeginPlay()` reads config, calls `CreateDataStore()` (factory function in `DataStoreFactory.h`)
- Caches the `IPlayerDataStore*` for the lifetime of the DS
- `BeginDestroy()` calls `Store->SaveAll()`

---

## **Data Flow**

```
DS Startup
    │
    ├── UOnsetPlayerDataSubsystem::Initialize()
    │       │
    │       ├── Read Config (DataStore=SQLite|Postgres, path/connection string)
    │       │
    │       ├── CreateDataStore(StoreType, ConnectionString)  ← from OnsetDataStore module
    │       │       │
    │       │       ├── [Server build]: new FSQLiteStore → Initialize(Path)
    │       │       │       ├── sqlite3_open(path)
    │       │       │       ├── PRAGMA journal_mode=WAL
    │       │       │       ├── RunMigrations()
    │       │       │       └── Ready
    │       │       │
    │       │       └── (or) new FPgSQLStore → Initialize(ConnString)
    │       │
    │       ├── [Client build]: returns stub/null (stores compiled out via ONSETDATASTORE_CLIENT_ONLY)
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
- **DS crashes mid-migration** — transaction rolls back, retries on next startup  
- **PgSQL connection failure** — log error, fall back to SQLite with warning (configurable)  

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
- [ ] BeginDestroy() flushes all pending writes
- [ ] DB file survives DS restart (read-after-reboot)

---

## **Future Extensions**

- **Read replica** — separate connection for reads vs writes  
- **Connection pooling** — for PgSQL, maintain a pool of connections  
- **Async writes** — queue save requests, batch-write on timer  
- **Backup** — periodic `VACUUM INTO` (SQLite) or `pg_dump` (PostgreSQL)  
- **Migration CI** — test that migration from every prior version produces correct schema  
