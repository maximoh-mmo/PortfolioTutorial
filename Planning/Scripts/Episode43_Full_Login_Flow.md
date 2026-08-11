# 🎬 **Episode 43 — Full Login → Character Select → Enter World**

## **Episode Goal**
End-to-end verification: auth → account load → character select → spawn from saved state → auto-save timer → save-on-disconnect. Production hardening with WAL mode, crash recovery, and migration tests.

---

## **Context & Dependencies**
- Episodes 40-42 complete (DB, SteamID, Character Select)
- All persistence RPCs working
- Dedicated server tested with 3 clients

---

## **High‑Level Summary**
This episode is the integration test and hardening pass. We run the full flow from DS launch through Steam auth, character select, world entry, gameplay, disconnect, and reconnect — verifying every save trigger and edge case. We also add production hardening: WAL mode validation, crash recovery, schema migration CI, and PgSQL stub verification.

---

## **Key Concepts Introduced**
- End-to-end integration testing checklist
- SQLite WAL mode verification under crash simulation
- Migration runner CI integration
- PgSQL stub compile verification
- Auto-save timer + save-on-disconnect + death-save coordination

---

## **Technical Breakdown**

### **1. Full Flow Verification Checklist**

#### **Login Server Launch**
- [ ] Login server starts on `MainMenu` (`GameDefaultMap`)
- [ ] Log: `FSQLiteStore: opened <path> (existing=0)` on first run
- [ ] Log: `FSQLiteStore: migration N applied` for each of the 3 migrations
- [ ] Log: `UOnsetPlayerDataSubsystem: initialized with SQLite (success=1)`
- [ ] DB file exists at `Project/Saved/OnsetPlayerData.db`

#### **Client Connect + Auth**
- [ ] Client connects → `OnsetPlayerController::BeginPlay` → `RequestSteamAuth()`
- [ ] Auth ticket sent via `Server_SendAuthTicket`
- [ ] Server `ValidateAuthTicket` → `BeginAuthSession` → SteamID extracted
- [ ] `PlayerState.PlayerPlatformID` = `"76561197960265728"`
- [ ] `PlayerState.PlayerPlatform` = `"Steam"`
- [ ] `Client_ClearAuthTimeout` received

#### **Account Load**
- [ ] `GameMode::PostLogin` → `DataSub->LoadAccount(Steam, SteamID)`
- [ ] First login: account auto-created (INSERT into `accounts`)
- [ ] `Client_AccountData` sent with the account's existing slots (empty array for new accounts)
- [ ] Widget pads to 3 "Empty Slot" panels (`FMath::Max(3, Slots.Num())`)

#### **Character Creation**
- [ ] Click slot 0 → enter "Hero" + class + appearance → `Server_CreateCharacter(0, "Hero", Class, Preset)`
- [ ] DB: INSERT into `characters` with level=1, pos=(0,0,150)
- [ ] `Client_AccountData` refresh → slot 0 shows "Hero, Level 1"

#### **Character Select + World Entry**
- [ ] Click slot 0 → highlight → "Enter World" enabled
- [ ] Press Enter World → `Server_SelectCharacter(0)`
- [ ] `DataSub->LoadCharacter(Steam, SteamID, 0)` → full data returned
- [ ] Login server generates session token → `Client_TravelToGameServer(IP, Port, Token)`
- [ ] Client travels with `?Token=` in URL → arrives at `DemoLevel`
- [ ] Pawn spawns at saved position, `OnRep_Pawn` hides the loading screen

#### **Gameplay + Save Triggers**
- [ ] Move pawn to (500, 300, 200)
- [ ] Wait 5 min → auto-save timer fires → DB `saved_at` updated
- [ ] Check DB: `saved_position_x=500, saved_position_y=300`
- [ ] Trigger death → respawn at home transform with full health (no save on death)
- [ ] Disconnect (Alt+F4) → `EndPlay` → `SaveCurrentCharacter` → DB updated
- [ ] Logout (return to menu) → save fires before travel
- [ ] Reconnect → select character → spawns at (500, 300, 200)

#### **Multi-Client**
- [ ] 3 clients connect simultaneously
- [ ] All see own character select (independent accounts)
- [ ] All enter world → 3 pawns in DemoLevel
- [ ] All move, disconnect, reconnect → correct positions restored

### **2. Production Hardening**

#### **SQLite WAL Mode Verification**
```cpp
// In FSQLiteStore::Initialize()
sqlite3_exec(DB, "PRAGMA journal_mode=WAL", nullptr, nullptr, nullptr);
sqlite3_exec(DB, "PRAGMA synchronous=NORMAL", nullptr, nullptr, nullptr);
sqlite3_exec(DB, "PRAGMA foreign_keys=ON", nullptr, nullptr, nullptr);
```
- **Test:** `taskkill /F /IM Onset.exe` mid-write → restart DS → WAL + `synchronous=NORMAL` guarantee the last committed transaction survives (at most the auto-save interval of progress lost)

