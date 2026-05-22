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

## 🔄 **Branching Strategy**

- **main** — stable full demo  
- **dev** — active development  
- **feature/*** — new systems  
- **episode/*** — staging branches for public releases  
- **steam/*** — Steam integration  
- **server/*** — dedicated server work  

---

## 🚀 **Episode Export Workflow**

1. Build the real system in `/Project`  
2. Create an `episode/*` branch  
3. Strip out advanced features  
4. Copy cleaned snapshot into `/Series/EpisodeXX`  
5. Add episode README + diagrams  
6. Push to public repo  

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
