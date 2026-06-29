# 📘 **TOP‑DOWN ARPG AI DEMO — SCOPE OVERVIEW**

## **Purpose of This Document**
This Scope Overview defines the **boundaries**, **deliverables**, and **intended scale** of the Top‑Down ARPG AI Demo and the accompanying 46‑episode tutorial series (see [Series Overview](Series_Overview.md) and [Episode List](Episode_List.md)).

It ensures:

- The project remains focused  
- The tutorial series stays coherent  
- The final demo is achievable, polished, and representative of real production workflows  

This document is the “contract” for what the project *is* and what it *is not*.

---

# 🎯 **Project Goal**
Build a **complete, multiplayer‑ready top‑down action RPG combat prototype** featuring:

- Multi-device movement: virtual joystick (touch), tap-to-move (touch/mouse), WASD (keyboard), left stick (gamepad)  
- Tap/click‑to‑target (mouse + touch + gamepad cursor)  
- Player abilities (GAS)  
- Server‑authoritative NPC AI  
- Object pooling  
- Group spawning  
- StateTree‑driven behaviour  
- Player AI autoplay  
- Dedicated server support  
- Steam authentication  
- PvP toggle & friendly‑fire rules

The final result is a polished, self‑contained demo suitable for:

- Portfolio pieces  
- Technical showcases  
- AI/Gameplay engineering reference  
- Teaching material  

---

# 🧱 **Core Systems Included in Scope**

## **1. Player Systems**
- Top‑down camera  
- Multi-device movement: virtual joystick (touch), tap-to-move (touch/mouse), WASD (keyboard), left stick (gamepad)  
- Click‑to‑target selection via [Targeting System](../../Docs/Gameplay/Targeting_System.md)  
- Basic attack  
- Multiple abilities (basic attack, AoE, Cone, Shadowstep passive) via [GAS System](../../Docs/GAS/GAS_System.md)  
- Ability targeting indicators via [Ability Targeting System](../../Docs/Gameplay/Ability_Targeting_System.md)  
- [Player AI autoplay mode](../../Docs/AI/Player_AI_System.md)  
- [PvP toggle](../../Docs/Gameplay/PVP_System.md) — player‑controlled PvP/PvE mode affecting targeting and damage rules  

---

## **2. NPC Systems**
- [NPC character class with StateTree AI](../../Docs/AI/NPC_AI_System.md) (C++ + BP)  
- AI Perception (sight + hearing)  
- Behaviour states:
  - Idle  
  - Roam  
  - Agro  
  - Chase  
  - Attack  
  - Flee  
- Dynamic enemy types (mesh, stats, behaviour)
- [Assist behaviour via Group System](../../Docs/AI/Group_System.md): allies respond when a group member is attacked
- [Threat System](../../Docs/AI/Threat_System.md): threat-driven target selection, angular combat spread, AI LOD

---

## **3. Combat Systems ([GAS](../../Docs/GAS/GAS_System.md))**
- AbilitySystemComponent  
- AttributeSet (Health, Damage, etc.)  
- GameplayEffects  
- GameplayTags  
- Basic attack ability  
- Hit reaction ability  
- Ability cooldowns  
- Damage + death handling  
- Ability bar UI  

---

## **4. [Spawning](../../Docs/AI/Spawner_System.md) & [Pooling](../../Docs/AI/Pooling_System.md)**
- Enemy spawner actors  
- Group spawning  
- [Object pooling for NPCs](../../Docs/AI/Pooling_System.md) — two-tier architecture (AI actors + corpse actors)  
- Respawn logic  
- [Group data](../../Docs/AI/Group_System.md) (center, direction, alive count)  
- [Corpse Actor System](../../Docs/AI/Corpse_System.md) — world-debris persistence, timed cleanup, loot-container extension point  

---

## **5. [Multiplayer](../../Docs/Multiplayer/Multiplayer_System.md)**
- Server‑authoritative simulation  
- Replicated NPCs  
- Replicated abilities  
- Replicated targeting  
- Server‑only AI logic  
- Client‑side visuals  
- Dedicated server build  
- Local + remote testing  
- [Steam authentication](../../Docs/Steam/Steam_Integration_System.md)  

---

## **6. Steam Integration**
- Online Subsystem Steam  
- AppID 480 (Spacewar) for testing  
- Auth ticket generation  
- Server‑side ticket verification  
- Steam‑authenticated multiplayer sessions  

---

## **7. UI & Feedback**
- Health bars  
- Target highlighting  
- Ability cooldown UI  
- Hit indicators  
- Debug UI for AI + autoplay  

---

## **8. Debugging & Tools**
- AIDebugger  
- StateTree debugger  
- [GAS System](../../Docs/GAS/GAS_System.md) debugger  
- On‑screen debug  
- [Autoplay mode](../../Docs/AI/Player_AI_System.md) for automated testing  
- Mobile viewport testing  

---

## **9. Final Demo Loop**
- Waves of enemies  
- Combat encounters  
- Respawn cycles  
- Player abilities  
- AI behaviours  
- Multiplayer support  
- Steam authentication  

---

# 🚫 **Out‑of‑Scope Features**
These features are intentionally excluded to keep the project focused and achievable:

### **Gameplay**
- Inventory systems (stub JSON blobs ready, gameplay payload deferred)  
- Full loot drops (corpse system provides the container architecture; loot gameplay payload is out of scope)  
- Equipment or stats progression (stub JSON blobs ready, gameplay payload deferred)  
- Skill trees  
- Quests or narrative (stub JSON blobs ready, gameplay payload deferred)  
- Complex animation blending  
- Advanced VFX or SFX polish  

### **AI**
- Advanced EQS‑driven tactics (beyond optional episode)  
- [NPC AI System](../../Docs/AI/NPC_AI_System.md) receives assist events from the [Group System](../../Docs/AI/Group_System.md) and transitions into Agro.
- Navigation mesh generation at runtime  
- Squad‑level tactics  
- Behavior Tree‑only AI (StateTree is primary)  

### **Multiplayer**
- Matchmaking UI  
- Cross‑platform support  
- Voice chat  
- Anti‑cheat systems  

### **Persistence**
- Inventory, equipment, or quest gameplay (data structures and JSON blobs exist; gameplay payload deferred to future sprint)
- Cross-platform account merge  

### **Steam**
- Achievements  
- Leaderboards  
- Cloud saves  
- Workshop support  

### **Production**
- Full game polish  
- Marketplace‑ready assets  
- Mobile or console support  

This ensures the project remains a **focused technical demo**, not a full game.

---

# 📦 **Deliverables**

## **1. Private Repo Deliverables**
- Full Unreal project  
- All systems implemented  
- All documentation  
- All diagrams  
- All planning materials  
- All experiments  
- Steam + server configs  
- Episode scripts  
- Episode snapshots (cleaned)  

---

## **2. Public Repo Deliverables**
- 43 episode branches  
- Clean code snapshots  
- Episode READMEs  
- Final demo branch (optional)  

---

## **3. Tutorial Series Deliverables**

- 43 recorded episodes
- Supporting diagrams  
- Code snippets  
- Explanations and breakdowns  
- Final showcase video  

---

# 🧭 **Success Criteria**

The project is considered successful if:

- The final demo is stable, playable, and multiplayer‑ready  
- All systems are implemented cleanly and modularly  
- The tutorial series is coherent, paced, and technically accurate  
- The public repo is clean and easy to follow  
- The private repo contains complete documentation  
- The architecture is production‑grade and extensible  

---
