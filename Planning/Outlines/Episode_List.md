# 📘 **TOP‑DOWN ARPG AI SERIES — FULL EPISODE LIST**

## **Overview**
This document contains the complete episode list for the 46‑episode tutorial series (see [Series Overview](Series_Overview.md) and [Scope Overview](Scope_Overview.md) for context).
Episodes are grouped into phases that reflect the natural progression of building the full demo.
The [Architecture Overview](../../Docs/Architecture/Architecture%20Overview.md) provides the full system design that this episode list implements.

Each episode includes:

- **Title**
- **One‑sentence goal**
- **Dependencies** (if any)
- **Script** — linked where drafted

This is the master outline for the entire series. Drafted scripts follow the [Episode Script Template](../Scripts/EPISODE_SCRIPT_TEMPLATE.md).

---

# **PHASE 0 — Player Core & Input**
### *Goal: Build a playable demo with input-agnostic movement, targeting, and enemies.*

---

### **Episode 1 — Project Setup & Enhanced Input Architecture**
Set up the Unreal project, folder structure, C++ base classes, and Enhanced Input system (Input Actions, Mapping Contexts for Touch/Desktop/Gamepad), and explain the final demo.
→ [Script](Episode01_Project_Setup.md) · [Architecture Overview](../../Docs/Architecture/Architecture%20Overview.md)

### **Episode 2 — Top‑Down Camera Setup**
Implement a fixed top‑down camera with smoothing and collision handling.
→ [Script](Episode02_TopDown_Camera.md)

### **Episode 3 — Movement System (Virtual Joystick + Tap-to-Move + WASD + Gamepad)**
Implement multi-device movement: touch virtual joystick + tap-to-move, mouse click-to-move, WASD keys, and gamepad left stick. Build the cursor abstraction layer (mouse cursor, touch pointer, gamepad R-Stick software cursor).
→ [Script](Episode03_Movement_System.md)

### **Episode 4 — Click‑to‑Target System**
Implement target selection (mouse + touch + gamepad cursor), highlighting, and basic attack input routing via Enhanced Input.
→ [Script](Episode04_ClickToTarget.md) · [Targeting System](../../Docs/Gameplay/Targeting_System.md)

### **Episode 5 — Enemy Spawner (C++)**
Create a spawner that generates groups of NPCs at defined points.
→ [Script](Episode05_Enemy_Spawner.md) · [Spawner System](../../Docs/AI/Spawner_System.md)

---

# **PHASE 1 — NPC Lifecycle**
### *Goal: Add pooling, groups, and respawn to the NPC pipeline.*

---

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

### **Episode 18a — Damage, Death, Pool Return, and Respawn**
Implement health depletion, death events, NPC pool return, and spawner respawn timers.
· [GAS System](../../Docs/GAS/GAS_System.md) · [Pooling System](../../Docs/AI/Pooling_System.md) · [Spawner System](../../Docs/AI/Spawner_System.md)

### **Episode 18b — Corpse Actor System**
Spawn a lightweight corpse actor on death; NPC returns to pool immediately. Timed despawn, corpse cap, and loot-container extension point.
· [Corpse System](../../Docs/AI/Corpse_System.md) · [Pooling System](../../Docs/AI/Pooling_System.md)

### **Episode 19 — Player Ability Targeting**
Add single‑target, AoE, and directional targeting with screen indicators (mouse/touch/gamepad).
· [Ability Targeting System](../../Docs/Gameplay/Ability_Targeting_System.md) · [UI System](../../Docs/Gameplay/UI_System.md)

### **Episode 20 — Adding Multiple Abilities**
Implement three new abilities: AoE (target-centered damage volume), Cone (directional frontal cone), and Shadowstep (passive on-kill teleport behind nearest enemy). Wire ability slots 1 and 2 to AoE and Cone, shadowstep triggers automatically. Add ability bar UI stub.
· [GAS System](../../Docs/GAS/GAS_System.md) · [UI System](../../Docs/Gameplay/UI_System.md)

---

# **PHASE 3.5 — Architecture Cleanup**
### *Goal: Refactor monolithic classes into focused components — a teaching moment in SOLID principles.*

---

### **Episode 21 — Architecture Cleanup: Interaction Component & Profile Split**
Extract interaction resolution from `AOnsetPlayerController` into `UInteractionComponent`, splitting the controller's responsibilities (Single Responsibility Principle). Split `UAIProfile` into `UVisualProfile`, `UAIProfile`, and `UPerceptionProfile` — three focused data assets instead of one god object. Retire `AOnsetPoolManager` in favour of `UOnsetPoolSubsystem` (subsystem pattern). Update all call sites (spawner, enemy, controller). The corpse mesh lands naturally in `UVisualProfile`. Teaches: SRP in UE5, subsystem vs. actor world management, data-oriented design for config assets.
· [Player System](../../Docs/Player/Player_System.md) · [NPC AI System](../../Docs/AI/NPC_AI_System.md) · [Pooling System](../../Docs/AI/Pooling_System.md) · [Spawner System](../../Docs/AI/Spawner_System.md)

