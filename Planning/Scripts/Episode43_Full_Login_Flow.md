# 🎬 **Episode 43 — Full Login → Character Select → Enter World**

## **Episode Goal**
End-to-end verification: auth → account load → character select → spawn from saved state → auto-save timer → save-on-disconnect. Production hardening with WAL mode, crash recovery, and migration tests.

---

## **Context & Dependencies**
- Episodes 40-42 complete (DB, SteamID, Lobby/Select)
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

#### **DS Launch**
- [ ] DS starts on `LobbyMap`
- [ ] Log: `SQLite store initialized at ...`
- [ ] Log: `WAL mode enabled`
- [ ] Log: `Schema version: 1`
- [ ] DB file exists at configured path

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
- [ ] `Client_AccountData` sent with 3 empty slots
- [ ] Widget shows 3 "Empty Slot" panels

#### **Character Creation**
- [ ] Click slot 0 → enter "Hero" → `Server_CreateCharacter(0, "Hero")`
- [ ] DB: INSERT into `characters` with level=1, pos=(0,0,200)
- [ ] `Client_AccountData` refresh → slot 0 shows "Hero, Level 1"

#### **Character Select + World Entry**
- [ ] Click slot 0 → highlight → "Enter World" enabled
- [ ] Press Enter World → `Server_SelectCharacter(0)`
- [ ] `DataSub->LoadCharacter(Steam, SteamID, 0)` → full data returned
- [ ] Pawn spawned at saved position (0,0,200)
- [ ] `Client_CharacterData` received
- [ ] `GameMode->RequestTravelToGameMap()` → `ServerTravel(/Game/Maps/DemoLevel)`
- [ ] Client arrives at `DemoLevel`, pawn at (0,0,200)

#### **Gameplay + Save Triggers**
- [ ] Move pawn to (500, 300, 200)
- [ ] Wait 5 min → auto-save fires → DB `saved_at` updated
- [ ] Check DB: `pos_x=500, pos_y=300`
- [ ] Trigger death → `OnDeath` → save fires → DB updated
- [ ] Disconnect (Alt+F4) → `EndPlay` → `Server_SaveCharacter` → DB updated
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
sqlite3_exec(DBHandle, "PRAGMA journal_mode=WAL", nullptr, nullptr, nullptr);
sqlite3_exec(DBHandle, "PRAGMA synchronous=NORMAL", nullptr, nullptr, nullptr);
```
- **Test:** `taskkill /F /IM Onset.exe` mid-save → restart DS → `PRAGMA integrity_check` → OK

#### **Crash Recovery**
- WAL mode ensures atomic commits
- On startup: `sqlite3_wal_checkpoint_v2(DBHandle, SQLITE_CHECKPOINT_TRUNCATE)`
- `integrity_check` pragma on first connection after unclean shutdown

#### **Migration CI**
```yaml
# .github/workflows/migrations.yml
jobs:
  test-migrations:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4
      - name: Build DS
        run: ./BuildDS.bat
      - name: Test migration v0->v1
        run: |
          # Fresh DB
          del OnsetDB\playerdata.db
          # Run DS (runs migration)
          start /wait Onset.exe ... -server -log -NoLoadingScreen -unattended
          # Verify schema
          sqlite3 OnsetDB\playerdata.db ".schema"
      - name: Test migration v1->v2 (future)
        run: |
          # Apply v2 migration SQL manually, verify
```

#### **PgSQL Stub Compile Verification**
- Add `libpq.lib` to `Onest.Build.cs` (wrapped in `#if WITH_PGSQL`)
- `FPgSQLStore` compiles and links
- `DataStore=Postgres` config doesn't crash (logs warning, falls back to SQLite)

### **3. Save Coordination Logic**

**Auto-save timer (5 min):**
```cpp
// UOnsetPlayerDataSubsystem.cpp
void UOnsetPlayerDataSubsystem::Initialize(...)
{
    // ...
    GetWorld()->GetTimerManager().SetTimer(AutoSaveTimer, this, &UOnsetPlayerDataSubsystem::AutoSaveAll, 300.0f, true);
}

void UOnsetPlayerDataSubsystem::AutoSaveAll()
{
    for (APlayerController* PC : TActorRange<APlayerController>(GetWorld()))
    {
        if (AOnsetPlayerController* OnsetPC = Cast<AOnsetPlayerController>(PC))
        {
            OnsetPC->Server_SaveCharacter(); // debounced internally
        }
    }
}
```

