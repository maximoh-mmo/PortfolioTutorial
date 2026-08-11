# 🎬 **Episode 43 — Full Login → Character Select → Enter World**

## **Goal**
End-to-end verification: auth → account load → character select → spawn from saved state → auto-save timer → save-on-logout/disconnect. Production hardening with WAL mode, crash recovery, and migration tests.

---

## **What Was Built**
Integration test and hardening pass. Full flow from login-server launch through Steam auth, character select, tokenized travel to the game server, gameplay, disconnect, and reconnect — verifying every save trigger and edge case. Production hardening: WAL mode validation under crash simulation, schema migrations, save-on-logout + save-on-disconnect coordination.

---

## **Project Snapshot**
Full Unreal project at end of Episode 43 (end of Sprint A5b).

### **Key Files Added/Modified**
| File | Description |
|---|---|
| `Source/Onset/Private/Subsystem/OnsetPlayerDataSubsystem.cpp` | Auto-save timer (5 min, `StartAutoSaveTimer` → `SaveAll`), `Deinitialize` flush |
| `Source/Onset/Private/Player/OnsetPlayerController.cpp` | `Server_SaveCharacter`/`SaveCurrentCharacter`, save on logout + `EndPlay` |
| `Source/Onset/Private/Player/OnsetPlayerCharacter.cpp` | `RespawnPlayer` (full-heal + home transform after death) |
| `Source/OnsetDataStore/Private/FSQLiteStore.cpp` | WAL mode, `synchronous=NORMAL`, schema migrations v1→v3 |
| `Source/OnsetDataStore/Private/DataStoreFactory.cpp` | Store selection (SQLite / PgSQL / HTTP) |
| `Config/DefaultEngine.ini` | `[Onset.DataStore]` config example (PgSQL commented) |

---

## **How to Test — Full Checklist**

### **Login Server Launch**
- [ ] Login server starts on `MainMenu` (`GameDefaultMap`)
- [ ] Log: `FSQLiteStore: opened <path> (existing=0)` on first run
- [ ] Log: `FSQLiteStore: migration N applied` for each of the 3 migrations
- [ ] Log: `UOnsetPlayerDataSubsystem: initialized with SQLite (success=1)`
- [ ] DB file exists at `Project/Saved/OnsetPlayerData.db`

### **Client Connect + Auth**
- [ ] Client connects → `RequestSteamAuth()` → ticket sent
- [ ] Server `ValidateAuthTicket` → `BeginAuthSession` → SteamID extracted
- [ ] `PlayerState.PlayerPlatformID` = `"76561197960265728"`
- [ ] `PlayerState.PlayerPlatform` = `"Steam"`
- [ ] `Client_ClearAuthTimeout` received

### **Account Load**
- [ ] `PostLogin` → `LoadAccount(Steam, SteamID)`
- [ ] First login: account auto-created (INSERT into `accounts`)
- [ ] `Client_AccountData` sent with the account's existing slots (empty array for new accounts)
- [ ] Widget pads to 3 "Empty Slot" panels (`FMath::Max(3, Slots.Num())`)

### **Character Creation**
- [ ] Click slot 0 → "Create" → enter name, pick class + appearance → `Server_CreateCharacter(0, "Hero", Class, Preset)`
- [ ] DB: INSERT into `characters` with level=1, pos=(0,0,150)
- [ ] `Client_AccountData` refresh → slot 0 shows "Hero, Level 1"

### **Character Select + World Entry**
- [ ] Click slot 0 → highlight → "Enter World" enabled
- [ ] Press Enter World → `Server_SelectCharacter(0)`
- [ ] `LoadCharacter(Steam, SteamID, 0)` → full data returned
- [ ] Login server generates session token → `Client_TravelToGameServer(IP, Port, Token)`
- [ ] Client travels with `?Token=` in URL → arrives at `DemoLevel`
- [ ] Pawn spawns at saved position, `OnRep_Pawn` hides the loading screen

### **Gameplay + Save Triggers**
- [ ] Move pawn to (500, 300, 200)
- [ ] Wait 5 min → auto-save timer fires → DB `saved_at` updated
- [ ] Check DB: `pos_x=500, pos_y=300`
- [ ] Trigger death → respawn at home transform with full health (no save on death)
- [ ] Disconnect (Alt+F4) → `EndPlay` → `SaveCurrentCharacter` → DB updated
- [ ] Logout (return to menu) → save fires before travel
- [ ] Reconnect → select character → spawns at (500, 300, 200)

### **Multi-Client**
- [ ] 3 clients connect simultaneously
- [ ] All see own character select (independent accounts)
- [ ] All enter world → 3 pawns in DemoLevel
- [ ] All move, disconnect, reconnect → correct positions restored

