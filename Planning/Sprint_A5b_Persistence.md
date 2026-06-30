# 🏃 Sprint A5b: Player Persistence & Account System

**Goal:** Add persistent player data, character slots, Steam ID extraction, and a full login → character select → enter world flow on the dedicated server.
**Target:** Player authenticates via Steam, sees a character select screen with up to 3 saved characters, picks one, and spawns in the game world with full state restored.
**Dependencies:** A5.1 (Authority), A5.2 (Replication), A5.3 (Dedicated Server), A5.4 (Steam Auth — ticket validation, LAN fallback)

---

## 📊 Sprint Overview

| Section | Tasks | Est. Days |
|---------|-------|-----------|
| Wave 1 — Foundation & Schema | 6 | 1 |
| Wave 2 — Data Structs & Serialization | 4 | 1 |
| Wave 3 — SteamID Extraction & Auth Flow | 5 | 1 |
| Wave 4 — Login → Auto-Create → Enter World | 5 | 1 |
| Wave 5 — Lobby Map & Character Select UI | 6 | 1.5 |
| Wave 6 — Postgres Store & Hardening | 4 | 0.5 |
| **Sprint Total** | **30** | **~6 days** |
| **Done** | **30/30** | **All waves complete** |

---

## 📋 Sprint Waves

### Wave 1 — Foundation & Schema Design (Day 1, ~1d)
**Design the data store abstraction and implement the SQLite backend with schema migrations.**

- [x] Add SQLite amalgamation to `Source/Onset/ThirdParty/SQLite/` (`sqlite3.h` + `sqlite3.c`)
- [x] Update `Onset.Build.cs` — add SQLite include path and lib to the DS module
- [x] Create `IPlayerDataStore` abstract interface:
  - `LoadAccount(Platform, PlatformID)` → `FOnsetAccountData`
  - `LoadCharacter(Platform, PlatformID, SlotIndex)` → `FOnsetFullCharacterData`
  - `SaveCharacter(Platform, PlatformID, SlotIndex, Data)`
  - `SaveAll()` — flush on DS shutdown
- [x] Create `FSQLiteStore` implementing `IPlayerDataStore`:
  - Open/create DB file next to DS executable (configurable path)
  - Schema version table with migration runner
  - WAL mode for concurrent safety
  - Parametrized queries (portable SQL, same queries work in PgSQL)
- [x] Create `FPgSQLStore` stub implementing `IPlayerDataStore`:
  - Same queries, different connection (libpq)
  - Returns `NotImplemented` for demo; swap via config key
- [x] Create `UOnsetPlayerDataSubsystem` (world subsystem, DS only):
  - `Init()` reads config to select store implementation
  - `GetAccountData()` / `GetCharacterData()` / `SaveCharacterData()` pass through to store
  - `BeginDestroy()` calls `SaveAll()` on the store

### Wave 2 — Account & Character Data Structs (Day 2, ~1d)
**Define all USTRUCTs for network transfer between DS and client.**

- [x] Create `FOnsetCharacterSlotData` (BlueprintType):
  - `int32 SlotIndex`, `FString CharacterName`, `int32 Level`, `bool bOccupied`
- [x] Create `FOnsetAccountData` (BlueprintType):
  - `FString PlatformID`, `FString Platform`, `TArray<FOnsetCharacterSlotData> Slots` (up to 3)
- [x] Create `FOnsetFullCharacterData`:
  - Slot identity: `SlotIndex`, `CharacterName`
  - Progression: `Level`, `Experience`
  - Attributes: `MaxHealth`
  - World state: `FVector Position`, `float RotationYaw`
  - Extensible blobs: `InventoryJSON`, `EquipmentJSON`, `QuestsJSON` (empty until future systems)
- [x] UOnsetPlayerDataSubsystem serialization methods:
  - `AccountFromDB(sqlite3_stmt*)` → `FOnsetAccountData`
  - `CharacterFromDB(sqlite3_stmt*)` → `FOnsetFullCharacterData`
  - `DBFromCharacter(const FOnsetFullCharacterData&)` → bind to prepared INSERT/UPDATE

