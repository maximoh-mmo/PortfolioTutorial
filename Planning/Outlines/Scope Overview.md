# 📘 **TOP‑DOWN ARPG AI DEMO — SCOPE OVERVIEW**

## **Purpose of This Document**
This Scope Overview defines the **boundaries**, **deliverables**, and **intended scale** of the Top‑Down ARPG AI Demo and the accompanying 36‑episode tutorial series.

It ensures:

- The project remains focused  
- The tutorial series stays coherent  
- The final demo is achievable, polished, and representative of real production workflows  

This document is the “contract” for what the project *is* and what it *is not*.

---

# 🎯 **Project Goal**
Build a **complete, multiplayer‑ready top‑down action RPG combat prototype** featuring:

- Click‑to‑move  
- Click‑to‑target  
- Player abilities (GAS)  
- Server‑authoritative NPC AI  
- Object pooling  
- Group spawning  
- StateTree‑driven behaviour  
- Player AI autoplay  
- Dedicated server support  
- Steam authentication  

The final result is a polished, self‑contained demo suitable for:

- Portfolio pieces  
- Technical showcases  
- AI/Gameplay engineering reference  
- Teaching material  

---

# 🧱 **Core Systems Included in Scope**

## **1. Player Systems**
- Top‑down camera  
- Point‑and‑click movement  
- Click‑to‑target selection  
- Basic attack  
- Multiple abilities (dash, AoE, projectile)  
- Ability targeting indicators  
- Player AI autoplay mode  

---

## **2. NPC Systems**
- NPC character class (C++ + BP)  
- StateTree‑driven behaviour  
- AI Perception (sight + hearing)  
- Behaviour states:
  - Idle  
  - Roam  
  - Agro  
  - Chase  
  - Attack  
  - Flee  
- Dynamic enemy types (mesh, stats, behaviour)  

---

## **3. Combat Systems (GAS)**
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

## **4. Spawning & Pooling**
- Enemy spawner actors  
- Group spawning  
- Object pooling for NPCs  
- Respawn logic  
- Group data (center, direction, alive count)  

---

## **5. Multiplayer**
- Server‑authoritative simulation  
- Replicated NPCs  
- Replicated abilities  
- Replicated targeting  
- Server‑only AI logic  
- Client‑side visuals  
- Dedicated server build  
- Local + remote testing  

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
- GAS debugger  
- On‑screen debug  
- Autoplay mode for automated testing  

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
- Inventory systems  
- Loot drops  
- Equipment or stats progression  
- Skill trees  
- Quests or narrative  
- Saving/loading  
- Complex animation blending  
- Advanced VFX or SFX polish  

### **AI**
- Advanced EQS‑driven tactics (beyond optional episode)  
- Navigation mesh generation at runtime  
- Squad‑level tactics  
- Behavior Tree‑only AI (StateTree is primary)  

### **Multiplayer**
- Matchmaking UI  
- Cross‑platform support  
- Voice chat  
- Anti‑cheat systems  

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
- 36 episode branches  
- Clean code snapshots  
- Episode READMEs  
- Final demo branch (optional)  

---

## **3. Tutorial Series Deliverables**
- 36 recorded episodes  
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