### **Episode 22 — GAS Movement Speed Attribute**
Replace scattered direct `MaxWalkSpeed` manipulation in StateTree tasks with a central `MovementSpeed` GAS attribute and stackable GameplayEffects. Refactor FleeTask and InvestigateTask to apply/remove tagged GEs instead of caching and restoring speed directly. Adds stacking: stagger slow + flee speed + haste buff all combine naturally through the attribute.
· [GAS System](../../Docs/GAS/GAS_System.md) · [NPC AI System](../../Docs/AI/NPC_AI_System.md)

---

# **PHASE 3.6 — Player AI (Autoplay / Testing Mode)**
### *Goal: Allow the player character to be AI‑controlled for testing.*

---

### **Episode 23 — Player AI Controller (`AOnsetPlayerAIController`)**
Create an AI controller for the player pawn (`AOnsetPlayerAIController`, inherits `AAIController`), implement possession switching via `EnableAutoCombat()`/`DisableAutoCombat()`, and handle camera handover with `DelayedSetViewTarget`.
· [Player AI System](../../Docs/AI/Player_AI_System.md) · [Player System](../../Docs/Player/Player_System.md)

### **Episode 24 — Player StateTree (`PlayerAutoCombat`)**
Implement the StateTree asset using `UOnsetStateTreeSchema` (shared with NPCs). Tasks: `PlayerAcquireTargetTask` (navmesh-projected target selection with `IsAlive()` filter) and `PlayerEngageTask` (combined approach + attack, throttle-gated ability activation filtered by `TAG_Ability_Attack`).
· [Player AI System](../../Docs/AI/Player_AI_System.md)

### **Episode 25 — Autoplay Mode**
Add a toggle for AI control, debug UI, and AI‑vs‑AI testing.
· [Player AI System](../../Docs/AI/Player_AI_System.md) · [UI System](../../Docs/Gameplay/UI_System.md)

---

# **PHASE 3.7 — Threat System & AI Optimization**
### *Goal: Add threat-driven targeting, angular combat spread, and AI LOD.*

---

### **Episode 26 — Threat/Threat System**
Implement a server-side threat table (`UOnsetThreatSubsystem`) that drives NPC target selection. Damage dealt generates threat; NPCs target the highest-threat player. Enables future taunt/tank mechanics.
· [Threat System](../../Docs/AI/Threat_System.md) · [NPC AI System](../../Docs/AI/NPC_AI_System.md) · [GAS System](../../Docs/GAS/GAS_System.md)

### **Episode 27 — Attack Positioning with Angular Spread**
Replace random-lateral-offset with deterministic angular spread: divide 360° by attacker count, assign each NPC an angle based on threat rank. Prevents bunching around the player.
· [Threat System](../../Docs/AI/Threat_System.md) · [NPC AI System](../../Docs/AI/NPC_AI_System.md)

### **Episode 28 — AI LOD & Tick Management**
Add distance-based tick tiers to `AOnsetAIController`. Far NPCs tick at 0.5s intervals or stop entirely. StateTree, perception, and navigation respect the LOD tier.
· [Threat System](../../Docs/AI/Threat_System.md) · [NPC AI System](../../Docs/AI/NPC_AI_System.md)

---

# **PHASE 4 — Advanced AI Behaviour**
### *Goal: Add polish, group coordination, and complexity.*

---

### **Episode 29 — Flee State**
Implement low‑health retreat behaviour with evaluators.
· [NPC AI System](../../Docs/AI/NPC_AI_System.md)

### **Episode 30 — Group Assist Integration**
Wire assist events from the Group System into the StateTree so NPCs respond when nearby allies are attacked.
· [NPC AI System](../../Docs/AI/NPC_AI_System.md) · [Group System](../../Docs/AI/Group_System.md)

### **Episode 31 — Optional Behavior Tree Integration**
Add a small BT/EQS subtree for advanced chase/positioning.
· [NPC AI System](../../Docs/AI/NPC_AI_System.md)

### **Episode 32 — Dynamic Enemy Types**
Swap meshes, stats, and behaviour profiles when recycling NPCs.
· [Spawner System](../../Docs/AI/Spawner_System.md) · [Pooling System](../../Docs/AI/Pooling_System.md)

