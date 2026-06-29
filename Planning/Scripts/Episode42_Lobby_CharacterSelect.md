# 🎬 **Episode 42 — Lobby Map & Character Select UI**

## **Episode Goal**
Build a lightweight lobby map on the DS, implement a 3-slot WBP character select screen, and wire create/select/pick flow. `ServerTravel` to the game map on ready.

---

## **Context & Dependencies**
- Requires Episode 41 (SteamID Resolution — `Client_AccountData`, `Server_SelectCharacter`, `Server_CreateCharacter` working)
- Episode 40 (Database Architecture)
- Existing WBP pattern from `GamepadCursorWidget` (touch + gamepad + mouse unified)

---

## **High‑Level Summary**
The lobby map is the "waiting room" where authenticated players pick or create their character before entering the game world. We create a minimal level with no NPCs, no combat — just the character select HUD. The WBP widget shows 3 slots with name/level, handles creation of new characters, and transitions to the game map via `ServerTravel`.

---

## **Key Concepts Introduced**
- Separator lobby map pattern (auth + select before gameplay)
- 3-slot character select WBP (touch-first design)
- `ServerTravel` for seamless map transition with connected players
- Touch-friendly UI: large hit zones, virtual keyboard for naming

---

## **Technical Breakdown**

### **1. Create Lobby Map**
- **File:** `/Game/Maps/LobbyMap.umap`
- Minimal level: just a `PlayerStart`, a `GameMode` reference, and a background (skybox or simple plane)
- No NPCs, no spawners, no AI — pure UI environment
- Set in **Project Settings → Maps & Modes → Game Default Map** = `LobbyMap`
- This ensures DS launches on LobbyMap, clients join LobbyMap

### **2. Character Select WBP — `WBP_CharacterSelect`**

#### **Widget Hierarchy**
```
WBP_CharacterSelect (UserWidget)
├── BackgroundOverlay
│   ├── TitleText ("Select Your Adventurer")
│   ├── SlotContainer (HorizontalBox)
│   │   ├── Slot_0 (CharacterSlotWidget)
│   │   ├── Slot_1 (CharacterSlotWidget)
│   │   └── Slot_2 (CharacterSlotWidget)
│   ├── CreateNameEntry (EditableTextBox) — hidden by default
│   ├── CreateButton (Button) — "Create Character"
│   ├── SelectButton (Button) — "Enter World" (enabled when slot selected)
│   └── StatusText (TextBlock) — "Select a character..." / error messages
```

#### **CharacterSlotWidget (UserWidget)**
```
SlotWidget
├── Border (selected highlight)
├── VerticalBox
│   ├── NameText (TextBlock) — "Adventurer" or "Empty Slot"
│   ├── LevelText (TextBlock) — "Level 1" or "—"
│   └── StatusText (TextBlock) — "Available" / "Occupied"
```

#### **Key WBP Logic (Blueprint or C++ `UUserWidget` subclass)**
- **On `Client_AccountData` received:**
  - For each of 3 slots: bind `FOnsetCharacterSlotData` → update `NameText`, `LevelText`, `StatusText`
  - Empty slots: show "Create" button when clicked
  - Occupied slots: show "Select" → enables "Enter World" button
- **Create flow:**
  - Click empty slot → show `CreateNameEntry` + `CreateButton`
  - Enter name → `Server_CreateCharacter(SlotIndex, Name)`
  - On `Client_AccountData` refresh → slot now occupied
- **Select flow:**
  - Click occupied slot → highlight border → store `SelectedSlotIndex`
  - Enable "Enter World" button
  - Press "Enter World" → `Server_SelectCharacter(SelectedSlotIndex)`
- **On `Client_CharacterData` (after select):**
  - Server will `ServerTravel` — widget just waits for level transition

### **3. ServerTravel to Game Map**
**In `AOnsetGameModeBase` or `AOnsetPlayerController`:**
```cpp
void AOnsetPlayerController::Server_SelectCharacter_Implementation(int32 SlotIndex)
{
    // ... load character, spawn pawn, apply save data ...
    
    // Once all connected players have selected (or after short delay), travel
    if (AOnsetGameModeBase* GM = GetWorld()->GetAuthGameMode<AOnsetGameModeBase>())
    {
        GM->RequestTravelToGameMap();
    }
}
```

