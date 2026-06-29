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
| `Source/Onset/ThirdParty/SQLite/sqlite3.h` | SQLite amalgamation header |
| `Source/Onset/ThirdParty/SQLite/sqlite3.c` | SQLite amalgamation source |
| `Source/Onset/Public/Server/IPlayerDataStore.h` | Abstract store interface |
| `Source/Onset/Public/Server/SQLiteStore.h` | SQLite implementation header |
| `Source/Onset/Private/Server/SQLiteStore.cpp` | SQLite implementation |
| `Source/Onset/Public/Server/PgSQLStore.h` | PgSQL stub header |
| `Source/Onset/Private/Server/PgSQLStore.cpp` | PgSQL stub |
| `Source/Onset/Public/Multiplayer/OnsetPlayerDataSubsystem.h` | DS world subsystem header |
| `Source/Onset/Private/Multiplayer/OnsetPlayerDataSubsystem.cpp` | DS world subsystem |
| `Source/Onset/Onset.Build.cs` | Added SQLite include path + lib |
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
1. Add SQLite amalgamation, update Build.cs, compile
2. Launch DS: `Onset.exe ... -server -log`
3. Check log for:
   - `SQLite store initialized at ...`
   - `WAL mode enabled`
   - `Schema version: 1`
4. Open `OnsetDB/playerdata.db` in DB Browser → verify tables exist
5. Stop DS → verify clean shutdown: `SQLite store saved all, WAL checkpoint done`

---

## **Code Snippets**

```cpp
// IPlayerDataStore.h
class IPlayerDataStore {
public:
    virtual ~IPlayerDataStore() = default;
    virtual bool Initialize(const FString& ConnectionString) = 0;
    virtual void Shutdown() = 0;
    virtual FOnsetAccountData LoadAccount(const FString& Platform, const FString& PlatformID) = 0;
    virtual FOnsetFullCharacterData LoadCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex) = 0;
    virtual bool SaveCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex, const FOnsetFullCharacterData& Data) = 0;
    virtual void SaveAll() = 0;
};

// FSQLiteStore.cpp — RunMigrations
void FSQLiteStore::RunMigrations() {
    int32 Version = 0;
    // ... read _schema_version ...
    if (Version < 1) { Exec("CREATE TABLE accounts ..."); Exec("CREATE TABLE characters ..."); Exec("CREATE TABLE _schema_version ..."); SetVersion(1); }
}

// UOnsetPlayerDataSubsystem.cpp
void UOnsetPlayerDataSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
    Super::Initialize(Collection);
    if (!GetWorld()->IsServer()) return;
    FString StoreType = GConfig->GetStr(TEXT("/Script/Onset.OnsetSettings"), TEXT("DataStore"), TEXT("SQLite"), GEngineIni);
    if (StoreType == TEXT("SQLite")) {
        FString Path = GConfig->GetStr(TEXT("/Script/Onset.OnsetSettings"), TEXT("SQLitePath"), TEXT("../../OnsetDB/playerdata.db"), GEngineIni);
        Store = MakeUnique<FSQLiteStore>();
        Store->Initialize(Path);
    }
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
    │       ├── Read Config (DataStore=SQLite|Postgres)
    │       │
    │       ├── new FSQLiteStore → Initialize(Path)
    │       │       │
    │       │       ├── sqlite3_open(path)
    │       │       ├── PRAGMA journal_mode=WAL
    │       │       ├── RunMigrations()
    │       │       └── Ready
    │       │
    │       └── (or) new FPgSQLStore → Initialize(ConnString)
    │
    └── Ready for player connections
```

---

## **Common Pitfalls**
- Forgetting to link `sqlite3.c` in Build.cs → unresolved symbols
- Not enabling WAL mode → DB locks under concurrent access
- String concatenation in SQL → injection bugs + broken migrations
- Forgetting `SaveAll()` in `Deinitialize()` → lost writes on DS shutdown

---

## **Episode Checklist**
- [ ] SQLite amalgamation added and compiling
- [ ] `IPlayerDataStore` interface defined
- [ ] `FSQLiteStore` implements all methods with WAL + migrations
- [ ] `FPgSQLStore` stub compiles
- [ ] `UOnsetPlayerDataSubsystem` initializes store from config
- [ ] Schema v1 creates accounts/characters tables
- [ ] DS starts, opens DB, runs migrations, shuts down cleanly
- [ ] Snapshot is clean (no stale assets, no temp files)
- [ ] README updated for public repo