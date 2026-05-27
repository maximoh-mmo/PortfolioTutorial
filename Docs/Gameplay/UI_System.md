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
- Provide minimal menu elements (optional)  

---

## **Non‑Responsibilities**
- Inventory  
- Skill trees  
- Quest UI  
- Dialogue  
- Complex HUD animations  
- Full menu systems  

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

---

## **Future Extensions**
- Animated ability icons  
- Damage numbers  
- Minimap  
- Full menu system  
- Style/theme pass  
- Mobile gesture support (pinch zoom, long-press for details)  