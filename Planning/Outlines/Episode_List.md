# 📘 **TOP‑DOWN ARPG AI SERIES — FULL EPISODE LIST**

## **Overview**
This document contains the complete episode list for the 36‑episode tutorial series (see [Series Overview](Series_Overview.md) and [Scope Overview](Scope_Overview.md) for context).  
Episodes are grouped into phases that reflect the natural progression of building the full demo.  
The [Architecture Overview](../../Docs/Architecture/Architecture%20Overview.md) provides the full system design that this episode list implements.

Each episode includes:

- **Title**  
- **One‑sentence goal**  
- **Dependencies** (if any)
- **Script** — linked where drafted

This is the master outline for the entire series. Drafted scripts follow the [Episode Script Template](../Scripts/EPISODE_SCRIPT_TEMPLATE.md).

---

# **PHASE 0 — Project Foundations (Top‑Down Player Core)**  
### *Goal: Build the player, camera, input, and basic combat foundation.*

---

### **Episode 1 — Project Setup & Architecture Overview**  
Set up the Unreal project, folder structure, C++ base classes, and explain the final demo.  
→ [Script](../Scripts/Episode01_Project_Setup.md) · [Architecture Overview](../../Docs/Architecture/Architecture%20Overview.md)

### **Episode 2 — Top‑Down Camera Setup**  
Implement a fixed top‑down camera with smoothing and collision handling.  
→ [Script](../Scripts/Episode02_TopDown_Camera.md)

### **Episode 3 — Point‑and‑Click Movement**  
Use mouse raycasts and MoveToLocation to implement click‑to‑move.  
→ [Script](../Scripts/Episode03_ClickToMove.md)

### **Episode 4 — Enemy Spawner (C++)**  
Create a spawner that generates groups of NPCs at defined points.  
→ [Script](../Scripts/Episode04_Enemy_Spawner.md) · [Spawner System](../../Docs/AI/Spawner_System.md)

---

# **PHASE 1 — NPC Lifecycle: Spawning, Pooling, Groups**  
### *Goal: Build real game‑ready NPC lifecycle systems before adding AI.*

---

### **Episode 5 — Click‑to‑Target System**  
Implement target selection, highlighting, and basic attack input routing.  
→ [Script](../Scripts/Episode05_ClickToTarget.md) · [Targeting System](../../Docs/Gameplay/Targeting_System.md)

### **Episode 6 — Object Pooling System**  
Implement NPC pooling for efficient reuse and performance.  
· [Pooling System](../../Docs/AI/Pooling_System.md)

### **Episode 7 — Group Data System**  
Track group center, direction, alive count, and expose this to NPCs.  
· [Group System](../../Docs/AI/Group_System.md)

### **Episode 8 — Respawn Logic**  
Return NPCs to the pool on death and respawn them with new types.  
· [Spawner System](../../Docs/AI/Spawner_System.md) · [Pooling System](../../Docs/AI/Pooling_System.md)

---

# **PHASE 2 — AI Foundations (StateTrees + Perception)**  
### *Goal: Build the high‑level AI brain.*

---

### **Episode 9 — StateTree Setup in C++**  
Add a StateTreeComponent, create the schema, and bind context data.  
· [NPC AI System](../../Docs/AI/NPC_AI_System.md)

### **Episode 10 — Idle & Roam States**  
Implement idle timers, Brownian motion roaming, and group cohesion.  
· [Group System](../../Docs/AI/Group_System.md)

### **Episode 11 — AI Perception (C++)**  
Add sight/hearing, handle perception events, and feed data into the StateTree.  
· [NPC AI System](../../Docs/AI/NPC_AI_System.md)

### **Episode 12 — Agro State**  
Detect the player, face the target, and transition into chase.  
· [NPC AI System](../../Docs/AI/NPC_AI_System.md)

### **Episode 13 — Chase State**  
Implement MoveToActor, distance checks, and lost‑target logic.  
· [NPC AI System](../../Docs/AI/NPC_AI_System.md)

---

# **PHASE 3 — Combat System (GAS)**  
### *Goal: Add real combat behaviour using GAS.*

---

### **Episode 14 — GAS Setup (C++)**  
Create the AbilitySystemComponent, AttributeSet, and GameplayTags.  
· [GAS System](../../Docs/GAS/GAS_System.md)

### **Episode 15 — Basic Attack Ability (Player + NPC)**  
Implement a basic attack ability for both player and NPCs.  
· [GAS System](../../Docs/GAS/GAS_System.md)

### **Episode 16 — Hit Reaction Ability**  
Add hitstop, stagger, and GameplayEffects for reactions.  
· [GAS System](../../Docs/GAS/GAS_System.md)

### **Episode 17 — NPC Attack State**  
Trigger GA_Attack from the StateTree with cooldowns and transitions.  
· [NPC AI System](../../Docs/AI/NPC_AI_System.md) · [GAS System](../../Docs/GAS/GAS_System.md)