#### **Crash Recovery**
- WAL mode ensures atomic commits; no explicit `integrity_check` pragma is run by the store

#### **Schema Migrations**
```sql
-- _schema_version drives ordered migration (1 → 3), each in a transaction.
-- v1: accounts + characters
-- v2: current_zone column
-- v3: character_class + appearance_json columns
-- Manual verification: run DS on a fresh DB, confirm all three "migration N applied" logs
```

#### **PgSQL Stub Compile Verification**
- Add `libpq` to `OnsetDataStore.Build.cs` (wrapped in `#if WITH_PGSQL`)
- `FPgSQLStore` compiles and links
- `Type=Postgres` config doesn't crash (logs warning, falls back to SQLite)

### **3. Save Coordination Logic**

**Auto-save timer (5 min):**
```cpp
// UOnsetPlayerDataSubsystem.cpp
void UOnsetPlayerDataSubsystem::StartAutoSaveTimer()
{
    float Interval = 300.0f; // default 5 minutes, configurable via [Onset.DataStore] AutoSaveInterval
    GetWorld()->GetTimerManager().SetTimer(AutoSaveTimerHandle,
        this, &UOnsetPlayerDataSubsystem::SaveAll, Interval, true);
}

void UOnsetPlayerDataSubsystem::SaveAll()
{
    if (Store) Store->SaveAll();
}
```

**Save on logout / disconnect:**
```cpp
// AOnsetPlayerController.cpp
void AOnsetPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Reason == EEndPlayReason::Destroyed || Reason == EEndPlayReason::RemovedFromWorld)
        SaveCurrentCharacter(GetPawn());
    Super::EndPlay(Reason);
}
// Logout (return to menu) saves first via SaveCurrentCharacter(LeavingPawn), then travels
```

**Death respawn (no save on death):**
```cpp
// AOnsetPlayerCharacter.cpp
void AOnsetPlayerCharacter::OnDeath(AActor* KillingActor)
{
    Super::OnDeath(KillingActor);
    DisableInput(nullptr);
    GetWorldTimerManager().SetTimerForNextTick(this, &AOnsetPlayerCharacter::RespawnPlayer);
}
// RespawnPlayer: full-heal + restore HomeTransform — position is only saved by the
// auto-save timer, logout, or disconnect paths (Server_SaveCharacter has no bForce param)
```

---

## **How to Test**
Run the full checklist above. Key verification:
1. Login server survives `taskkill /F` mid-session → DB intact on restart (WAL)
2. Migration from empty DB → v1→v3 schema works (three "migration N applied" logs)
3. 3 clients → independent accounts → correct positions on reconnect
4. Auto-save timer fires every 5 min (log: `Auto-save timer started (interval=300.0s)`)
5. PgSQL config → compiles, logs fallback warning

---

## **Code Snippets**

```cpp
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

// UOnsetPlayerDataSubsystem.cpp — SaveAll
void UOnsetPlayerDataSubsystem::SaveAll()
{
    if (Store) Store->SaveAll();
}

// AOnsetPlayerController.cpp — Save (no debounce, no bForce param)
void AOnsetPlayerController::Server_SaveCharacter_Implementation()
{
    // read SelectedCharacterSlot + pawn, build CharData,
    // SaveCharacterPreservingIdentity(...) → Client_SaveComplete(bSuccess)
}
```

---

## **Common Pitfalls**
- Auto-save timer not cleared on DS shutdown → access violation (cleared in `StopAutoSaveTimer` inside `Deinitialize`)
- Saving in `EndPlay` after the pawn is gone → `SaveCurrentCharacter(GetPawn())` may no-op; save earlier on logout while pawn + PlayerState are still valid
- Migration runner doesn't run in a transaction → partial migration leaves DB in bad state
- PgSQL link fails on non-Windows → guard with `PLATFORM_WINDOWS` or use dynamic load

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

---

## **Next Steps (Post-Sprint)**

The episode numbering has been revised to include a new Auth Extraction & Login Server phase. The post-persistence episodes are now:

- Episode 44: Auth Subsystem Extraction
- Episode 45: Session Token System
- Episode 46: Login Server Target
- Episode 47: Client & Game Server Token Flow
- Episode 48: UI & Feedback (health bars, cooldowns, virtual joystick, gamepad cursor)
- Episode 49: Final Gameplay Loop (waves, respawn, combat flow)
- Episode 50: Performance Optimization
- Episode 51: Final Showcase