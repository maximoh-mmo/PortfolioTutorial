# 🎬 **Episode 43 — Full Login → Character Select → Enter World**

## **Goal**
End-to-end verification: auth → account load → character select → spawn from saved state → auto-save timer → save-on-disconnect. Production hardening with WAL mode, crash recovery, and migration tests.

---

## **What Was Built**
Integration test and hardening pass. Full flow from DS launch through Steam auth, character select, world entry, gameplay, disconnect, and reconnect — verifying every save trigger and edge case. Production hardening: WAL mode validation under crash simulation, migration CI, PgSQL stub verification, auto-save + death-save + disconnect-save coordination.

---

## **Project Snapshot**
Full Unreal project at end of Episode 43 (end of Sprint A5b).

### **Key Files Added/Modified**
| File | Description |
|---|---|
| `Source/Onset/Private/Multiplayer/OnsetPlayerDataSubsystem.cpp` | Auto-save timer (5 min), `AutoSaveAll()` |
| `Source/Onset/Private/Player/OnsetPlayerController.cpp` | Debounced `Server_SaveCharacter`, death save bypass |
| `Source/Onset/Private/Player/OnsetPlayerCharacter.cpp` | Death triggers immediate save (`bForce=true`) |
| `Source/Onest/Private/Server/SQLiteStore.cpp` | Crash recovery: `integrity_check`, WAL checkpoint |
| `.github/workflows/migrations.yml` | CI test for schema migrations |
| `Config/DefaultEngine.ini` | PgSQL config example (commented) |

---

## **How to Test — Full Checklist**

### **DS Launch**
- [ ] DS starts on `LobbyMap`
- [ ] Log: `SQLite store initialized at ...`
- [ ] Log: `WAL mode enabled`
- [ ] Log: `Schema version: 1`
- [ ] DB file exists at configured path

### **Client Connect + Auth**
- [ ] Client connects → `RequestSteamAuth()` → ticket sent
- [ ] Server `ValidateAuthTicket` → `BeginAuthSession` → SteamID extracted
- [ ] `PlayerState.PlayerPlatformID` = `"76561197960265728"`
- [ ] `PlayerState.PlayerPlatform` = `"Steam"`
- [ ] `Client_ClearAuthTimeout` received

### **Account Load**
- [ ] `PostLogin` → `LoadAccount(Steam, SteamID)`
- [ ] First login: account auto-created (INSERT into `accounts`)
- [ ] `Client_AccountData` sent with 3 empty slots
- [ ] Widget shows 3 "Empty Slot" panels

### **Character Creation**
- [ ] Click slot 0 → enter "Hero" → `Server_CreateCharacter(0, "Hero")`
- [ ] DB: INSERT into `characters` with level=1, pos=(0,0,200)
- [ ] `Client_AccountData` refresh → slot 0 shows "Hero, Level 1"

### **Character Select + World Entry**
- [ ] Click slot 0 → highlight → "Enter World" enabled
- [ ] Press Enter World → `Server_SelectCharacter(0)`
- [ ] `LoadCharacter(Steam, SteamID, 0)` → full data returned
- [ ] Pawn spawned at saved position (0,0,200)
- [ ] `Client_CharacterData` received
- [ ] `ServerTravel(/Game/Maps/DemoLevel)`
- [ ] Client arrives at `DemoLevel`, pawn at (0,0,200)

### **Gameplay + Save Triggers**
- [ ] Move pawn to (500, 300, 200)
- [ ] Wait 5 min → auto-save fires → DB `saved_at` updated
- [ ] Check DB: `pos_x=500, pos_y=300`
- [ ] Trigger death → `OnDeath` → save fires → DB updated
- [ ] Disconnect (Alt+F4) → `EndPlay` → `Server_SaveCharacter` → DB updated
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
# Log should show:
# SQLite integrity_check: ok
# WAL checkpoint completed
```

### **Migration CI**
```yaml
# .github/workflows/migrations.yml
# Fresh DB → run DS → schema v1 applied → verify tables exist
```

### **PgSQL Stub**
```ini
# Config (commented by default)
; DataStore=Postgres
; PgSQLConnectionString=host=localhost dbname=onset user=onset password=...
```
- [ ] Compiles on Windows with libpq
- [ ] `DataStore=Postgres` → logs fallback warning, uses SQLite

---

## **Code Snippets**

```cpp
// OnsetPlayerDataSubsystem.cpp — AutoSaveAll
void UOnsetPlayerDataSubsystem::Initialize(...)
{
    GetWorld()->GetTimerManager().SetTimer(AutoSaveTimer, this, &UOnsetPlayerDataSubsystem::AutoSaveAll, 300.0f, true);
}

void UOnsetPlayerDataSubsystem::AutoSaveAll()
{
    UE_LOG(LogOnsetData, Log, TEXT("Auto-save: %d players"), ConnectedPlayers.Num());
    for (AOnsetPlayerController* PC : ConnectedPlayers)
    {
        PC->Server_SaveCharacter();
    }
}

// OnsetPlayerController.cpp — Debounced save
void AOnsetPlayerController::Server_SaveCharacter_Implementation(bool bForce)
{
    if (!bForce && GetWorld()->GetTimeSeconds() - LastSaveTime < 30.0f) return;
    LastSaveTime = GetWorld()->GetTimeSeconds();
    // ... save logic ...
    Client_SaveComplete(bSuccess);
}

// OnsetPlayerCharacter.cpp — Death save (immediate)
void AOnsetPlayerCharacter::OnDeath(AActor* KillingActor)
{
    Super::OnDeath(KillingActor);
    if (AOnsetPlayerController* PC = Cast<AOnsetPlayerController>(GetController()))
    {
        PC->Server_SaveCharacter(true); // bForce=true bypasses debounce
    }
}

// SQLiteStore.cpp — Crash recovery
bool FSQLiteStore::Initialize(const FString& Path)
{
    if (sqlite3_open(TCHAR_TO_UTF8(*Path), &DBHandle) != SQLITE_OK) return false;
    sqlite3_exec(DBHandle, "PRAGMA journal_mode=WAL", nullptr, nullptr, nullptr);
    sqlite3_exec(DBHandle, "PRAGMA integrity_check", nullptr, nullptr, nullptr);
    RunMigrations();
    return true;
}
```

---

## **Common Pitfalls**
- Auto-save timer not cleared on DS shutdown → access violation
- `ServerTravel` loses PlayerController reference during save → capture pawn data before travel
- Migration runner doesn't run in transaction → partial migration leaves DB in bad state
- PgSQL link fails on non-Windows → guard with `PLATFORM_WINDOWS` or dynamic load

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
- [ ] Death triggers immediate save (`bForce=true`)
- [ ] Disconnect triggers save
- [ ] Reconnect restores exact position
- [ ] No memory leaks in `UOnsetPlayerDataSubsystem` after 1hr session
- [ ] Snapshot clean for public repo

---

## **Next Steps (Post-Sprint A5b)**
- **Episode 44** — UI & Feedback (health bars, cooldowns, virtual joystick, gamepad cursor)
- **Episode 45** — Final Gameplay Loop (waves, respawn, combat flow)
- **Episode 46** — Performance Optimization
- **Episode 47** — Final Showcase