# 📘 **UI SYSTEM**  
**File:** `/Docs/Gameplay/UI_System.md`

---

# **UI System**

## **Purpose**
Provide clear, responsive UI for:

- Player health  
- Enemy health bars  
- Ability cooldowns  
- Target highlighting  
- Debug information (AI, autoplay, networking)  
- Final demo presentation  

The UI is intentionally minimal and functional — not a full RPG interface.
Touch targets sized for mobile (minimum 44×44 px). Ability buttons use on-screen touch controls alongside keyboard hotkeys.

---

## **Status**

- **Implemented:** CommonUI login/character-select screen stack, C++ character slots, world-transition loading screen (see **Current Implementation** below).
- **Planned (A6):** in-game HUD — player health, enemy health bars, ability bar/cooldowns, target indicators, debug overlays.

---

## **Current Implementation — Menu & Screen Stack (CommonUI)**

### CommonUI Screen Stack
- `UOnsetScreenBase` — `UCommonActivatableWidget` base for full-screen menus; handles activation/deactivation + input mode; Blueprint hooks `BP_OnScreenActivated` / `BP_OnScreenDeactivated`.
- `UOnsetRootLayout` (`WBP_RootLayout`) — single viewport-root widget with three layer stacks: **Game**, **Menu**, **Modal**.
- `UOnsetActivatableWidgetStack` — project subclass of `UCommonActivatableWidgetStack` with an `EOnsetUILayer` label.
- `UOnsetUISubsystem` — `UGameInstanceSubsystem` owning the root layout; exposes `PushScreen` / `PopScreen` / `CleanupUI` / `ShowLoadingScreen` / `HideLoadingScreen`.
- `UOnsetButtonBase` (`WBP_ButtonBase`) — button with hover/click sound hooks.
- `UOnsetGameViewportClient` — `UCommonGameViewportClient` subclass.

### Screens
- `UMainMenuScreen` (`WBP_MainMenu`) — `ConnectToServer()` pushes the character select screen.
- `UCharacterSelectScreen` (`WBP_CharacterSelect`) — builds three `UCharacterSlot` widgets dynamically into a bound panel container; occupied slot → `Server_SelectCharacter` (enter world), empty slot → creation screen. Refreshes in place via `OnAccountDataChanged`.
- `UCharacterCreationScreen` (`WBP_CharacterCreation`) — name input, class select, appearance presets → `Server_CreateCharacter`.

### Character Slot Widget
`UCharacterSlot` (`WBP_CharacterSlot`) — all logic in C++: occupied/empty display state (name, level, delete-button visibility), click routing via `OnSlotActivated` / `OnDeleteRequested`, and a `BP_OnSlotDataChanged` Blueprint hook for styling.

### Loading Screen
`UOnsetLoadingScreen` (`WBP_LoadingScreen`, backed by `M_Spinner`) — full-screen overlay shown by `UOnsetUISubsystem::ShowLoadingScreen()` before any world travel (create, select, reconnect). Tears down menu screens first, enforces a 0.5s minimum display, and hides on client pawn possession via `AOnsetPlayerController::OnPossess` (10s timeout fallback). Widget configured via `[Onset.UI] LoadingScreenClass` in `DefaultEngine.ini`.

---

## **Responsibilities**
- Display player health  
- Display enemy health bars  
- Display ability bar with cooldowns (touch-friendly buttons)  
- Show targeting indicators (single‑target, AoE, directional)  
- Show hit indicators  
- Provide on-screen touch controls for mobile (ability buttons, PvP toggle)  
- Provide debug overlays for:
  - AI state  
  - Autoplay mode  
  - Networking status  
- Provide login/character-select menus (CommonUI screen stack) and world-transition loading screens  

---

## **Non‑Responsibilities**
- Inventory  
- Skill trees  
- Quest UI  
- Dialogue  
- Complex HUD animations  
- Complex menu systems (settings, pause, inventory)  