**In `AOnsetGameModeBase`:**
```cpp
void AOnsetGameModeBase::RequestTravelToGameMap()
{
    // Optional: wait for all players to have SelectedCharacterSlot != INDEX_NONE
    // For demo: travel immediately after first player selects
    
    FString GameMap = TEXT("/Game/Maps/DemoLevel");
    GetWorld()->ServerTravel(GameMap, true, true); // absolute, notify clients
}
```

- `ServerTravel` preserves PlayerController/PlayerState connections
- Players arrive at `DemoLevel` with their `SelectedCharacterSlot` set
- `PostLogin` on new map → `GameMode` sees `SelectedCharacterSlot` set → spawns pawn at saved position

### **4. Touch / Gamepad / Mouse Support**
- **Touch:** Large slot buttons (min 80px), `CreateNameEntry` summons virtual keyboard
- **Gamepad:** D-pad / left stick navigates between slots, A = select/create, B = back
- **Mouse:** Click slots, type name in entry box
- Reuse `UCursorManager` abstraction from Episode 3 for unified cursor position

### **5. Visual Polish**
- Background: Simple skybox or dark gradient panel
- Slot hover: scale 1.05x, glow outline
- Selected slot: gold border, pulse animation
- "Enter World" button: disabled (gray) until slot selected, then bright green
- Loading spinner on "Enter World" press (disable buttons until travel)

---

## **How to Test**
1. Launch DS → loads `LobbyMap`
2. Connect client → Steam auth → `Client_AccountData` arrives
3. Widget shows 3 empty slots
4. Click slot 0 → enter "Hero" → `Server_CreateCharacter` → slot fills
5. Click slot 0 → "Enter World" enables → press → `ServerTravel` to `DemoLevel`
6. Pawn spawns at saved position (0,0,200 for new char)
7. Move, disconnect, reconnect → character select shows "Hero, Level 1"
8. Select → spawns at last saved position

---

## **Code Snippets**

```cpp
// AOnsetGameModeBase.cpp — ServerTravel
void AOnsetGameModeBase::RequestTravelToGameMap()
{
    FString GameMap = TEXT("/Game/Maps/DemoLevel");
    GetWorld()->ServerTravel(GameMap, true, true);
}

// WBP_CharacterSelect — OnAccountDataReceived (Blueprint)
ForEach SlotData in AccountData.Slots
    if SlotData.bOccupied
        SlotWidgets[SlotData.SlotIndex]->SetOccupied(SlotData.CharacterName, SlotData.Level)
    else
        SlotWidgets[SlotData.SlotIndex]->SetEmpty()

// WBP_CharacterSelect — OnCreateClicked
Server_CreateCharacter(SelectedSlotIndex, CreateNameEntry->GetText())

// WBP_CharacterSelect — OnEnterWorldClicked
Server_SelectCharacter(SelectedSlotIndex)
```

---

## **Common Pitfalls**
- `ServerTravel` called on client → must be server-only (`HasAuthority()`)
- PlayerState not carried over `ServerTravel` → ensure `PlayerStateClass` is set and persists
- Widget not updating on `Client_AccountData` → bind in `NativeConstruct` or `OnInitialized`
- Virtual keyboard not showing on touch → `EditableTextBox` needs `SupportsVirtualKeyboard = true`
- "Enter World" enabled before slot selected → guard with `SelectedSlotIndex != INDEX_NONE`

---

## **Dependencies**
- Episode 41 (Save/Load RPCs)
- Episode 3 (Cursor abstraction for unified input)

---

## **Next Episode Preview**
End-to-end verification: auth → account load → character select → spawn → auto-save timer → save-on-disconnect. Production hardening with WAL mode, crash recovery, and migration tests.

---

## **Episode Checklist**
- [ ] `LobbyMap` created, set as default map
- [ ] `WBP_CharacterSelect` with 3 slot widgets
- [ ] Slot shows name/level for occupied, "Create" for empty
- [ ] Create flow: name entry → `Server_CreateCharacter` → slot fills
- [ ] Select flow: highlight → "Enter World" enabled
- [ ] Enter World → `Server_SelectCharacter` → `ServerTravel` to DemoLevel
- [ ] Touch: virtual keyboard works for name entry
- [ ] Gamepad: D-pad navigates, A selects, B backs out
- [ ] PlayerState persists across `ServerTravel`
- [ ] Pawn spawns at saved position on DemoLevel entry