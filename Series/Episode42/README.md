# 🎬 **Episode 42 — Character Select UI & Login Flow**

## **Goal**
Build the character select flow: a main-menu map on the login server, a 3‑slot WBP character select screen, and the create/select/pick flow that ends with a tokenized travel to the game server. Travel is a client-side `Client_TravelToGameServer`, not a server `ServerTravel`.

---

## **What Was Built**
The login server hosts the **MainMenu** map (`/Game/Maps/MainMenu`) — no NPCs, no combat. On `PostLogin` the server sends `Client_ShowMainMenuUI`, and after account data arrives (`Client_AccountData`) the menu shows the 3-slot character select. `WBP_CharacterSelect` shows name/level for occupied slots and a "Create" flow for empty ones. Create → class/appearance selection → `Server_CreateCharacter`. Select → highlight → "Enter World" enabled → `Server_SelectCharacter` → login server generates a session token and calls `Client_TravelToGameServer(IP, Port, Token)` → client travels to the game server with `?Token=` in the URL.

---

## **Project Snapshot**
Full Unreal project at end of Episode 42.

### **Key Files Added/Modified**
| File | Description |
|---|---|
| `/Game/Maps/MainMenu` | Login-server map, default pawn = none, no spawners |
| `/Game/UI/Screens/WBP_CharacterSelect` | 3-slot character select widget |
| `/Game/UI/Screens/WBP_CharacterCreation` | Create-character (name + class + appearance) |
| `/Game/UI/Components/WBP_CharacterSlot` | Single slot panel (name, level, Create/Select) |
| `Source/Onset/Public/Player/OnsetPlayerController.h` | `Server_CreateCharacter(int32, FString, EOnsetCharacterClass, int32)` |
| `Source/Onset/Private/Player/OnsetPlayerController.cpp` | Account data broadcast, travel logic |
| `Config/DefaultEngine.ini` | `GameDefaultMap=/Game/Maps/MainMenu` |

### **New Assets**
| Asset | Type | Purpose |
|---|---|---|
| `MainMenu` | Level | Login-server default map, character select only |
| `WBP_CharacterSelect` | Widget Blueprint | Main character select screen |
| `WBP_CharacterCreation` | Widget Blueprint | Create flow (name, class, appearance preset) |
| `WBP_CharacterSlot` | Widget Blueprint | Reusable slot panel |

---

## **How to Test**
1. Login server launches on `MainMenu` (set in `DefaultEngine.ini`)
2. Client connects → auth → `Client_ShowMainMenuUI` → `Client_AccountData`
3. `WBP_CharacterSelect` appears with 3 slots (padded from the account's slots array)
4. Empty slot → click "Create" → name/class/appearance entry → slot fills
5. Occupied slot → click → highlights → "Enter World" enabled
6. Press "Enter World" → `Server_SelectCharacter` → login server generates token → `Client_TravelToGameServer`
7. Client travels to the game server, arrives at `DemoLevel`, pawn at saved position

---

## **Code Snippets**

```cpp
// DefaultEngine.ini
[/Script/EngineSettings.GameMapsSettings]
GameDefaultMap=/Game/Maps/MainMenu.MainMenu
ServerDefaultMap=/Game/Maps/DemoLevel.DemoLevel

// OnsetPlayerController.cpp — Account data arrives
void AOnsetPlayerController::Client_AccountData_Implementation(const FOnsetAccountData& AccountData)
{
    CachedAccountData = AccountData;
    OnAccountDataChanged.Broadcast();   // CharacterSelectScreen listens to this
    BP_OnAccountDataUpdated();
}

// CharacterSelectScreen.cpp — pad slots to 3
const int32 DesiredCount = FMath::Max(3, CachedAccountData.Slots.Num());

// WBP_CharacterSelect — OnEnterWorldClicked
OnEnterWorldClicked()
    if (SelectedSlotIndex != INDEX_NONE)
        Server_SelectCharacter(SelectedSlotIndex)

// OnsetPlayerController.cpp — select → tokenized travel
void AOnsetPlayerController::Server_SelectCharacter_Implementation(int32 SlotIndex)
{
    // load character data, set PlayerState.SelectedCharacterSlot
    // Direct mode: GenerateToken(...) → Client_TravelToGameServer(IP, Port, Token)
}
```

---

## **Dependencies**
- Episode 41 (Save/Load RPCs)
- Episode 40 (Database Architecture — account/character data)
- Episodes 37-39 (Dedicated server + Steam auth)

---

## **Diagrams**

```
LoginServer (MainMenu)                    Client
──────────────────────────────────────────────────────────────
    │                                       │
    │  [Auth Complete]                      │
    │  Client_ShowMainMenuUI ──────────────►│
    │  Client_AccountData ─────────────────►│
    │                                       │  WBP_CharacterSelect opens
    │                                       │  3 slots: [Empty] [Empty] [Empty]
    │                                       │
    │                                       │  Click Slot 0 "Create"
    │                                       │  Enter "Hero" + class + appearance
    │  Server_CreateCharacter(...) ◄───────│
    │  INSERT INTO characters ...           │
    │  Client_AccountData (updated) ───────►│
    │                                       │  Slot 0: "Hero, Level 1" [Select] [Enter World]
    │                                       │
    │                                       │  Click Slot 0 → Select
    │                                       │  "Enter World" enabled
    │                                       │  Press Enter World
    │  Server_SelectCharacter(0) ◄─────────│
    │  LoadCharacter → GenerateToken        │
    │  Client_TravelToGameServer(IP,Port,Token) ──►
    │                                       │  ClientTravel with ?Token= in URL
    │                                       │  Loading screen overlay
    │                                       │  Arrive at DemoLevel (game server)
    │                                       │  Pawn spawns at saved position
    │                                       │  OnRep_Pawn → hide loading screen
```

---

## **Common Pitfalls**
- Travel is `Client_TravelToGameServer` (client-side travel with token in URL) — not a server `ServerTravel`; the login and game servers are separate processes
- PlayerState not carried across processes → the game server re-derives identity from the token (`PreLogin` validates it)
- Widget not updating on `Client_AccountData` → the screen must subscribe to `OnAccountDataChanged` or override `BP_OnAccountDataUpdated`
- Virtual keyboard not showing on touch → `EditableTextBox` needs `SupportsVirtualKeyboard = true`
- "Enter World" enabled before slot selected → guard with `SelectedSlotIndex != INDEX_NONE`

---

## **Episode Checklist**
- [x] `MainMenu` created, set as `GameDefaultMap`
- [x] `WBP_CharacterSelect` with 3 `WBP_CharacterSlot` children (padded to 3)
- [x] Slot shows name+level for occupied, "Create" for empty
- [x] Create flow: name/class/appearance → `Server_CreateCharacter` → slot fills
- [x] Select flow: highlight → "Enter World" enabled
- [x] Enter World → `Server_SelectCharacter` → `Client_TravelToGameServer` with token
- [x] Touch: virtual keyboard works for name entry
- [x] Gamepad: D-pad navigates, A selects, B backs out
- [x] Identity survives the login → game server hop (token validation)
- [x] Pawn spawns at saved position on DemoLevel entry
- [x] Snapshot clean for public repo
