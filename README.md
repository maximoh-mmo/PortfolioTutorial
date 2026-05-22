## 📘 **Private Unreal Project — Internal Repository**

This repository contains the **full private development version** of the Top‑Down ARPG AI Demo and the complete production pipeline for the **36‑episode tutorial series**.

It includes:

- The full Unreal Engine project  
- All internal documentation  
- All design materials  
- All planning for the tutorial series  
- Steam integration  
- Dedicated server configuration  
- Experimental systems  
- Episode export tools  

This repo is **not** intended for public release.

---

## 🧱 **Repository Structure**

### **/Project/**
The full Unreal Engine project — this is the *real game*, not the tutorial version.

Contains all systems:

- Top‑down player controller  
- Click‑to‑move  
- Click‑to‑target  
- GAS abilities  
- NPC AI (StateTree + Perception)  
- Object pooling  
- Spawner system  
- Player AI autoplay  
- Multiplayer support  
- Steam authentication  
- Dedicated server support  

---

### **/Docs/**
Internal technical documentation.

Includes:

- Architecture diagrams  
- AI behaviour flowcharts  
- StateTree schemas  
- GAS ability flow  
- Multiplayer authority diagrams  
- Pooling lifecycle  
- Steam auth flow  
- Player AI logic  

---

### **/Design/**
Game design documentation.

Includes:

- Ability design  
- Enemy types & behaviours  
- Player controls  
- Camera behaviour  
- UI mockups  
- Final demo flow  

---

### **/Planning/**
Tutorial series planning.

Includes:

- Episode outlines  
- Episode scripts  
- Talking points  
- Recording notes  
- Release plan  

---

### **/Series/**
Clean episode snapshots prepared for the **public repo**.

Each episode folder contains:

- Cleaned project snapshot  
- Episode README  
- Diagrams  
- Code snippets  

---

### **/Tools/**
Internal tools and automation.

Includes:

- Episode export scripts  
- Server build scripts  
- Debug utilities  
- Profiling tools  

---

### **/Server/**
Dedicated server configuration.

Includes:

- Config files  
- Launch scripts  
- Network emulation configs  
- Deployment notes  

---

### **/Steam/**
Steam integration (private).

Includes:

- Steamworks SDK  
- Auth ticket testing  
- Server registration scripts  
- Steam session testing  

---

### **/Experiments/**
Prototypes and throwaway tests.

Used for:

- AI experiments  
- Ability prototypes  
- Movement tests  
- UI mockups  
- Networking experiments  

---

### **/Assets/**
Non‑Unreal assets.

Includes:

- Diagrams  
- Images  
- Audio  
- Video  
- Reference material  

---

### **/Scripts/**
General automation scripts.

---

## 📖 **Documentation Index**

### **Architecture**
- [Architecture Overview](Docs/Architecture/Architecture%20Overview.md) — high-level system map

### **Core Systems**
- [Player System](Docs/Player/Player_System.md) — click-to-move, click-to-target, ability activation, PvP toggle
- [NPC AI System](Docs/AI/NPC_AI_System.md) — StateTree-driven enemy behaviour, perception, combat
- [Targeting System](Docs/Gameplay/Targeting_System.md) — deterministic target selection, PvP-aware filtering
- [Ability Targeting System](Docs/Gameplay/Ability_Targeting_System.md) — single-target, AoE, directional targeting
- [PvP System](Docs/Gameplay/PVP_System.md) — player-controlled PvP/PvE toggle, damage filtering
- [UI System](Docs/Gameplay/UI_System.md) — health bars, cooldowns, target highlighting, debug overlays

### **AI Systems**
- [NPC AI System](Docs/AI/NPC_AI_System.md) — enemy StateTree behaviour
- [Player AI System](Docs/AI/Player_AI_System.md) — autoplay/testing mode
- [Group System](Docs/AI/Group_System.md) — NPC group cohesion, assist behaviour
- [Spawner System](Docs/AI/Spawner_System.md) — NPC group creation, respawn logic
- [Pooling System](Docs/AI/Pooling_System.md) — NPC instance reuse for performance

### **Technical Systems**
- [GAS System](Docs/GAS/GAS_System.md) — abilities, effects, attributes, PvP damage filtering
- [Multiplayer System](Docs/Multiplayer/Multiplayer_System.md) — server authority, replication, dedicated server
- [Steam Integration System](Docs/Steam/Steam_Integration_System.md) — auth tickets, server verification

### **Planning & Tracking**
- [Series Overview](Planning/Outlines/Series_Overview.md) — tutorial series vision, structure, audience
- [Scope Overview](Planning/Outlines/Scope_Overview.md) — project boundaries, deliverables, success criteria
- [Episode List](Planning/Outlines/Episode_List.md) — all 36 episodes by phase

### **Workflow**
- [Branching Strategy](Planning/Workflow/BRANCHING_STRATEGY.md) — branch conventions, merge flow, rules
- [Episode Export Workflow](Planning/Workflow/EPISODE_EXPORT_WORKFLOW.md) — snapshot creation and export process
- [Public Release Checklist](Planning/Workflow/PUBLIC_RELEASE_CHECKLIST.md) — pre-push verification items

### **Templates**
- [System Documentation Template](Docs/Templates/SYSTEM_DOCUMENTATION_TEMPLATE.md) — reusable system doc structure
- [Episode Script Template](Planning/Scripts/EPISODE_SCRIPT_TEMPLATE.md) — episode script structure
- [Episode README Template](Planning/Templates/EPISODE_README_TEMPLATE.md) — public episode README structure

---

## 🔄 **Branching Strategy**

See the full [Branching Strategy document](Planning/Workflow/BRANCHING_STRATEGY.md) for merge flow and rules.

- **main** — stable full demo  
- **dev** — active development  
- **feature/*** — new systems  
- **fix/*** — bug fixes  
- **episode/*** — staging branches for public releases  
- **steam/*** — Steam integration  
- **server/*** — dedicated server work  

---

## 🚀 **Episode Export Workflow**

See the full [Episode Export Workflow](Planning/Workflow/EPISODE_EXPORT_WORKFLOW.md) for the complete step-by-step process and stripping guidelines.  

---

## 🔒 **Security Notes**

- Steam AppID and SDK must remain private  
- Dedicated server configs must remain private  
- No sensitive keys or tokens should ever be exported  

---

## 🎯 **Purpose of This Repository**

This repo is the **master source of truth** for:

- The full game demo  
- The tutorial series production pipeline  
- All internal documentation  
- All experiments and prototypes  

The public repo will contain only the **clean, step‑by‑step episode snapshots**.

---