### **Episode 33 — AI Debugging Tools**
Use AIDebugger, StateTree debugger, GAS debugger, and on‑screen debug.
· [UI System](../../Docs/Gameplay/UI_System.md)

---

# **PHASE 5 — Multiplayer Support**
### *Goal: Make the entire system multiplayer‑safe.*

---

### **Episode 34 — Server/Client Architecture**
Explain authority, replication, RPCs, and server‑only logic.
· [Multiplayer System](../../Docs/Multiplayer/Multiplayer_System.md)

### **Episode 35 — Multiplayer‑Safe NPCs**
Replicate health, enemy type, and ensure server‑only AI.
· [Multiplayer System](../../Docs/Multiplayer/Multiplayer_System.md) · [NPC AI System](../../Docs/AI/NPC_AI_System.md)

### **Episode 36 — Multiplayer‑Safe Spawner & Pool**
Make spawner/pooling server‑only with replicated activation.
· [Multiplayer System](../../Docs/Multiplayer/Multiplayer_System.md) · [Spawner System](../../Docs/AI/Spawner_System.md)

### **Episode 37 — Dedicated Server Testing**
Run server + client, debug replication, and simulate latency.
· [Multiplayer System](../../Docs/Multiplayer/Multiplayer_System.md)

### **Episode 38 — PvP System**
Implement the PvP toggle with replicated state, targeting filtering, and damage-rule enforcement.
· [PvP System](../../Docs/Gameplay/PVP_System.md) · [Multiplayer System](../../Docs/Multiplayer/Multiplayer_System.md)

### **Episode 39 — Steam Integration**
Add Steam OSS, auth tickets, server verification, and testing.
· [Steam Integration System](../../Docs/Steam/Steam_Integration_System.md) · [Multiplayer System](../../Docs/Multiplayer/Multiplayer_System.md)

---

# **PHASE 5.5 — Player Persistence & Account System**
### *Goal: Add persistent player data, character slots, Steam ID extraction, and a full login → character select → enter world flow.*

---

### **Episode 40 — Database Architecture & Player Data Subsystem**
Design the `IPlayerDataStore` abstraction, implement the SQLite store with schema migrations, and create the `UOnsetPlayerDataSubsystem` world subsystem on the DS.
· [Account System](../../Docs/Player/Account_System.md) · [Persistence Data Store](../../Docs/Server/Persistence_Data_Store.md)

### **Episode 41 — Steam ID Resolution & Save/Load Flow**
Extract the numeric SteamID from the auth ticket via `SteamGameServer()->BeginAuthSession()`, store on `PlayerState`, and implement the full save/load RPC flow — account auto‑create on first login, load character data on select.
· [Account System](../../Docs/Player/Account_System.md) · [Steam Integration System](../../Docs/Steam/Steam_Integration_System.md)

### **Episode 42 — Lobby Map & Character Select UI**
Build a lightweight lobby map on the DS, implement a 3‑slot WBP character select screen, and wire create/select/pick flow. `ServerTravel` to the game map on ready.
· [Account System](../../Docs/Player/Account_System.md) · [UI System](../../Docs/Gameplay/UI_System.md)

### **Episode 43 — Full Login → Character Select → Enter World**
End‑to‑end verification: auth → account load → character select → spawn from saved state → auto‑save timer → save‑on‑disconnect. Production hardening with WAL mode, crash recovery, and migration tests.
· [Account System](../../Docs/Player/Account_System.md) · [Persistence Data Store](../../Docs/Server/Persistence_Data_Store.md)

---

# **PHASE 6 — Final Demo & Polish**
### *Goal: Deliver a polished, impressive final result.*

---

### **Episode 44 — UI & Feedback**
Add health bars, hit indicators, cooldown UI, virtual joystick widget, virtual ability buttons, gamepad cursor overlay, and target highlights.
· [UI System](../../Docs/Gameplay/UI_System.md) · [GAS System](../../Docs/GAS/GAS_System.md)

### **Episode 45 — Final Gameplay Loop**
Implement waves, respawn cycles, and full combat flow with all input methods supported.
· [Spawner System](../../Docs/AI/Spawner_System.md) · [Architecture Overview](../../Docs/Architecture/Architecture%20Overview.md)

### **Episode 46 — Performance Optimization**
Reduce ticks, add AI LOD, optimize pooling and networking.
· [Pooling System](../../Docs/AI/Pooling_System.md) · [Multiplayer System](../../Docs/Multiplayer/Multiplayer_System.md)

### **Episode 47 — Final Showcase**
Record the final demo, recap architecture, and discuss next steps.
· [Architecture Overview](../../Docs/Architecture/Architecture%20Overview.md)

---
