# 🎬 **Episode 40 — Database Architecture & Player Data Subsystem**

## **Goal**
Design the `IPlayerDataStore` abstraction, implement the SQLite store with schema migrations, and create the `UOnsetPlayerDataSubsystem` world subsystem on the dedicated server.

---

## **What Was Built**
A clean interface (`IPlayerDataStore`) that the DS uses for all reads/writes, an SQLite implementation with WAL mode and versioned migrations, a PostgreSQL stub for production, and a world subsystem that owns the store and provides typed access to account/character data.

---

## **Project Snapshot**
This folder contains the **full Unreal project** as it stands at the end of Episode 40.

### **Key Files Added/Modified**
| File | Description |
|---|---|
| `Source/OnsetDataStore/Public/IPlayerDataStore.h` | Abstract store interface |
| `Source/OnsetDataStore/Public/FSQLiteStore.h` | SQLite implementation header |
| `Source/OnsetDataStore/Private/FSQLiteStore.cpp` | SQLite implementation (WAL, migrations, parametrized queries) |
| `Source/OnsetDataStore/Public/FPgSQLStore.h` | PgSQL stub header |
| `Source/OnsetDataStore/Private/FPgSQLStore.cpp` | PgSQL stub |
| `Source/OnsetDataStore/Public/DataStoreFactory.h` | Store factory (SQLite / PgSQL / HTTP) |
| `Source/OnsetDataStore/Private/DataStoreFactory.cpp` | Store factory impl |
| `Source/OnsetDataStore/Public/OnsetPlayerDataTypes.h` | Shared account/character structs |
| `Source/OnsetDataStore/OnsetDataStore.Build.cs` | Links engine `SQLiteCore` + `HTTP` (no bundled `sqlite3.c`) |
| `Source/Onset/Public/Subsystem/OnsetPlayerDataSubsystem.h` | DS world subsystem header |
| `Source/Onset/Private/Subsystem/OnsetPlayerDataSubsystem.cpp` | DS world subsystem |
| `Config/DefaultEngine.ini` | `[Onset.DataStore]` config section |

### **New Classes**
| Class | Purpose |
|---|---|
| `IPlayerDataStore` | Pure virtual interface for all persistence operations |
| `FSQLiteStore` | SQLite backend with WAL mode, migrations, parametrized queries |
| `FPgSQLStore` | PostgreSQL backend (stub for demo, compiles against libpq) |
| `UOnsetPlayerDataSubsystem` | World subsystem (DS only) that owns the store |

---

## **How to Test**
1. Compile the `OnsetDataStore` module (uses engine `SQLiteCore`, no amalgamation to add)
2. Launch DS: `Onset.exe ... -server -log`
3. Check log for:
   - `UOnsetPlayerDataSubsystem: initialized with SQLite (success=1)`
   - `FSQLiteStore: opened ... (existing=0)` on first run
   - `FSQLiteStore: migration N applied` for each of the 3 migrations
4. Open `Project/Saved/OnsetPlayerData.db` in DB Browser → verify `accounts`, `characters`, `_schema_version` tables exist
5. Stop DS → verify clean shutdown log: `SaveAll` flushed pending writes

---

## **Code Snippets**

```cpp
// IPlayerDataStore.h
class IPlayerDataStore {
public:
    virtual ~IPlayerDataStore() = default;
    virtual bool Initialize(const FString& ConnectionString) = 0;
    virtual void Shutdown() = 0;
    virtual bool LoadAccount(const FString& Platform, const FString& PlatformID, FOnsetAccountData& OutAccount) = 0;
    virtual bool CreateAccount(const FString& Platform, const FString& PlatformID) = 0;
    virtual bool LoadCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex, FOnsetFullCharacterData& OutData) = 0;
    virtual bool SaveCharacter(const FString& Platform, const FString& PlatformID, const FOnsetFullCharacterData& Data) = 0;
    virtual bool DeleteCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex) = 0;
    virtual void SaveAll() = 0;
};

// FSQLiteStore.cpp — EnsureSchema (3 migrations)
void FSQLiteStore::EnsureSchema() {
    // v1: accounts + characters
    // v2: current_zone column
    // v3: character_class + appearance_json columns
    // Each version guarded by _schema_version and applied in order.
}

// UOnsetPlayerDataSubsystem.cpp
void UOnsetPlayerDataSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
    Super::Initialize(Collection);
    if (!GetWorld()->IsServer()) return;
    FString DataStoreType, ConnectionString;
    GConfig->GetString(TEXT("Onset.DataStore"), TEXT("Type"), DataStoreType, GEngineIni);
    GConfig->GetString(TEXT("Onset.DataStore"), TEXT("ConnectionString"), ConnectionString, GEngineIni);
    // -OnsetDataStoreType= / -OnsetDataStoreURL= command-line overrides
    Store = CreateDataStore(DataStoreType, ConnectionString, bInitialized);
}
```

---

## **Dependencies**
- Requires Episode 39 (Steam auth ticket flow)
- Requires dedicated server build (Episodes 37-38)

---

## **Diagrams**

```
DS Startup
    │
    ├── UOnsetPlayerDataSubsystem::Initialize()
    │       │
    │       ├── Read Config ([Onset.DataStore] Type / ConnectionString)
    │       │
    │       ├── CreateDataStore("SQLite", ...)
    │       │       │
    │       │       ├── FSQLiteStore::Initialize → open Project/Saved/OnsetPlayerData.db
    │       │       ├── PRAGMA journal_mode=WAL
    │       │       ├── EnsureSchema() → migrations v1→v3
    │       │       └── Ready
    │       │
    │       └── (or) FPgSQLStore / FHttpStore
    │
    └── Ready for player connections
```

---

## **Common Pitfalls**
- SQLite is linked via the engine `SQLiteCore` module — do not bundle `sqlite3.c`/libpq sources into the module
- Not enabling WAL mode → DB locks under concurrent access
- String concatenation in SQL → injection bugs + broken migrations
- Forgetting `SaveAll()` in `Deinitialize()` → lost writes on DS shutdown
- Client connections must skip store init (`ShouldCreateSubsystem` server-only)

---

## **Episode Checklist**
- [x] `IPlayerDataStore` interface defined (bool + out-param signature)
- [x] `FSQLiteStore` implements all methods with WAL + migrations
- [x] `FPgSQLStore` stub compiles
- [x] `UOnsetPlayerDataSubsystem` initializes store from `[Onset.DataStore]` config
- [x] Schema migrations v1→v3 create accounts/characters tables
- [x] DS starts, opens DB, runs migrations, shuts down cleanly
- [x] Snapshot is clean (no stale assets, no temp files)
- [x] README updated for public repo