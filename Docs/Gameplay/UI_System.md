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

---

## **Responsibilities**
- Display player health  
- Display enemy health bars  
- Display ability bar with cooldowns  
- Show targeting indicators (single‑target, AoE, directional)  
- Show hit indicators  
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

### **`UHUDWidget`**
- Main HUD  
- Contains health, abilities, debug panels  

### **`UEnemyHealthBarWidget`**
- Attached to NPCs  
- Shows current health  

### **`UAbilityBarWidget`**
- Shows abilities + cooldowns  

### **`UTargetIndicatorWidget`**
- Shows targeting reticles  

### **`UAutoplayDebugWidget`**
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

### **Player System**
- Provides health, ability cooldowns, targeting data  

### **NPC AI System**
- Provides enemy health values  
- Provides AI debug info  

### **GAS System**
- Provides cooldowns and attribute changes  

### **Player AI System**
- Provides autoplay debug info  

### **Multiplayer System**
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
- [ ] Player health updates correctly  
- [ ] Enemy health bars appear/disappear correctly  
- [ ] Ability cooldowns update correctly  
- [ ] Target indicators match ability behaviour  
- [ ] Debug UI toggles correctly  
- [ ] UI behaves correctly in multiplayer  

---

## **Future Extensions**
- Animated ability icons  
- Damage numbers  
- Minimap  
- Full menu system  
- Style/theme pass  