---

## **Production Hardening Verification**

### **SQLite WAL Mode + Crash Recovery**
```bash
# Simulate crash mid-save
taskkill /F /IM Onset.exe
# Restart DS
# Log should show FSQLiteStore: opened ... (existing=1)
# WAL mode + synchronous=NORMAL guarantee the last committed transaction survives
```
WAL and `synchronous=NORMAL` mean an abrupt kill loses at most the in-flight write; the last auto-save checkpoint survives (at most the auto-save interval of progress lost).

### **Schema Migrations**
```sql
-- _schema_version drives ordered migration (v1 → v3), each run in a transaction.
-- v1: accounts + characters
-- v2: current_zone column
-- v3: character_class + appearance_json columns
```

### **PgSQL Stub**
```ini
# Config (commented by default)
; Type=Postgres
; ConnectionString=host=localhost dbname=onset user=onset password=...
```
- [ ] Compiles on Windows with libpq
- [ ] Unavailable backend → logs fallback warning, uses SQLite

---

## **Code Snippets**

```cpp
// OnsetPlayerDataSubsystem.cpp — Auto-save timer
void UOnsetPlayerDataSubsystem::StartAutoSaveTimer()
{
    float Interval = 300.0f; // default 5 minutes
    GConfig->GetFloat(TEXT("Onset.DataStore"), TEXT("AutoSaveInterval"), Interval, GEngineIni);
    GetWorld()->GetTimerManager().SetTimer(AutoSaveTimerHandle,
        this, &UOnsetPlayerDataSubsystem::SaveAll, Interval, true);
}

// OnsetPlayerDataSubsystem.cpp — Deinitialize flushes
void UOnsetPlayerDataSubsystem::Deinitialize()
{
    StopAutoSaveTimer();
    SaveAll();   // Store->SaveAll()
    Store.Reset();
}

// OnsetPlayerController.cpp — Save (no debounce, no bForce param)
void AOnsetPlayerController::Server_SaveCharacter_Implementation()
{
    // read SelectedCharacterSlot + pawn, build CharData,
    // SaveCharacterPreservingIdentity(...) → Client_SaveComplete(bSuccess)
}

// OnsetPlayerController.cpp — Save on logout / disconnect
void AOnsetPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Reason == EEndPlayReason::Destroyed || Reason == EEndPlayReason::RemovedFromWorld)
        SaveCurrentCharacter(GetPawn());
    Super::EndPlay(Reason);
}

// FSQLiteStore.cpp — Initialize
bool FSQLiteStore::Initialize(const FString& InConnectionString)
{
    DBPath = InConnectionString.IsEmpty()
        ? FPaths::ProjectSavedDir() / TEXT("OnsetPlayerData.db")
        : NormalizePath(InConnectionString);
    if (sqlite3_open(TCHAR_TO_UTF8(*DBPath), &DB) != SQLITE_OK)
        return false; // falls back to :memory: in PIE
    Exec("PRAGMA journal_mode=WAL;");
    Exec("PRAGMA synchronous=NORMAL;");
    Exec("PRAGMA foreign_keys=ON;");
    EnsureSchema();
}
```

---

## **Common Pitfalls**
- Auto-save timer not cleared on DS shutdown → access violation (cleared in `StopAutoSaveTimer` inside `Deinitialize`)
- Saving in `EndPlay` after the pawn is gone → `SaveCurrentCharacter(GetPawn())` may no-op; capture/save earlier on logout while pawn + PlayerState are still valid
- Migration runner doesn't run in a transaction → partial migration leaves DB in bad state
- PgSQL link fails on non-Windows → guard with `PLATFORM_WINDOWS` or dynamic load

---

## **Dependencies**
- Episodes 40-42 complete

---

## **Episode Checklist**
- [x] Full flow: login server launch → auth → select → world → move → save → reconnect
- [x] 3 clients independent accounts verified
- [x] `taskkill` DS mid-write → DB intact (WAL + `synchronous=NORMAL`)
- [x] Migrations v1→v3 run on fresh DB
- [x] PgSQL stub compiles on Windows
- [x] Auto-save log appears every 5 min
- [x] Logout + disconnect both trigger save
- [x] Reconnect restores exact position
- [x] No memory leaks in `UOnsetPlayerDataSubsystem` after 1hr session
- [x] Snapshot clean for public repo

---

## **Next Steps (Post-Sprint A5b)**
- **Episode 44** — UI & Feedback (health bars, cooldowns, virtual joystick, gamepad cursor)
- **Episode 45** — Final Gameplay Loop (waves, respawn, combat flow)
- **Episode 46** — Performance Optimization
- **Episode 47** — Final Showcase
