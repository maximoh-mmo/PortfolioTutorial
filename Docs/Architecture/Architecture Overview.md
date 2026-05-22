# 🧩 **ARCHITECTURE OVERVIEW (PRIVATE DOCUMENT)**

## **High‑Level Architecture Overview**

This document describes the **major systems**, their responsibilities, and how they interact.

---

# **Top‑Level Architecture Diagram**

```
                   ┌──────────────────────────┐
                   │        Player            │
                   │  - Input (click move)    │
                   │  - Targeting             │
                   │  - GAS Abilities         │
                   └─────────────┬────────────┘
                                 │
                                 ▼
                   ┌──────────────────────────┐
                   │     Player AI (optional) │
                   │  - StateTree             │
                   │  - Auto-targeting        │
                   │  - Auto-abilities        │
                   └─────────────┬────────────┘
                                 │
                                 ▼
                   ┌──────────────────────────┐
                   │     Ability System       │
                   │  - GAS                  │
                   │  - Effects              │
                   │  - Cooldowns            │
                   └─────────────┬────────────┘
                                 │
                                 ▼
                   ┌──────────────────────────┐
                   │         NPC AI           │
                   │  - StateTree             │
                   │  - Perception            │
                   │  - Behaviour             │
                   └─────────────┬────────────┘
                                 │
                                 ▼
                   ┌──────────────────────────┐
                   │   Spawner + Pooling      │
                   │  - Group spawning        │
                   │  - Object pooling        │
                   │  - Respawn logic         │
                   └─────────────┬────────────┘
                                 │
                                 ▼
                   ┌──────────────────────────┐
                   │      Multiplayer         │
                   │  - Server authority      │
                   │  - Replication           │
                   │  - RPCs                  │
                   └─────────────┬────────────┘
                                 │
                                 ▼
                   ┌──────────────────────────┐
                   │         Steam            │
                   │  - Auth tickets          │
                   │  - Server registration   │
                   └──────────────────────────┘
```

---

## **System Responsibilities**

### **Player System**
- Handles input  
- Click‑to‑move  
- Click‑to‑target  
- Ability activation  
- Camera control  

### **Player AI System**
- Optional autoplay  
- Uses StateTree  
- Selects targets  
- Moves and attacks automatically  

### **NPC AI System**
- StateTree for high‑level behaviour  
- Perception for agro  
- Movement and combat logic  
- GAS abilities  

### **Spawner System**
- Creates groups of NPCs  
- Controls spawn points  
- Manages respawn timers  

### **Object Pooling System**
- Pre‑spawns NPCs  
- Recycles them  
- Resets state  
- Assigns enemy types  

### **Combat System (GAS)**
- Abilities  
- Effects  
- Cooldowns  
- Damage  
- Hit reactions  

### **Multiplayer System**
- Server‑authoritative simulation  
- Replication of NPCs, abilities, targeting  
- Dedicated server support  

### **Steam Integration**
- User authentication  
- Server registration  
- Session discovery  

---