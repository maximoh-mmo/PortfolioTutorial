# 🎬 Post 12 — Remember You: SQLite Persistence, SteamID Resolution, and the ServerTravel Gauntlet

*25 commits · Jun 29–30, 2026 · covering Sprint A5b Persistence & Account System*

---

## Introduction

Sprint A5 proved the game can run multiplayer. But when the player disconnects, everything is lost — no save, no character select, no account. Sprint A5b builds the persistence layer that makes death meaningful and progression real.

Twenty-five commits in roughly 38 hours. The arc opens with the data backbone (SQLite store, data structs, player data subsystem), adds the auth interface and SteamID extraction, wires the full login → character select → enter world RPC flow, and then spends a full day fixing the cascade of packaging and ServerTravel bugs that surface when you try to actually ship the thing. It closes with the OnsetDataStore module extraction (factory pattern, client/server split) and the PostgreSQL backend — because a demo that only works with SQLite isn't a demo that scales.

---

## Technical Exposition

### The store interface first

The `IPlayerDataStore` interface is the contract that everything else obeys. Gameplay never talks to a DB class directly; the subsystem configures the backend at startup:

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

Three backends implement this: SQLite (dev), PostgreSQL (self-hosted), and FHttpStore (Lambda + DynamoDB, added in A5c). The interface is the abstraction that makes all three interchangeable.

### Server-only data access

`UOnsetPlayerDataSubsystem` returns `false` for `ShouldCreateSubsystem()` on clients. Account data is never replicated — only sent via reliable RPCs. This is server-authoritative storage with anti-cheat integrity for free.

### The world-travel flow

The character select triggers a `ServerTravel` to the game map. But ServerTravel is a minefield of URL format issues, GameMode overrides, and map-path quirks. The arc documents every pitfall:

1. Full path `/Game/DemoLevel.DemoLevel` contains `.` which the engine rejects for `LongPackageNames` in ServerTravel → short name `DemoLevel`
2. The map's `GameModeOverride` (Blueprint) bypasses the lobby → explicit `?game=` override in the URL
3. `CharacterSelectWidgetClass` was never set in C++ → RPC could never instantiate the widget
4. `PlayerStateClass` was never set in GameMode → `GetPlayerState<AOnsetPlayerState>()` returned nullptr → `Client_AccountData` never sent

Each fix is a one-liner that would have taken hours to debug without the sprint's systematic approach.

---

## Implementation Details

### 1. The SQLite store with versioned migrations

```cpp
bool FSQLiteStore::EnsureSchema()
{
    Exec("CREATE TABLE IF NOT EXISTS _schema_version (version INTEGER PRIMARY KEY, applied_at TEXT NOT NULL DEFAULT (datetime('now')));");
    int32 Version = GetSchemaVersion();
    const int32 LatestVersion = 3;
    while (Version < LatestVersion)
    {
        RunMigration(Version);
        Version = GetSchemaVersion();
    }
    return true;
}
```

The migration runner loops from current to latest, applying each migration in order. This means existing dev accounts survive schema changes — the character class + appearance columns added in migration 3 don't break existing data.

### 2. PostLogin: SteamID extraction + account load-or-create

```cpp
FString Platform = TEXT("Steam");
FString PlatformID = TEXT("");
FUniqueNetIdRepl UniqueId = PS->GetUniqueId();
if (UniqueId.IsValid()) PlatformID = UniqueId->ToString();

PS->PlayerPlatform = Platform;
PS->PlayerPlatformID = PlatformID;

FOnsetAccountData AccountData;
if (!DataSubsystem->LoadAccount(Platform, PlatformID, AccountData))
{
    if (DataSubsystem->CreateAccount(Platform, PlatformID))
        DataSubsystem->LoadAccount(Platform, PlatformID, AccountData);
}
PC->Client_AccountData(AccountData);
```

