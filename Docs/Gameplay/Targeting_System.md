# 📘 **Targeting System Document**  
**File:** `/Docs/Gameplay/Targeting_System.md`

---

# **Targeting System**

## **Purpose**
Provide deterministic targeting for all actors using a **single authoritative target**, with PvP rules integrated into target validation.

> **Episode order note:** The targeting system is built in Episode 5, after the Spawner (Episode 4) has placed NPCs in the world, so testing can begin immediately.

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

## **Non‑Responsibilities**
- Ability‑specific targeting rules (handled by [Ability Targeting System](../Gameplay/Ability_Targeting_System.md))  
- Damage calculations (handled by [GAS System](../GAS/GAS_System.md))  
- UI rendering (handled by [UI System](UI_System.md))  

---

## **Key Classes**
- **`UTargetingComponent`** — attached to player, maintains `CurrentTarget`, validates targets  

---

## **Targeting Flow Diagram**

```mermaid
flowchart TD
    Input[Player Input<br/>Mouse Click] --> Targeting
    AI[AI Target Selection] --> Targeting

    Targeting --> Validate[Validate Target<br/>Range, LOS, PvP Rules]
    Validate --> CurrentTarget[Set CurrentTarget]

    CurrentTarget --> AbilityData[Build Ability Target Data]
    AbilityData --> GAS[Gameplay Ability System]
```

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

---

## **Replication**
- Target selection is **client‑side**  
- Server validates target data when ability activates via GAS  

---

## **Testing Checklist**
- [ ] Click‑to‑target sets `CurrentTarget`  
- [ ] Auto‑target fallback selects nearest valid enemy  
- [ ] PvP filtering correctly excludes players when OFF  
- [ ] AI targeting respects PvP rules  
- [ ] Target highlight appears/disappears correctly  

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