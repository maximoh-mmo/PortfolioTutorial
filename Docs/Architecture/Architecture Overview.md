# 🧩 **ARCHITECTURE OVERVIEW**

## **High‑Level Architecture**
The project is composed of several modular systems that interact through clean, well‑defined boundaries:

- **Player System**  
- **NPC AI System**  
- **StateTree System**  
- **Targeting System**  
- **GAS (Gameplay Ability System)**  
- **Attribute System**  
- **Spawner System**  
- **Pooling System**  
- **Group System**  
- **PvP System (NEW)**  
- **UI System**  
- **Multiplayer System**  
- **Steam Integration System**

Each system is responsible for a specific domain and communicates with others through explicit data flows.

---

# 🧱 **Architecture Diagram (Text Version)**

```
                   ┌──────────────────────────┐
                   │        UI System         │
                   │  (HUD, PvP Toggle, etc.) │
                   └─────────────┬────────────┘
                                 │
                                 ▼
                      ┌──────────────────┐
                      │ PlayerController │
                      │  (Input + UI)    │
                      └───────┬──────────┘
                              │
                              ▼
                     ┌──────────────────┐
                     │  PlayerState     │◄──────────────┐
                     │ bIsPvPEnabled    │               │
                     └───────┬──────────┘               │
                             │                          │
                             ▼                          │
                   ┌──────────────────────┐             │
                   │ Targeting System     │             │
                   │ (PvP-aware filtering)│             │
                   └──────────┬───────────┘             │
                              │                         │
                              ▼                         │
                     ┌──────────────────┐               │
                     │      GAS         │               │
                     │ (Damage Exec)    │               │
                     └────────┬─────────┘               │
                              │                         │
                              ▼                         │
                     ┌──────────────────┐               │
                     │ Attribute System │               │
                     └──────────────────┘               │
                                                        │
                                                        │
                                                        ▼
                                            ┌──────────────────────────┐
                                            │ Multiplayer System       │
                                            │ (Server authority, RPCs) │
                                            └──────────────────────────┘
```

---

# 🧩 **Updated System Interactions (PvP Included)**

## **Player System → PvP System**
- UI toggle triggers `Server_SetPvPEnabled`  
- PlayerState stores and replicates PvP flag  

## **PvP System → Targeting System**
- TargetingComponent filters out player actors when PvP is OFF  
- Auto‑target fallback ignores players when PvP is OFF  

## **PvP System → GAS System**
- Damage execution checks PvP flag  
- Blocks player→player damage when PvP is OFF  

## **PvP System → UI System**
- UI displays PvP status  
- UI updates on `OnRep_PvPEnabled`  

## **PvP System → Player AI System**
- Player AI ignores players when PvP is OFF  
- Player AI may target players when PvP is ON  

## **PvP System → Multiplayer System**
- PvP flag is server‑authoritative  
- Replicated to all clients  
- Cannot be spoofed client‑side  

---

# 🧠 **Updated Architecture Overview (Full Document)**

Below is the **complete updated Architecture Overview**, rewritten to include the PvP System cleanly and consistently.

---

# 📘 **TOP‑DOWN ARPG — ARCHITECTURE OVERVIEW**

## **Purpose**
Define the high‑level architecture of the Top‑Down ARPG AI Demo, including all major systems, their responsibilities, and how they interact.  
This document is the technical map for the entire project.

---

# 🧱 **Core Systems**

### **1. Player System**
- Input handling  
- Click‑to‑move  
- Click‑to‑target  
- Ability activation  
- PvP toggle UI → PlayerState  
- Autoplay handoff  

### **2. NPC AI System**
- StateTree‑driven behaviour  
- Perception (sight/hearing)  
- Target selection  
- Combat behaviour  
- Assist logic via Group System  

### **3. StateTree System**
- High‑level behaviour logic  
- Evaluators (distance, health, perception)  
- Tasks (MoveTo, Attack, Flee)  
- Transitions  

### **4. Targeting System (PvP‑aware)**
- Maintains `CurrentTarget`  
- Manual + automatic targeting  
- PvP filtering  
- Provides target data to abilities  

### **5. GAS System**
- Ability execution  
- GameplayEffects  
- Cooldowns  
- Damage calculation  
- PvP damage filtering  

### **6. Attribute System**
- Health, MaxHealth, Damage  
- Replication  
- Death triggers  

### **7. Spawner System**
- Group spawning  
- Respawn logic  
- Enemy type assignment  

### **8. Pooling System**
- NPC reuse  
- Resetting state on reuse  

### **9. Group System**
- Group membership  
- Group center/direction  
- Assist behaviour  

### **10. PvP System (NEW)**
- Player‑controlled PvP toggle  
- Stored in PlayerState  
- Filters targeting  
- Filters damage  
- Replicated to all clients  

### **11. UI System**
- HUD  
- Ability bar  
- Health bars  
- Target highlight  
- PvP toggle  

### **12. Multiplayer System**
- Server‑authoritative simulation  
- RPCs  
- Replication  
- Dedicated server support  

### **13. Steam Integration System**
- Auth tickets  
- Server registration  
- Steam‑authenticated sessions  

---

# 🔗 **System Interaction Summary (PvP Included)**

### **Player ↔ PvP System**
- UI toggles PvP  
- PlayerState replicates PvP flag  

### **PvP System ↔ Targeting**
- Filters player targets  

### **PvP System ↔ GAS**
- Blocks player→player damage  

### **PvP System ↔ UI**
- UI updates PvP status  

### **PvP System ↔ Multiplayer**
- Server enforces PvP rules  
- Replicates PvP flag  

---

# 🎯 **Final Notes**
The PvP System is intentionally lightweight and modular.  
It does not complicate AI, spawning, pooling, or StateTrees — it simply adds a **rules layer** on top of targeting and damage.

This keeps the architecture clean, predictable, and multiplayer‑safe.