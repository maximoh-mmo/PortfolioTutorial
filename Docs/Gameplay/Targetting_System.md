# 📘 **Targeting System Document**  
**File:** `/Docs/Gameplay/TargetingSystem.md`

---

# **Targeting System**

## **Purpose**
Provide deterministic targeting for all actors using a **single authoritative target**, with PvP rules integrated into target validation.

---

## **Responsibilities**
- Maintain `CurrentTarget`  
- Manual targeting (player)  
- Automatic targeting (AI + fallback)  
- PvP‑aware target filtering  
- Provide target data for abilities  
- Drive target highlighting UI  

---

## **Target Selection Logic**

### **Player**
- Click enemy → set `CurrentTarget`  
- If no target → auto‑select nearest valid enemy  
- If PvP disabled → players are **not valid targets**  
- If PvP enabled → players become valid targets  

### **NPC**
- Always target players  
- PvP flag does **not** affect NPC behaviour  

---

## **Target Validation Rules**

### Valid target if:
- Target is alive  
- Target is within range  
- Target is visible (optional LOS)  
- **PvP rules allow targeting**  

### PvP Filtering:
```
if (Target is Player && !SourcePlayer->bIsPvPEnabled)
    return false;
```

---

## **Ability Targeting Rules**

### Single‑Target
Uses `CurrentTarget`  
PvP rules apply.

### PBAoE
Centered on caster  
Players inside AoE are ignored if PvP disabled.

### Target‑Centered AoE
Centered on `CurrentTarget`  
Fails if target invalid due to PvP.

### Directional
Direction = Caster → CurrentTarget  
If PvP disabled and target is player → ability fails.

---

## **Interactions**
- **[PvP System](PVP_System.md):** filters player targets  
- **[GAS System](../GAS/GAS_System.md):** validates damage  
- **[UI System](UI_System.md):** updates target highlight  
- **[Player System](../Player/Player_System.md):** sets target from input  

---

## **Edge Cases**
- Player toggles PvP OFF while targeting a player  
- AoE overlaps players when PvP disabled  
- Player AI must ignore players when PvP disabled  