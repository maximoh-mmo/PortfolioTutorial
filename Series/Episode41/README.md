# 🎬 **Episode 41 — Steam ID Resolution & Save/Load Flow**

## **Goal**
Extract the numeric SteamID from the auth ticket via `SteamGameServer()->BeginAuthSession()`, store it on `PlayerState`, and implement the full save/load RPC flow — account auto-create on first login, character load on select.

---

## **What Was Built**
Bridge from Steam auth to persistence. Server validates auth ticket, calls Steam's `BeginAuthSession()` to get SteamID64, stores as `(Platform="Steam", PlatformID="76561197960265728")` on `PlayerState`. PostLogin triggers account load (auto-create if first time). RPCs for character select/create/save wired end-to-end.

---

## **Project Snapshot**
Full Unreal project at end of Episode 41.

### **Key Files Added/Modified**
| File | Description |
|---|---|
| `Source/Onset/Public/Player/OnsetPlayerState.h` | Added `PlayerPlatformID`, `PlayerPlatform`, `SelectedCharacterSlot` |
| `Source/Onset/Public/Player/OnsetPlayerController.h` | Added `Client_AccountData`, `Client_CharacterData`, `Server_SelectCharacter`, `Server_CreateCharacter`, `Server_SaveCharacter` RPCs |
| `Source/Onset/Private/Player/OnsetPlayerController.cpp` | RPC implementations |
| `Source/Onset/Public/Multiplayer/OnsetGameModeBase.h` | SteamID extraction helper |
| `Source/Onset/Private/Multiplayer/OnsetGameModeBase.cpp` | `ValidateAuthTicket` → `BeginAuthSession` → account load |

### **New RPCs**
| RPC | Direction | Purpose |
|---|---|---|
| `Client_AccountData` | Server→Client | Send `FOnsetAccountData` (3 slots) |
| `Client_CharacterData` | Server→Client | Send `FOnsetFullCharacterData` on select |
| `Server_SelectCharacter(int32)` | Client→Server | Pick slot, spawn pawn, apply save |
| `Server_CreateCharacter(int32, FString)` | Client→Server | Create new char in empty slot |
| `Server_SaveCharacter()` | Client→Server | Manual save request |
| `Client_SaveComplete(bool)` | Server→Client | Save confirmation |

---

## **How to Test**
1. Launch DS: `Onset.exe ... -server -log`
2. Launch client: `Onset.exe ... 127.0.0.1 -game`
3. Client auto-sends auth ticket → DS validates → extracts SteamID
4. Check log: `PostLogin: auto-created account for Steam/<SteamID64>` (first login)
5. Client receives `Client_AccountData` → shows 3 empty slots
6. Click slot 0 → enter "Hero" → `Server_CreateCharacter(0, "Hero")`
7. Slot fills → "Hero, Level 1"
8. Click slot 0 → "Enter World" → `Server_SelectCharacter(0)`
8. Pawn spawns at saved position (0,0,200 default)

---

## **Code Snippets**

```cpp
// OnsetGameModeBase.cpp — SteamID extraction
void AOnsetGameModeBase::ValidateAuthTicket(APlayerController* NewPlayer, const FString& AuthTicket)
{
    // ... existing validation ...
    
    // Extract SteamID via Steamworks
    CSteamID SteamID;
    if (SteamGameServer()->BeginAuthSession(TCHAR_TO_UTF8(*AuthTicket), AuthTicket.Len(), SteamID))
    {
        FString SteamIDStr = FString::Printf(TEXT("%llu"), SteamID.ConvertToUint64());
        AOnsetPlayerState* PS = NewPlayer->GetPlayerState<AOnsetPlayerState>();
        if (PS)
        {
            PS->PlayerPlatformID = SteamIDStr;
            PS->PlayerPlatform = TEXT("Steam");
        }
        // Load account
        if (UOnsetPlayerDataSubsystem* DataSub = GetWorld()->GetSubsystem<UOnsetPlayerDataSubsystem>())
        {
            FOnsetAccountData AccountData = DataSub->GetAccountData(TEXT("Steam"), SteamIDStr);
            NewPlayer->Client_AccountData(AccountData);
        }
    }
}

// OnsetPlayerController.cpp — Server_SelectCharacter
void AOnsetPlayerController::Server_SelectCharacter_Implementation(int32 SlotIndex)
{
    if (!HasAuthority()) return;
    if (SlotIndex < 0 || SlotIndex > 2) return;
    
    AOnsetPlayerState* PS = GetPlayerState<AOnsetPlayerState>();
    if (!PS) return;
    
    if (UOnsetPlayerDataSubsystem* DataSub = GetWorld()->GetSubsystem<UOnsetPlayerDataSubsystem>())
    {
        FOnsetFullCharacterData CharData = DataSub->GetCharacterData(PS->PlayerPlatform, PS->PlayerPlatformID, SlotIndex);
        if (!CharData.IsValid()) return; // slot empty
        
        PS->SelectedCharacterSlot = SlotIndex;
        // Spawn/reposition pawn, apply CharData
        // ...
        Client_CharacterData(CharData);
    }
}
```

---

## **Dependencies**
- Episode 40 (Database Architecture)
- Episode 39 (Steam Auth ticket flow)

---

## **Diagrams**

```
Client                          DS (GameMode)
──────────────────────────────────────────────────────────────
    │                               │
    ├── Auth Ticket ───────────────►│
    │                               │  SteamGameServer()->BeginAuthSession()
    │                               │  → SteamID64
    │                               │  PS->PlayerPlatformID = "76561197960265728"
    │                               │  PS->PlayerPlatform = "Steam"
    │                               │  DataSub->LoadAccount(Steam, SteamID)
    │                               │  (auto-create if first login)
    │◄── Client_AccountData ────────│
    │                               │
    │  [Character Select]           │
    │                               │
    ├── Server_SelectCharacter(0) ─►│
    │                               │  DataSub->LoadCharacter(Steam, SteamID, 0)
    │                               │  Spawn pawn at saved position
    │                               │  Apply saved MaxHealth, etc.
    │◄── Client_CharacterData ──────│
    │                               │
    │  [ServerTravel to DemoLevel]  │
    │                               │
    │  [Enter World]                │
```

---

## **Common Pitfalls**
- `BeginAuthSession` requires Steamworks SDK headers in GameMode
- PlayerState not replicated to client before `Client_AccountData` → use `PlayerController` RPC directly
- `ServerTravel` resets PlayerController but `PlayerState` persists — ensure `SelectedCharacterSlot` is on `PlayerState`
- Empty slot selected → `LoadCharacter` returns invalid → guard in UI

---

## **Episode Checklist**
- [ ] `PlayerPlatformID` / `PlayerPlatform` on `AOnsetPlayerState`
- [ ] `ValidateAuthTicket` calls `BeginAuthSession`, extracts SteamID
- [ ] `PostLogin` loads account, auto-creates if first login
- [ ] `Client_AccountData` RPC works, client shows 3 slots
- [ ] `Server_SelectCharacter` loads char, spawns pawn, sends `Client_CharacterData`
- [ ] `Server_CreateCharacter` inserts new character row
- [ ] `Server_SaveCharacter` + `Client_SaveComplete` round-trip
- [ ] Snapshot clean for public repo