### **Episode 18 — Damage, Death, and Cleanup**  
Implement health, death events, and notify the spawner/pool.  
· [GAS System](../../Docs/GAS/GAS_System.md) · [Pooling System](../../Docs/AI/Pooling_System.md)

### **Episode 19 — Player Ability Targeting**  
Add single‑target, AoE, and directional targeting with cursor indicators.  
· [Ability Targeting System](../../Docs/Gameplay/Ability_Targeting_System.md) · [UI System](../../Docs/Gameplay/UI_System.md)

### **Episode 20 — Adding Multiple Abilities**  
Implement dash, AoE, projectile abilities, and ability bar UI.  
· [GAS System](../../Docs/GAS/GAS_System.md) · [UI System](../../Docs/Gameplay/UI_System.md)

---

# **PHASE 3.5 — Player AI (Autoplay / Testing Mode)**  
### *Goal: Allow the player character to be AI‑controlled for testing.*

---

### **Episode 21 — Player AI Controller**  
Create a PlayerAIController and implement possession switching.  
· [Player AI System](../../Docs/AI/Player_AI_System.md) · [Player System](../../Docs/Player/Player_System.md)

### **Episode 22 — Player StateTree**  
Implement auto‑targeting, auto‑movement, and auto‑ability usage.  
· [Player AI System](../../Docs/AI/Player_AI_System.md)

### **Episode 23 — Autoplay Mode**  
Add a toggle for AI control, debug UI, and AI‑vs‑AI testing.  
· [Player AI System](../../Docs/AI/Player_AI_System.md) · [UI System](../../Docs/Gameplay/UI_System.md)

---

# **PHASE 4 — Advanced AI Behaviour**  
### *Goal: Add polish and complexity.*

---

### **Episode 24 — Flee State**  
Implement low‑health retreat behaviour with evaluators.  
· [NPC AI System](../../Docs/AI/NPC_AI_System.md)

### **Episode 25 — Optional Behavior Tree Integration**  
Add a small BT/EQS subtree for advanced chase/positioning.  
· [NPC AI System](../../Docs/AI/NPC_AI_System.md)

### **Episode 26 — Dynamic Enemy Types**  
Swap meshes, stats, and behaviour profiles when recycling NPCs.  
· [Spawner System](../../Docs/AI/Spawner_System.md) · [Pooling System](../../Docs/AI/Pooling_System.md)

### **Episode 27 — AI Debugging Tools**  
Use AIDebugger, StateTree debugger, GAS debugger, and on‑screen debug.  
· [UI System](../../Docs/Gameplay/UI_System.md)

---

# **PHASE 5 — Multiplayer Support**  
### *Goal: Make the entire system multiplayer‑safe.*

---

### **Episode 28 — Server/Client Architecture**  
Explain authority, replication, RPCs, and server‑only logic.  
· [Multiplayer System](../../Docs/Multiplayer/Multiplayer_System.md)

### **Episode 29 — Multiplayer‑Safe NPCs**  
Replicate health, enemy type, and ensure server‑only AI.  
· [Multiplayer System](../../Docs/Multiplayer/Multiplayer_System.md) · [NPC AI System](../../Docs/AI/NPC_AI_System.md)

### **Episode 30 — Multiplayer‑Safe Spawner & Pool**  
Make spawner/pooling server‑only with replicated activation.  
· [Multiplayer System](../../Docs/Multiplayer/Multiplayer_System.md) · [Spawner System](../../Docs/AI/Spawner_System.md)

### **Episode 31 — Dedicated Server Testing**  
Run server + client, debug replication, and simulate latency.  
· [Multiplayer System](../../Docs/Multiplayer/Multiplayer_System.md)

### **Episode 32 — Steam Integration**  
Add Steam OSS, auth tickets, server verification, and testing.  
· [Steam Integration System](../../Docs/Steam/Steam_Integration_System.md) · [Multiplayer System](../../Docs/Multiplayer/Multiplayer_System.md)

---

# **PHASE 6 — Final Demo & Polish**  
### *Goal: Deliver a polished, impressive final result.*

---

### **Episode 33 — Final Gameplay Loop**  
Implement waves, respawn cycles, and full combat flow.  
· [Spawner System](../../Docs/AI/Spawner_System.md) · [Architecture Overview](../../Docs/Architecture/Architecture%20Overview.md)

### **Episode 34 — UI & Feedback**  
Add health bars, hit indicators, cooldown UI, and target highlights.  
· [UI System](../../Docs/Gameplay/UI_System.md) · [GAS System](../../Docs/GAS/GAS_System.md)

### **Episode 35 — Performance Optimization**  
Reduce ticks, add AI LOD, optimize pooling and networking.  
· [Pooling System](../../Docs/AI/Pooling_System.md) · [Multiplayer System](../../Docs/Multiplayer/Multiplayer_System.md)

### **Episode 36 — Final Showcase**  
Record the final demo, recap architecture, and discuss next steps.  
· [Architecture Overview](../../Docs/Architecture/Architecture%20Overview.md)

---
