# 📘 **SERIES OVERVIEW (PRIVATE DOCUMENT)**

## **Top‑Down ARPG AI Demo — Tutorial Series Overview**

### **Purpose**
This tutorial series teaches viewers how to build a **complete top‑down action RPG combat prototype** in Unreal Engine using **C++‑first architecture**, **StateTrees**, **[GAS System](../../Docs/GAS/GAS_System.md)**, **object pooling**, **[Multiplayer System](../../Docs/Multiplayer/Multiplayer_System.md)**, and **[Steam Integration System](../../Docs/Steam/Steam_Integration_System.md)**.

The series is designed to feel like a real studio workflow: modular, scalable, multiplayer‑safe, and production‑ready.

---

## **Final Demo Features**
The final demo includes:

### **Player**
- Top‑down camera  
- Multi-device movement: virtual joystick (touch), tap-to-move (touch/mouse), WASD (keyboard), left stick (gamepad)  
- Click‑to‑target combat (mouse + touch + gamepad cursor)  
- Basic attack + multiple abilities ([GAS System](../../Docs/GAS/GAS_System.md))  
- Ability targeting (single‑target, AoE, directional)  
- Optional AI‑controlled autoplay mode  
- [PvP toggle](../../Docs/Gameplay/PVP_System.md): player‑controlled setting that enables/disables friendly fire against other players

### **NPC Enemies**
- Server‑authoritative AI  
- StateTree‑driven behaviour  
- Idle → Roam → Agro → Chase → Attack → Flee  
- Group spawning  
- Object pooling (two-tier: AI actors + corpse actors)  
- Respawn logic  
- Dynamic enemy types (mesh, stats, behaviour)  
- Corpse actor system — lightweight world-debris on death, timed cleanup, loot extension point  

### **Combat**
- GAS abilities  
- GameplayEffects for damage, hit reactions, cooldowns  
- Animation montages  
- [Targeting system](../../Docs/Gameplay/Targeting_System.md)  
- Health bars and [UI feedback](../../Docs/Gameplay/UI_System.md)  

### **Multiplayer**
- Dedicated server authoritative simulation  
- Replicated NPCs  
- Replicated abilities  
- Replicated targeting  
- [Steam authentication](../../Docs/Steam/Steam_Integration_System.md)  
- Server browser support (optional)  

---

## **Series Structure**
The series is divided into **8 phases** (see the full [Episode List](Episode_List.md) for details):

0. **Player Core & Input** — Project setup, camera, multi-device movement, targeting, enemy spawner  
1. **NPC Lifecycle** — Pooling, groups, respawn  
2. **AI Foundations** — StateTrees, perception, behaviour  
3. **Combat (GAS)** — Abilities, damage, death  
3.5 **Architecture Cleanup** — Controller refactor, profile split, GAS movement speed attribute  
3.6 **Player AI** — Autoplay/testing mode  
4. **Advanced AI** — Flee, group assist, dynamic types, debugging  
5. **Multiplayer** — Replication, dedicated server, Steam, PvP  
6. **Final Demo & Polish** — UI, gameplay loop, optimization, showcase  

Total: **40 episodes** (Episode 18 split into 18a — Death/Pool/Respawn + 18b — Corpse Actor System). See the [Scope Overview](Scope_Overview.md) for project boundaries and the [Architecture Overview](../../Docs/Architecture/Architecture%20Overview.md) for system design.

---

## **Target Audience**
- Intermediate Unreal developers  
- C++‑curious Blueprint users  
- AI and gameplay programmers  
- Developers wanting to learn GAS  
- Anyone building multiplayer action games  

---

## **Prerequisites**
- Basic C++ knowledge  
- Basic Unreal familiarity  
- Understanding of gameplay programming concepts  

---

## **Learning Outcomes**
By the end of the series, viewers will understand:

- How to architect scalable gameplay systems  
- How to build AI using StateTrees  
- How to use GAS for abilities and combat  
- How to implement object pooling  
- How to build multiplayer‑safe systems  
- How to integrate Steam authentication  
- How to structure a real game project  
- How to support input-agnostic control schemes (touch, mouse, keyboard, gamepad)  

---
