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
- **Implemented (A6):** in-game HUD — player health bar, target frame with target-type skins, ability bar with cooldowns, pooled damage numbers, combat (PvP) toggle, ground-reticle decal targeting indicator. Content lives under `Content/UI/` with the new content pipeline (`Docs/UI_ASSET_CHECKLIST.md`).
- **Implemented (items pass):** loot overlay popup listing just-looted items (`ULootOverlayWidget`), driven by `Client_ShowLootOverlay`. See [Inventory & Loot System](../Inventory/Inventory_System.md).

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
`UOnsetLoadingScreen` (`WBP_LoadingScreen`, backed by `M_Spinner`) — full-screen overlay shown by `UOnsetUISubsystem::ShowLoadingScreen()` before any world travel (create, select, reconnect). Tears down menu screens first, enforces a 0.5s minimum display, and hides on client pawn possession via `AOnsetPlayerController::OnRep_Pawn` (10s timeout fallback). Widget configured via `[Onset.UI] LoadingScreenClass` in `DefaultEngine.ini`.

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
  - Networking status  
- Autoplay state feedback — carried by the `UCombatToggleWidget` toggle visual (dedicated debug overlay scrapped)
- Provide login/character-select menus (CommonUI screen stack) and world-transition loading screens  
- Show the loot overlay after clicking a corpse (list of just-looted items, rarity-tinted)  

---

## **Non‑Responsibilities**
- Full inventory management UI (bag/equipment screens) — loot is handled by the [Inventory & Loot System](../Inventory/Inventory_System.md) + this overlay; a full bag UI is future work
- Skill trees  
- Quest UI  
- Dialogue  
- Complex HUD animations  
- Complex menu systems (settings, pause, inventory)  

---

## **Key Classes**

### **`UHUDWidget`** (`WBP_HUD`)
- Main in-game HUD, created and bound in `AOnsetPlayerController::OnRep_Pawn`
- Owns the player health bar, ability bar, combat toggle, and target frame
- `BindToPlayer(Controller, Pawn)` wires each sub-widget to the player's `AbilitySystemComponent` and `TargetingComponent`
- Listens for `OnTargetChanged` → updates `UTargetHUDWidget::SetTarget` and subscribes the target ASC
- Spawns pooled damage numbers (`SpawnDamageNumber`) on player/target health drops
- Applies the DPI-corrected `ProjectToScreen` (viewport pixels ÷ widget geometry scale) so damage numbers land on the correct world location

### **`UPlayerHealthBarWidget`** (`WBP_PlayerHealthBar`)
- Player health bar bound to the player's ASC; `BindToASC` subscribes to health attribute changes

### **`UTargetHUDWidget`** (`WBP_TargetHUD`)
- Static target frame (does not float above the target); shows target name + health via `SetTarget(AActor*)`
- Uses target-type skins (player vs NPC) for the frame styling

### **`UAbilityBarWidget`** / **`UAbilitySlotWidget`** (`WBP_AbilityBar` / `WBP_AbilitySlot`)
- Ability bar with per-slot cooldown overlays; `BindToPlayer` reads granted abilities from the player ASC and reads each ability's icon/cooldown

### **`UDamageNumberWidget`** (`WBP_DamageNumber`)
- Pooled world-anchored damage number; `ShowDamage(Amount, ScreenPos, Color)` activates, animates, and deactivates; round-robin pool recycling

### **`UCombatToggleWidget`** (`WBP_CombatToggle`)
- PvP/PvE toggle button; `BindToPlayer` routes to `Server_SetPvPEnabled`

### **`UJoystickWidget`**
- Touch virtual joystick (movement thumbstick zone with drag-outline)
- Injects normalized axis directly into `IA_Move` via `UEnhancedInputLocalPlayerSubsystem::InjectInputForAction()`

### **`UGamepadCursorWidget`**
- Software crosshair overlay for gamepad R-Stick cursor
- Positioned via `SetRenderTranslation()` from PlayerController
- Automatically hides after idle timeout  

### **`UOnsetLoadingScreen`** (`WBP_LoadingScreen`)
- Full-screen overlay shown by `UOnsetUISubsystem::ShowLoadingScreen()` before any world travel; hides on client pawn possession (`AOnsetPlayerController::OnRep_Pawn`, 10s timeout fallback)

### **`ULootOverlayWidget`**
- Popup overlay listing the items picked up from a looted corpse (items auto-inventory on loot, so the overlay is purely informational)
- Fully built in C++ (bottom-center panel) so it works with no authored asset; `Blueprintable` with `BindWidgetOptional ItemList` for a designer WBP override
- Rarity-tinted rows; `ShowLoot(TArray<FOnsetInventoryEntry>)` populates + shows; auto-hides after `Lifetime` (4s)
- Triggered by `AOnsetPlayerController::Client_ShowLootOverlay` (Client RPC); created lazily by `UHUDWidget::ShowLoot`

---

## **Key Functions**

### **HUD**
- `BindToPlayer(Controller, Pawn)`  
- `HandleTargetChanged(AActor*)` — swaps target ASC binding and updates `UTargetHUDWidget`
- `SpawnDamageNumber(WorldLocation, Amount, Color)` — projects to screen and activates a pooled damage number

### **Player Health Bar**
- `BindToASC(UAbilitySystemComponent*)`  
- Health updates driven by ASC attribute-change delegates

### **Targeting**
- `UTargetHUDWidget::SetTarget(AActor*)` — updates target frame (name/health/skin)
- Ground-reticle decal: attached to the current target by the targeting flow (see `Targeting_System.md`)

### **Debug**
- `SetAIState(FName)` *(planned)*  
- ~~`SetAutoplayEnabled(bool)`~~ *(scrapped — the `UCombatToggleWidget` toggle visual provides autoplay state feedback)*  

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
TargetingComponent (CurrentTarget)
        │
        ▼
Target Frame (UTargetHUDWidget) + Ground Reticle Decal
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

### **[Inventory & Loot System](../Inventory/Inventory_System.md)**
- Loot overlay displays the items transferred by click-to-loot (`Client_ShowLootOverlay` RPC → `ShowLoot`)

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
- [x] Player health bar updates from ASC health attribute changes
- [x] Target frame shows the selected target's name/health with target-type skins
- [x] Ability bar shows granted abilities with cooldown overlays
- [x] Pooled damage numbers appear on player/target damage and recycle round-robin
- [x] Ground-reticle decal tracks the current target
- [ ] Debug UI toggles correctly *(planned)*  
- [ ] UI behaves correctly in multiplayer *(planned)*  
- [x] Main menu → character select → creation flow works (CommonUI screen stack)
- [x] Character slots refresh in place after create/delete
- [x] Loading screen shows during create/select/reconnect travel and hides on possession
- [ ] Loot overlay pops on corpse click, lists looted items with rarity tinting, auto-hides after 4s

---

## **Future Extensions**
- Animated ability icons  
- Minimap  
- Settings/pause menus  
- Style/theme pass  
- Mobile gesture support (pinch zoom, long-press for details)  