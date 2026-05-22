# 📘 **TOP‑DOWN ARPG AI SERIES — FULL EPISODE LIST**

## **Overview**
This document contains the complete episode list for the 36‑episode tutorial series.  
Episodes are grouped into phases that reflect the natural progression of building the full demo.

Each episode includes:

- **Title**  
- **One‑sentence goal**  
- **Dependencies** (if any)

This is the master outline for the entire series.

---

# **PHASE 0 — Project Foundations (Top‑Down Player Core)**  
### *Goal: Build the player, camera, input, and basic combat foundation.*

---

### **Episode 1 — Project Setup & Architecture Overview**  
Set up the Unreal project, folder structure, C++ base classes, and explain the final demo.

### **Episode 2 — Top‑Down Camera Setup**  
Implement a fixed top‑down camera with smoothing and collision handling.

### **Episode 3 — Point‑and‑Click Movement**  
Use mouse raycasts and MoveToLocation to implement click‑to‑move.

### **Episode 4 — Click‑to‑Target System**  
Implement target selection, highlighting, and basic attack input routing.

---

# **PHASE 1 — NPC Lifecycle: Spawning, Pooling, Groups**  
### *Goal: Build real game‑ready NPC lifecycle systems before adding AI.*

---

### **Episode 5 — Enemy Spawner (C++)**  
Create a spawner that generates groups of NPCs at defined points.

### **Episode 6 — Object Pooling System**  
Implement NPC pooling for efficient reuse and performance.

### **Episode 7 — Group Data System**  
Track group center, direction, alive count, and expose this to NPCs.

### **Episode 8 — Respawn Logic**  
Return NPCs to the pool on death and respawn them with new types.

---

# **PHASE 2 — AI Foundations (StateTrees + Perception)**  
### *Goal: Build the high‑level AI brain.*

---

### **Episode 9 — StateTree Setup in C++**  
Add a StateTreeComponent, create the schema, and bind context data.

### **Episode 10 — Idle & Roam States**  
Implement idle timers, Brownian motion roaming, and group cohesion.

### **Episode 11 — AI Perception (C++)**  
Add sight/hearing, handle perception events, and feed data into the StateTree.

### **Episode 12 — Agro State**  
Detect the player, face the target, and transition into chase.

### **Episode 13 — Chase State**  
Implement MoveToActor, distance checks, and lost‑target logic.

---

# **PHASE 3 — Combat System (GAS)**  
### *Goal: Add real combat behaviour using GAS.*

---

### **Episode 14 — GAS Setup (C++)**  
Create the AbilitySystemComponent, AttributeSet, and GameplayTags.

### **Episode 15 — Basic Attack Ability (Player + NPC)**  
Implement a basic attack ability for both player and NPCs.

### **Episode 16 — Hit Reaction Ability**  
Add hitstop, stagger, and GameplayEffects for reactions.

### **Episode 17 — NPC Attack State**  
Trigger GA_Attack from the StateTree with cooldowns and transitions.

### **Episode 18 — Damage, Death, and Cleanup**  
Implement health, death events, and notify the spawner/pool.

### **Episode 19 — Player Ability Targeting**  
Add single‑target, AoE, and directional targeting with cursor indicators.

### **Episode 20 — Adding Multiple Abilities**  
Implement dash, AoE, projectile abilities, and ability bar UI.

---

# **PHASE 3.5 — Player AI (Autoplay / Testing Mode)**  
### *Goal: Allow the player character to be AI‑controlled for testing.*

---

### **Episode 21 — Player AI Controller**  
Create a PlayerAIController and implement possession switching.

### **Episode 22 — Player StateTree**  
Implement auto‑targeting, auto‑movement, and auto‑ability usage.

### **Episode 23 — Autoplay Mode**  
Add a toggle for AI control, debug UI, and AI‑vs‑AI testing.

---

# **PHASE 4 — Advanced AI Behaviour**  
### *Goal: Add polish and complexity.*

---

### **Episode 24 — Flee State**  
Implement low‑health retreat behaviour with evaluators.

### **Episode 25 — Optional Behavior Tree Integration**  
Add a small BT/EQS subtree for advanced chase/positioning.

### **Episode 26 — Dynamic Enemy Types**  
Swap meshes, stats, and behaviour profiles when recycling NPCs.

### **Episode 27 — AI Debugging Tools**  
Use AIDebugger, StateTree debugger, GAS debugger, and on‑screen debug.

---

# **PHASE 5 — Multiplayer Support**  
### *Goal: Make the entire system multiplayer‑safe.*

---

### **Episode 28 — Server/Client Architecture**  
Explain authority, replication, RPCs, and server‑only logic.

### **Episode 29 — Multiplayer‑Safe NPCs**  
Replicate health, enemy type, and ensure server‑only AI.

### **Episode 30 — Multiplayer‑Safe Spawner & Pool**  
Make spawner/pooling server‑only with replicated activation.

### **Episode 31 — Dedicated Server Testing**  
Run server + client, debug replication, and simulate latency.

### **Episode 32 — Steam Integration**  
Add Steam OSS, auth tickets, server verification, and testing.

---

# **PHASE 6 — Final Demo & Polish**  
### *Goal: Deliver a polished, impressive final result.*

---

### **Episode 33 — Final Gameplay Loop**  
Implement waves, respawn cycles, and full combat flow.

### **Episode 34 — UI & Feedback**  
Add health bars, hit indicators, cooldown UI, and target highlights.

### **Episode 35 — Performance Optimization**  
Reduce ticks, add AI LOD, optimize pooling and networking.

### **Episode 36 — Final Showcase**  
Record the final demo, recap architecture, and discuss next steps.

---
