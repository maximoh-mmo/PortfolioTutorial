# 🎬 **Episode 42 — Character Select UI & Login Flow**

## **Episode Goal**
Build the character select flow on the login server: a main-menu map, a 3-slot WBP character select screen, and the create/select/pick flow that ends in a tokenized client travel to the game server.

---

## **Context & Dependencies**
- Requires Episode 41 (SteamID Resolution — `Client_AccountData`, `Server_SelectCharacter`, `Server_CreateCharacter` working)
- Episode 40 (Database Architecture)
- CommonUI screen stack (Episodes 13-14) — `UOnsetScreenBase` + `UOnsetUISubsystem::PushScreen`

---

## **High‑Level Summary**
The login server's **MainMenu** map is the "waiting room" where authenticated players pick or create their character before traveling to the game server. We create a minimal level with no NPCs, no combat — just the character select HUD. The WBP widget shows 3 slots with name/level, handles creation of new characters, and hands off via a session token: the login server calls `Client_TravelToGameServer(IP, Port, Token)` and the client does `ClientTravel` to the game server with `?Token=` in the URL.

---

## **Key Concepts Introduced**
- Separator login-server pattern (auth + select before gameplay)
- 3-slot character select WBP (touch-first design)
- Tokenized client travel between server processes (`Client_TravelToGameServer`)
- Touch-friendly UI: large hit zones, virtual keyboard for naming

---

## **Technical Breakdown**

### **1. Create the Main Menu Map**
- **File:** `/Game/Maps/MainMenu.umap`
- Minimal level: just a `PlayerStart`, a `GameMode` reference, and a background (skybox or simple plane)
- No NPCs, no spawners, no AI — pure UI environment
- Set in **Project Settings → Maps & Modes → Game Default Map** = `MainMenu`
- Login server launches on `MainMenu`; game server launches on `DemoLevel` (`ServerDefaultMap`)
- On `PostLogin` the server sends `Client_ShowMainMenuUI(RootLayoutClass, MainMenuClass)` which pushes the menu onto the CommonUI stack

### **2. Character Select WBP — `WBP_CharacterSelect`**

#### **Widget Hierarchy**
```
WBP_CharacterSelect (UOnsetScreenBase / UCharacterSelectScreen)
├── BackgroundOverlay
│   ├── TitleText ("Select Your Adventurer")
│   ├── SlotContainer (HorizontalBox)
│   │   ├── Slot_0 (UCharacterSlot / WBP_CharacterSlot)
│   │   ├── Slot_1 (UCharacterSlot / WBP_CharacterSlot)
│   │   └── Slot_2 (UCharacterSlot / WBP_CharacterSlot)
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

#### **Key WBP Logic (Blueprint or C++ `UCharacterSelectScreen` subclass)**
- **On `Client_AccountData` received:**
  - The controller caches `CachedAccountData`, broadcasts `OnAccountDataChanged`, and calls `BP_OnAccountDataUpdated()`
  - The screen pads to 3 slots: `const int32 DesiredCount = FMath::Max(3, CachedAccountData.Slots.Num());` (`CharacterSelectScreen.cpp:154`)
  - For each slot: bind `FOnsetCharacterSlotData` → update `NameText`, `LevelText`, `StatusText`
  - Empty slots: show "Create" button when clicked
  - Occupied slots: show "Select" → enables "Enter World" button
- **Create flow:**
  - Click empty slot → show `CreateNameEntry` + `CreateButton`
  - Enter name + pick class + appearance preset → `Server_CreateCharacter(SlotIndex, Name, CharacterClass, AppearancePresetIndex)`
  - On `Client_AccountData` refresh → slot now occupied
- **Select flow:**
  - Click occupied slot → highlight border → store `SelectedSlotIndex`
  - Enable "Enter World" button
  - Press "Enter World" → `Server_SelectCharacter(SelectedSlotIndex)`
- **Delete flow:** occupied slots expose a delete button → `Server_DeleteCharacter(SlotIndex)`

### **3. Tokenized Travel to Game Server**
**In `AOnsetPlayerController`:**
```cpp
void AOnsetPlayerController::Server_SelectCharacter_Implementation(int32 SlotIndex)
{
    // ... load character, set PlayerState.SelectedCharacterSlot ...

    if (UOnsetAuthSubsystem* Auth = GetWorld()->GetSubsystem<UOnsetAuthSubsystem>())
    {
        FString Token = Auth->GenerateToken(Platform, PlatformID, SlotIndex);
        Client_TravelToGameServer(GameServerIP, GameServerPort, Token);
    }
}
```

`Client_TravelToGameServer` does `ClientTravel` to `IP:Port?Token=...`. The game server validates the token in `PreLogin`, then `HandlePostLogin` loads the account/character and spawns the pawn. On the client, `OnRep_Pawn` hides the loading screen overlay (`UOnsetLoadingScreen`) shown during the transition.

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
1. Launch login server → loads `MainMenu`
2. Connect client → Steam auth → `Client_ShowMainMenuUI` → `Client_AccountData` arrives
3. Widget shows 3 empty slots (padded from the account's slots array)
4. Click slot 0 → enter "Hero" + class + appearance → `Server_CreateCharacter` → slot fills
5. Click slot 0 → "Enter World" enables → press → token generated → `Client_TravelToGameServer`
6. Client travels to game server → pawn spawns at saved position (0,0,150 for new char)
7. Move, disconnect, reconnect → character select shows "Hero, Level 1"
8. Select → spawns at last saved position

---

## **Code Snippets**

```cpp
// AOnsetGameModeBase.cpp — zone travel (not ServerTravel to a second process)
void AOnsetGameModeBase::TravelToZone(const FString& MapName, const FString& EntryPoint)
{
    // used for in-process zone gates; login→game handoff is Client_TravelToGameServer
}

// WBP_CharacterSelect — OnAccountDataReceived (Blueprint)
ForEach SlotData in AccountData.Slots
    if SlotData.bOccupied
        SlotWidgets[SlotData.SlotIndex]->SetOccupied(SlotData.CharacterName, SlotData.Level)
    else
        SlotWidgets[SlotData.SlotIndex]->SetEmpty()

// WBP_CharacterSelect — OnCreateClicked
Server_CreateCharacter(SelectedSlotIndex, CreateNameEntry->GetText(), SelectedClass, AppearancePreset)

// WBP_CharacterSelect — OnEnterWorldClicked
Server_SelectCharacter(SelectedSlotIndex)
```

---

## **Common Pitfalls**
- `ServerTravel` from the login server does **not** carry the client across processes — the login→game hop uses `Client_TravelToGameServer` with a session token in the URL
- The game server re-derives identity from the token in `PreLogin` — `PlayerState` is not shared across processes
- Widget not updating on `Client_AccountData` → the screen must subscribe to `OnAccountDataChanged` or override `BP_OnAccountDataUpdated`
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
- [x] `MainMenu` created, set as default map
- [x] `WBP_CharacterSelect` with 3 slot widgets (padded to 3)
- [x] Slot shows name/level for occupied, "Create" for empty
- [x] Create flow: name/class/appearance → `Server_CreateCharacter` → slot fills
- [x] Select flow: highlight → "Enter World" enabled
- [x] Enter World → `Server_SelectCharacter` → `Client_TravelToGameServer` with token
- [x] Touch: virtual keyboard works for name entry
- [x] Gamepad: D-pad navigates, A selects, B backs out
- [x] Identity survives the login → game server hop (token validation)
- [x] Pawn spawns at saved position on DemoLevel entry