### Wave 3 — SteamID Extraction & Auth Integration (Day 3, ~1d)
**Resolve the numeric SteamID from the auth ticket and wire to persistence.**

- [x] Add `PlayerPlatformID` (FString) and `PlayerPlatform` (FString) to `AOnsetPlayerState`
- [x] Include Steamworks SDK headers in GameMode for `ISteamGameServer`
- [x] In `AOnsetGameModeBase::ValidateAuthTicket`:
  - After successful validation, call `SteamGameServer()->BeginAuthSession()` to get SteamID
  - Store SteamID as `FString` on PlayerState: `PlayerPlatformID = FString::Printf(TEXT("%llu"), SteamID)`
  - Set `PlayerPlatform = TEXT("Steam")`
- [x] After auth completes in PostLogin:
  - `UOnsetPlayerDataSubsystem::LoadAccount(PlatformID, Platform)`
  - If no account: auto-create (insert row, return empty slots)
  - Send `Client_AccountData(FOnsetAccountData)` to client
- [x] Store Steam SDK headers path in `Onset.Build.cs` (or reference existing ThirdParty)

### Wave 4 — Login → Auto-Create → Enter World (Day 4, ~1d)
**Implement the full RPC flow for account loading, character selection, and world entry.**

- [x] Add to `AOnsetPlayerController`:
  - `Client_AccountData(FOnsetAccountData)` — receive account overview
  - `Client_CharacterData(FOnsetFullCharacterData)` — receive full character data on select
  - `Server_SelectCharacter(int32 SlotIndex)` — client picks a slot
  - `Server_CreateCharacter(int32 SlotIndex, FString Name)` — client creates new character
  - `Server_SaveCharacter()` — manual save request
  - `Client_SaveComplete(bool bSuccess)` — confirm save
- [x] `Server_SelectCharacter` implementation:
  - Load character from `UOnsetPlayerDataSubsystem`
  - Store chosen slot on `AOnsetPlayerState`
  - Spawn / reposition `AOnsetPlayerCharacter`
  - Apply saved attributes (MaxHealth, position, rotation)
  - Send `Client_CharacterData` for client-side UI update
- [x] `Server_CreateCharacter` implementation:
  - Validate slot is empty and in range 0-2
  - Create default character record (level 1, default position, name)
  - Auto-select (same flow as Server_SelectCharacter)
- [x] Save-on-disconnect: `AOnsetPlayerController::EndPlay` / `Logout` triggers save
- [x] Periodic auto-save: `FTimerHandle` in `UOnsetPlayerDataSubsystem`, 5-min interval, saves all connected players
- [x] Save on level-up / death placeholder (hooks for future systems; save full state for now)

### Wave 5 — Lobby Map & Character Select UI (Days 5-6, ~1.5d)
**Build the character select experience connecting auth to gameplay.**

- [x] Create lobby map (`/Game/Maps/MainMenu`) — serves as both main menu and character select hub
- [x] Set as default map for the DS (via `ServerDefaultMap` in config)
- [x] No NPCs, no combat — just the character select HUD
- [x] Implement `AOnsetLobbyHUD` (canvas-based, replaces UMG widget approach):
  - 3 slot panels showing name, level, status (empty/occupied)
  - Create/Enter button for each slot
  - Hint text ("Select a character (1,2,3)") when no slot selected
  - Keyboard input: 1/2/3 select, Enter confirm
- [x] Wire slot creation: empty slot → select → Enter → `Server_CreateCharacter` → slot fills
- [x] Wire slot selection: select occupied slot → highlight → Enter → `Server_SelectCharacter`
- [x] Wire "Enter World": `Server_SelectCharacter(Index)` → server responds → **ServerTravel** to DemoLevel
- [x] DS startup flow:
  - DS launches on MainMenu
  - Clients connect, auth, receive account data via `Client_AccountData`, see character select
  - When client presses Enter on a slot, server **ServerTravel**s to DemoLevel with all connected players
  - (Future: seamless travel when we add zone servers)
- [x] `AOnsetMenuHUD` (canvas-based main menu, replaces UMG `MainMenuWidget`)
- [x] `AOnsetMenuGameMode` with HUDClass set to `AOnsetMenuHUD`
- [x] Packaged build verified: main menu → server connect → auth → character select → zone travel → player spawns with visible mesh