The platform ID comes from the connection's unique net ID — no Steam SDK call needed at this layer. The `FSteamAuth` wrapper (added in a parallel commit) is deliberately *not wired in yet* — the interface exists for future platforms.

### 3. The RPC suite

Six RPCs form the full account lifecycle:

| RPC | Direction | Purpose |
|---|---|---|
| `Client_AccountData` | Server → Client | Sends 3 empty slots for new accounts |
| `Client_CharacterData` | Server → Client | Full character data after select |
| `Client_SaveComplete` | Server → Client | Confirms save succeeded |
| `Server_SelectCharacter` | Client → Server | Loads character, triggers travel |
| `Server_CreateCharacter` | Client → Server | Creates new character in slot |
| `Server_SaveCharacter` | Client → Server | Manual save trigger |

### 4. The canvas HUD fallback

UMG widgets silently don't render in packaged `-game` builds. The arc's pragmatic answer was canvas-based `AHUD` fallbacks:

```cpp
// OnsetLobbyHUD.cpp — keys 1/2/3 select slot, Enter confirms
if (PC->WasInputKeyJustPressed(EKeys::One))   SelectedSlot = 0;
if (PC->WasInputKeyJustPressed(EKeys::Two))   SelectedSlot = 1;
if (PC->WasInputKeyJustPressed(EKeys::Three)) SelectedSlot = 2;
if (SelectedSlot >= 0 && PC->WasInputKeyJustPressed(EKeys::Enter))
    PC->Server_SelectCharacter(SelectedSlot);
```

Pressing a number on an empty slot now calls `Server_CreateCharacter(i, "Hero_N")` — the create-and-enter flow that makes the demo feel complete.

### 5. The OnsetDataStore module extraction

All store code moves to a new `Source/OnsetDataStore/` module. The factory pattern decouples the subsystem from concrete stores:

```cpp
TUniquePtr<IPlayerDataStore> CreateDataStore(const FString& Type, const FString& ConnectionString, bool& bOutSuccess)
{
    bOutSuccess = false;
#ifndef ONSETDATASTORE_CLIENT_ONLY
    if (Type.Equals(TEXT("SQLite"), ESearchCase::IgnoreCase))
    {
        TUniquePtr<FSQLiteStore> Store = MakeUnique<FSQLiteStore>();
        bOutSuccess = Store->Initialize(ConnectionString);
        if (bOutSuccess) return Store;
    }
    // ... PgSQL, HttpApi branches
#endif
    return nullptr;
}
```

Client builds define `ONSETDATASTORE_CLIENT_ONLY` and never link SQLiteCore or libpq — no DB DLLs or credentials ship to players.

### 6. The PostgreSQL backend

`FPgSQLStore` vendors libpq 17.10 (EDB) into `ThirdParty/PostgreSQL/`, implements all 7 interface methods with PG `$1` params and `INSERT ... ON CONFLICT DO UPDATE` upserts, same migration system as SQLite. The Build.cs is platform-aware: Win64 → bundled DLLs, Linux → system libpq (`/usr/include/postgresql`, `-lpq`).

---

## Results & Validation

- `IPlayerDataStore` interface with SQLite, PostgreSQL, and HTTP backends
- `UOnsetPlayerDataSubsystem` — DS-only, config-selected store, server-authoritative
- Full RPC suite: account load, character CRUD, auto-save, save-on-disconnect
- `ServerTravel` world-travel flow with zone gates and `CurrentZone`
- Canvas HUD fallback for packaged builds (UMG silent failure)
- `OnsetDataStore` module with factory pattern + `ONSETDATASTORE_CLIENT_ONLY`
- `FPgSQLStore` with libpq 17.10, cross-platform Build.cs
- A5b = 30/30 (100%)

---

## Challenges & Solutions

### SQLite open failure aborts cooking

**Problem:** `sqlite3_open` failure logged via `UE_LOG(Error)` fails the cook commandlet.

