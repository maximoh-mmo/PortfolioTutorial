# 🎬 **Episode 42 — Lobby Map & Character Select UI**

## **Goal**
Build a lightweight lobby map on the DS, implement a 3‑slot WBP character select screen, and wire create/select/pick flow. `ServerTravel` to the game map on ready.

---

## **What Was Built**
Lobby map (`/Game/Maps/LobbyMap`) — no NPCs, no combat, just the character select HUD. `WBP_CharacterSelect` with 3 slot panels showing name/level for occupied slots, "Create" button for empty. Create → name entry → `Server_CreateCharacter`. Select → highlight → "Enter World" enabled → `Server_SelectCharacter` → `ServerTravel` to `DemoLevel`.

---

## **Project Snapshot**
Full Unreal project at end of Episode 42.

### **Key Files Added/Modified**
| File | Description |
|---|---|
| `/Game/Maps/LobbyMap` | Minimal level, default pawn = none, no spawners |
| `/Game/UI/WBP_CharacterSelect` | 3-slot character select widget |
| `/Game/UI/WBP_CharacterSlot` | Single slot panel (name, level, Create/Select buttons) |
| `Source/Onset/Public/Player/OnsetPlayerController.h` | Lobby map name constant |
| `Source/Onset/Private/Player/OnsetPlayerController.cpp` | Lobby map travel logic |
| `Config/DefaultEngine.ini` | `GameDefaultMap=/Game/Maps/LobbyMap` |

### **New Assets**
| Asset | Type | Purpose |
|---|---|---|
| `LobbyMap` | Level | DS default map, character select only |
| `WBP_CharacterSelect` | Widget Blueprint | Main character select screen |
| `WBP_CharacterSlot` | Widget Blueprint | Reusable slot panel |

---

## **How to Test**
1. DS launches on `LobbyMap` (set in `DefaultEngine.ini`)
2. Client connects → auth → receives `Client_AccountData`
3. `WBP_CharacterSelect` appears with 3 slots
4. Empty slot → click "Create" → name entry → slot fills
5. Occupied slot → click → highlights → "Enter World" enabled
6. Press "Enter World" → `Server_SelectCharacter` → DS `ServerTravel` to `DemoLevel`
7. Client arrives at `DemoLevel`, pawn at saved position

---

## **Code Snippets**

```cpp
// DefaultEngine.ini
[/Script/EngineSettings.GameMapsSettings]
GameDefaultMap=/Game/Maps/LobbyMap
EditorStartupMap=/Game/Maps/LobbyMap

// OnsetPlayerController.cpp — Lobby map
void AOnsetPlayerController::Client_AccountData_Implementation(const FOnsetAccountData& AccountData)
{
    if (UWBP_CharacterSelect* Widget = Cast<UWBP_CharacterSelect>(CharacterSelectWidget))
    {
        Widget->SetAccountData(AccountData);
        Widget->SetVisibility(ESlateVisibility::Visible);
    }
}

// WBP_CharacterSelect — SetAccountData (Blueprint)
ForEach SlotData in AccountData.Slots
    SlotWidgets[SlotData.SlotIndex]->UpdateSlot(SlotData)

// WBP_CharacterSlot — OnCreateClicked
OnCreateClicked()
    CreateNameEntry->SetVisibility(Visible)
    CreateConfirmButton->SetVisibility(Visible)

OnCreateConfirmClicked()
    Server_CreateCharacter(SlotIndex, CreateNameEntry->GetText())

// WBP_CharacterSlot — OnSelectClicked
OnSelectClicked()
    ParentWidget->SetSelectedSlot(SlotIndex)

// WBP_CharacterSelect — OnEnterWorldClicked
OnEnterWorldClicked()
    if (SelectedSlotIndex != INDEX_NONE)
        Server_SelectCharacter(SelectedSlotIndex)

// OnsetGameModeBase.cpp — ServerTravel
void AOnsetGameModeBase::RequestTravelToGameMap()
{
    FString GameMap = TEXT("/Game/Maps/DemoLevel");
    GetWorld()->ServerTravel(GameMap, true, true);
}
```

---

## **Dependencies**
- Episode 41 (Save/Load RPCs)
- Episode 3 (Cursor abstraction for unified input)

---

## **Diagrams**

```
LobbyMap (DS)                          Client
──────────────────────────────────────────────────────────────
    │                                       │
    │  [Auth Complete]                      │
    │  Client_AccountData ─────────────────►│
    │                                       │  WBP_CharacterSelect opens
    │                                       │  3 slots: [Empty] [Empty] [Empty]
    │                                       │
    │                                       │  Click Slot 0 "Create"
    │                                       │  Enter "Hero"
    │  Server_CreateCharacter(0,"Hero") ◄──│
    │  INSERT INTO characters ...           │
    │  Client_AccountData (updated) ───────►│
    │                                       │  Slot 0: "Hero, Level 1" [Select] [Enter World]
    │                                       │
    │                                       │  Click Slot 0 → Select
    │                                       │  "Enter World" enabled
    │                                       │  Press Enter World
    │  Server_SelectCharacter(0) ◄─────────│
    │  LoadCharacter → Spawn Pawn           │
    │  Client_CharacterData ───────────────►│
    │                                       │
    │  ServerTravel(/Game/Maps/DemoLevel) ──│
    │                                       │  Loading screen
    │                                       │  Arrive at DemoLevel
    │                                       │  Pawn at saved position
```

---

## **Common Pitfalls**
- `ServerTravel` called on client → must be server-only (`HasAuthority()`)
- PlayerState not carried over `ServerTravel` → ensure `PlayerStateClass` set and `SelectedCharacterSlot` on `PlayerState`
- Widget not updating on `Client_AccountData` → bind in `NativeConstruct` or `OnInitialized`
- Virtual keyboard not showing on touch → `EditableTextBox` needs `SupportsVirtualKeyboard = true`
- "Enter World" enabled before slot selected → guard with `SelectedSlotIndex != INDEX_NONE`

---

## **Episode Checklist**
- [ ] `LobbyMap` created, set as `GameDefaultMap`
- [ ] `WBP_CharacterSelect` with 3 `WBP_CharacterSlot` children
- [ ] Slot shows name+level for occupied, "Create" for empty
- [ ] Create flow: name entry → `Server_CreateCharacter` → slot fills
- [ ] Select flow: highlight → "Enter World" enabled
- [ ] Enter World → `Server_SelectCharacter` → `ServerTravel` to DemoLevel
- [ ] Touch: virtual keyboard works for name entry
- [ ] Gamepad: D-pad navigates, A selects, B backs out
- [ ] PlayerState persists across `ServerTravel`
- [ ] Pawn spawns at saved position on DemoLevel entry
- [ ] Snapshot clean for public repo