**Save debounce (per-player):**
```cpp
// AOnsetPlayerController.cpp
void AOnsetPlayerController::Server_SaveCharacter_Implementation()
{
    if (GetWorld()->GetTimeSeconds() - LastSaveTime < 30.0f) return; // 30s debounce
    LastSaveTime = GetWorld()->GetTimeSeconds();
    // ... actual save ...
}
```

**Death save (immediate, no debounce):**
```cpp
// AOnsetPlayerCharacter.cpp
void AOnsetPlayerCharacter::OnDeath(AActor* KillingActor)
{
    Super::OnDeath(KillingActor);
    if (AOnsetPlayerController* PC = Cast<AOnsetPlayerController>(GetController()))
    {
        PC->Server_SaveCharacter(true); // bForce = true bypasses debounce
    }
}
```

---

## **How to Test**
Run the full checklist above. Key verification:
1. DS survives `taskkill /F` mid-session → DB intact on restart
2. Migration from empty DB → v1 schema works
3. 3 clients → independent accounts → correct positions on reconnect
4. Auto-save timer fires (log: `Auto-save: 3 players saved`)
5. PgSQL config → compiles, logs fallback warning

---

## **Code Snippets**

```cpp
// FSQLiteStore.cpp — Crash recovery
bool FSQLiteStore::Initialize(const FString& Path)
{
    if (sqlite3_open(TCHAR_TO_UTF8(*Path), &DBHandle) != SQLITE_OK) return false;
    sqlite3_exec(DBHandle, "PRAGMA journal_mode=WAL", nullptr, nullptr, nullptr);
    sqlite3_exec(DBHandle, "PRAGMA integrity_check", nullptr, nullptr, nullptr);
    RunMigrations();
    return true;
}

// UOnsetPlayerDataSubsystem.cpp — AutoSaveAll
void UOnsetPlayerDataSubsystem::AutoSaveAll()
{
    UE_LOG(LogOnsetData, Log, TEXT("Auto-save: %d players"), ConnectedPlayers.Num());
    for (AOnsetPlayerController* PC : ConnectedPlayers)
    {
        PC->Server_SaveCharacter();
    }
}

// AOnsetPlayerController.cpp — Debounced save
void AOnsetPlayerController::Server_SaveCharacter_Implementation(bool bForce)
{
    if (!bForce && GetWorld()->GetTimeSeconds() - LastSaveTime < 30.0f) return;
    LastSaveTime = GetWorld()->GetTimeSeconds();
    // ... save logic ...
    Client_SaveComplete(bSuccess);
}
```

---

## **Common Pitfalls**
- Auto-save timer not cleared on DS shutdown → access violation
- `ServerTravel` loses PlayerController reference during save → capture pawn data before travel
- Migration runner doesn't run in transaction → partial migration leaves DB in bad state
- PgSQL link fails on non-Windows → guard with `PLATFORM_WINDOWS` or use dynamic load

---

## **Dependencies**
- Episodes 40-42 complete

---

## **Episode Checklist**
- [ ] Full flow: DS launch → auth → select → world → move → save → reconnect
- [ ] 3 clients independent accounts verified
- [ ] `taskkill` DS mid-save → DB `integrity_check` OK
- [ ] Migration v0→v1 runs on fresh DB in CI
- [ ] PgSQL stub compiles on Windows
- [ ] Auto-save log appears every 5 min
- [ ] Death triggers immediate save (bForce=true)
- [ ] Disconnect triggers save
- [ ] Reconnect restores exact position
- [ ] No memory leaks in `UOnsetPlayerDataSubsystem` after 1hr session

---

## **Next Steps (Post-Sprint)**
- Episode 44: UI & Feedback (health bars, cooldowns, virtual joystick, gamepad cursor)
- Episode 45: Final Gameplay Loop (waves, respawn, combat flow)
- Episode 46: Performance Optimization
- Episode 47: Final Showcase