**Solution:** Downgrade to `Warning`. Then add `IsRunningCommandlet()` guards. Then discover `UnrealEditor-Cmd.exe` *is* detected as a commandlet, so the guards block the DS runtime. Remove the guards. The final state: Warning-level logging with no commandlet guards — cook is safe because the DS is never run in cook.

### `CharacterSelectWidgetClass` never initialized

**Problem:** `EditDefaultsOnly TSubclassOf` set nowhere in C++, so `Client_AccountData` could never instantiate the widget.

**Solution:** Initialize in the GameMode constructor + add a runtime guard for BP CDO overriding it to None.

### `ServerTravel` URL format wars

**Problem:** The URL went through four mutations across five commits — stale Blueprint path → C++ path → short name → GameMode override fix. Each mutation was a one-liner, but the cumulative debugging would have been hours without the systematic approach.

**Solution:** The final URL format is `DemoLevel` (short name) with explicit `?game=/Script/Onset.OnsetGameModeBase` override. The short name avoids the `.` in `LongPackageNames` rejection; the explicit `?game=` overrides the map's Blueprint GameMode override.

### UMG doesn't render in packaged `-game` builds

**Problem:** `UUserWidget` subclasses silently don't render in packaged `-game` builds. The arc's pragmatic answer was canvas-based `AHUD` fallbacks.

**Solution:** `AOnsetLobbyHUD` and `AOnsetMenuHUD` with immediate-mode canvas drawing + `WasInputKeyJustPressed`. This is documented as "HUD over UMG for menus (OUTDATED)" — CommonUI's `UCommonActivatableWidget` (added in Post 13) finally solves packaged `-game` UMG rendering.

### libpq DLL dependency chain

**Problem:** `FPgSQLStore` staged 5 DLLs but was missing `libwinpthread-1.dll` (a dependency of `libintl-9`).

**Solution:** Added the missing DLL + made Build.cs platform-aware (Win64 bundled vs Linux system libpq).

---

## Reflection & Lessons Learned

The `IPlayerDataStore` interface is the arc's most durable idea. By abstracting the storage backend behind a pure-virtual interface, the entire account system is decoupled from the persistence implementation. When A5c adds `FHttpStore` (Lambda + DynamoDB), zero GameMode or PlayerController code changes — just a new backend and a config key. The interface makes the system future-proof.

The ServerTravel URL debugging is the arc's cautionary tale. Each one-liner fix was correct at the moment it was made, but the cumulative effect was a URL format that took 5 commits to stabilize. This is why the sprint plan included a "ServerTravel path" checklist item — the kind of thing that's trivial to verify and catastrophic to miss.

The canvas HUD fallback taught me that UMG's packaged `-game` rendering is a platform-specific quirk that can't be worked around with good intentions. When the toolchain fails silently (widgets compile, look fine in PIE, and don't render in package), the pragmatic answer is a different rendering path entirely.

---

## Forward-Looking Content

| Phase | Status |
|---|---|
| A5b — Persistence & Account System | ✅ **100% (30/30)** |
| A5c — Auth Extraction & Login Server | ⏭️ Next |
| CommonUI UI Migration | Planned |
| Character Classes + Final Polish | Planned |

**What's next:** the persistence backbone is solid, but the menus are still canvas-drawn HUDs. Post 13 tears them out and migrates everything to CommonUI — a screen stack with Game/Menu/Modal layers, Blueprint-authored screens, and a GameInstanceSubsystem that owns navigation. Then Post 14 adds the cloud account API and two-server token auth. And Post 15 delivers the final polish: character classes, loading screens, and API access control hardening.

> **Next time in Post 13:** Menus That Actually Stack — rebuilding Onset's UI on a CommonUI screen-stack subsystem.

---

*Arc commits: `c7e4327` → `2d583b2`. Related docs: `Docs/Server/Persistence_Data_Store.md`, `Docs/Player/Account_System.md`, `Docs/Multiplayer/Multiplayer_System.md`, `Planning/Scripts/Episode40–43_*.md`.*