### Wave 6 — Postgres Store & Production Hardening (Day 6, ~0.5d)
**Add the production store implementation and harden the persistence layer.**

- [x] `FPgSQLStore` implementation:
  - Connect via `PQconnectdb()` with configurable connection string
  - Same schema, same parametrized queries as SQLite
  - `Config/DefaultEngine.ini` key: `Type=Postgres` / `Type=SQLite`
  - libpq linked from `ThirdParty/PostgreSQL/` (EDB binary distribution)
- [x] WAL mode enabled for SQLite on all connections
- [x] Migration system: `_schema_version` table, version array in code, sequential migrations
- [x] Crash recovery: transaction wrapping all writes, rollback on failure
- [x] Config key for DB path / connection string (`[Onset.DataStore]`)
- [x] Verify clean shutdown saves all pending writes

---

## ⚠ Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| DB corruption on DS crash | Data loss for all connected players | Low | WAL mode + transaction wrapping + periodic checkpoints |
| Schema migration incompatibility | DS fails to start after update | Low | Versioned migrations with forward-only ordering; test migration from every prior version |
| SteamID extraction fails | Player cannot authenticate | Medium | BeginAuthSession returns SteamID directly; fallback to ticket string hash (not unique per-player, but prevents hard crash) |
| Save RPC during combat | Stale/partial save (position after death) | Medium | Save queue queued and debounced; death state saved separately |
| Character select UI touch-unfriendly | Touch players can't create/select | Low | WBP designed for touch first (large buttons, minimal text input via virtual keyboard) |
| PgSQL connection string config | DS doesn't start if misconfigured | Medium | Validate connection at startup; fall back to SQLite with warning log |

---

## 📐 Design Decisions

**IPlayerDataStore abstraction:** The DS never knows whether it's talking to SQLite or PostgreSQL. Config key selects implementation at startup. Ensures zero code changes when swapping backends.

**SQLite first, PgSQL stubbed:** SQLite ships with the DS with zero external dependencies. PgSQL implementation exists but requires `libpq.dll` + connection string — documented but not required for the demo.

**FString for platform ID everywhere:** Cross-platform consistency. Steam's `uint64` becomes `FString::Printf(TEXT("%llu"))`. Xbox/PS/Switch IDs are already strings.

**Character select on a lobby map, not overlay:** Separates auth flow from game world. DS travels to game map once all players are ready. Naturally supports future zone server architecture.

**Extensible JSON blobs for inventory/quests/equipment:** These systems don't exist yet. JSON blobs in the DB keep the schema stable while those systems evolve independently later.

**Three JSON blobs, not one:** `inventory_json`, `equipment_json`, `quests_json`. Separate concerns even in storage — allows independent versioning and migration per system.

**Auto-save on disconnect + 5-min timer, not write-on-demand:** Avoids save RPC spam. The timer catches progress between discrete events. Disconnect save ensures no lost session.

**Module extraction (OnsetDataStore):** Data store code moved from `Onset` module to dedicated `Source/OnsetDataStore/` module. Factory pattern (`CreateDataStore()`) decouples subsystem from concrete stores. Client build links only interface + types — no `SQLiteCore` dependency. `ONSETDATASTORE_CLIENT_ONLY` define guards store implementations.

**HUD over UMG for menus:** Pure C++ `UUserWidget` subclasses do not render in UE5.8 packaged `-game` builds. Replaced with canvas-based `AOnsetLobbyHUD` and `AOnsetMenuHUD` (HPainted). Both work in PIE, `-game`, and packaged builds.

---

## 🔗 Dependencies

| System | Dependency Type |
|--------|-----------------|
| A5.4 Steam Auth | Required — ticket validation must work before SteamID extraction |
| Steamworks SDK headers | Required for `ISteamGameServer::BeginAuthSession()` |
| SQLite amalgamation | Required — bundled in ThirdParty/ |
| libpq (optional) | Required only for PgSQL store — not bundled, documented |
| WBP widgets | Required for character select — existing project UI pattern |