---

## **Key Classes**

### **`UHUDWidget`** *(planned)*
- Main HUD  
- Contains health, abilities, debug panels  
- Scales layout for mobile vs desktop  

### **`UJoystickWidget`**
- Touch virtual joystick (movement thumbstick zone with drag-outline)
- Injects normalized axis directly into `IA_Move` via `UEnhancedInputLocalPlayerSubsystem::InjectInputForAction()`

### **`UGamepadCursorWidget`**
- Software crosshair overlay for gamepad R-Stick cursor
- Positioned via `SetRenderTranslation()` from PlayerController
- Automatically hides after idle timeout  

### **`UEnemyHealthBarWidget`** *(planned)*
- Attached to NPCs  
- Shows current health  

### **`UAbilityBarWidget`** *(planned)*
- Shows abilities + cooldowns  

### **`UTargetIndicatorWidget`** *(planned)*
- Shows targeting reticles  

### **`UAutoplayDebugWidget`** *(planned)*
- Shows player AI state  

---

## **Key Functions**

### **HUD**
- `UpdateHealth(float)`  
- `UpdateAbilityCooldown(int32 AbilityIndex, float TimeRemaining)`  

### **Enemy Health Bars**
- `SetHealthPercent(float)`  
- `Show/Hide()`  

### **Targeting**
- `ShowIndicator(FAbilityTargetData)`  
- `HideIndicator()`  

### **Debug**
- `SetAIState(FName)`  
- `SetAutoplayEnabled(bool)`  

---

## **Data Flow Diagram**

```
Player/NPC Attribute Changes
        │
        ▼
GAS AttributeSet
        │
        ▼
HUD / Health Bars Update
```

```
Player Input / AI Targeting
        │
        ▼
AbilityTargetingComponent
        │
        ▼
Target Indicator Widget
```

---

## **Interactions With Other Systems**

### **[Player System](../Player/Player_System.md)**
- Provides health, ability cooldowns, targeting data  

### **[NPC AI System](../AI/NPC_AI_System.md)**
- Provides enemy health values  
- Provides AI debug info  

### **[GAS System](../GAS/GAS_System.md)**
- Provides cooldowns and attribute changes  

### **[Player AI System](../AI/Player_AI_System.md)**
- Provides autoplay debug info  

### **[Multiplayer System](../Multiplayer/Multiplayer_System.md)**
- UI reads replicated PlayerState data  
- Loading screen covers world travel and hides on client possession  

### **[Account System](../Player/Account_System.md)**
- Drives character select/creation screens; `OnAccountDataChanged` refreshes slots in place  

---

## **Replication Rules**
- UI is **client‑side only**  
- Reads replicated data from:
  - PlayerState  
  - NPC attributes  
  - Ability cooldowns  

---

## **Edge Cases**
- Target dies while selected  
- Ability cooldown desync (client prediction)  
- Enemy health bar visibility in crowded scenes  
- Debug UI overlapping gameplay UI  

---

## **Testing Checklist**
- [x] Virtual joystick moves character on touch/mobile viewport  
- [x] Gamepad cursor renders and follows R-Stick  
- [ ] Player health updates correctly *(planned)*  
- [ ] Enemy health bars appear/disappear correctly *(planned)*  
- [ ] Ability cooldowns update correctly *(planned)*  
- [ ] Target indicators match ability behaviour *(planned)*  
- [ ] Debug UI toggles correctly *(planned)*  
- [ ] UI behaves correctly in multiplayer *(planned)*  
- [x] Main menu → character select → creation flow works (CommonUI screen stack)
- [x] Character slots refresh in place after create/delete
- [x] Loading screen shows during create/select/reconnect travel and hides on possession

---

## **Future Extensions**
- Animated ability icons  
- Damage numbers  
- Minimap  
- Settings/pause menus  
- Style/theme pass  
- Mobile gesture support (pinch zoom, long-press for details)  