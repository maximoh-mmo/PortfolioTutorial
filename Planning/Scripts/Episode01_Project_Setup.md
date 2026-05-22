# 🎬 **Episode 1 — Project Setup & Architecture Overview**

## **Episode Goal**
Set up the Unreal project, folder structure, C++ base classes, and explain the final demo.

---

## **Context & Dependencies**
- This is the first episode — no dependencies (see the [Episode List](../Outlines/Episode_List.md) for the full series roadmap).

---

## **High‑Level Summary**
We start from scratch: create a new Unreal project, set up the C++ classes that will be the foundation for the entire series, and walk through the full architecture. By the end, viewers understand what they're building and have a working project skeleton.

---

## **Key Concepts Introduced**
- Blank C++ project setup
- Project folder structure convention
- Base class hierarchy (Character, Controller, PlayerState, GameMode)
- High-level [architecture overview](../../Docs/Architecture/Architecture%20Overview.md) of all 13 systems
- The [36-episode road map](../Outlines/Episode_List.md)

---

## **Technical Breakdown**

### **1. Project Creation**
- Create new Blank C++ project via Unreal Editor
- Name: `MPTDARPG` (Multiplayer Top-Down ARPG)
- Choose appropriate project settings (scalability, target platform)

### **2. Folder Structure**
```
Project/
├── Source/MPTDARPG/
│   ├── Player/
│   ├── AI/
│   ├── Combat/
│   ├── Spawning/
│   └── Multiplayer/
├── Content/
│   ├── Characters/
│   ├── Abilities/
│   ├── UI/
│   └── Maps/
└── Config/
```

### **3. Base C++ Classes (Stubs)**
- `AMPTDARPGCharacter` — base character with movement component
- `AMPTDARPGPlayerController` — input handling stub
- `AMPTDARPGPlayerState` — player state stub
- `AMPTDARPGGameMode` — game mode stub
- `AMPTDARPGGameState` — game state stub

### **4. Architecture Walkthrough**
Show the [Architecture Overview](../../Docs/Architecture/Architecture%20Overview.md) diagram and explain how each system connects:
- [Player](../../Docs/Player/Player_System.md) → [Targeting](../../Docs/Gameplay/Targeting_System.md) → [GAS](../../Docs/GAS/GAS_System.md) → Attributes
- [NPC AI](../../Docs/AI/NPC_AI_System.md) ↔ [Group](../../Docs/AI/Group_System.md) ↔ [Spawner](../../Docs/AI/Spawner_System.md) ↔ [Pooling](../../Docs/AI/Pooling_System.md)
- [PvP](../../Docs/Gameplay/PVP_System.md) toggle as a cross-cutting rules layer
- [Multiplayer](../../Docs/Multiplayer/Multiplayer_System.md) + [Steam](../../Docs/Steam/Steam_Integration_System.md) as the outer authority layer

### **5. Testing**
- Open the project
- Verify it compiles
- Press Play in Editor — empty world with default pawn

---

## **Code Snippets**

```cpp
// AMPTDARPGCharacter.h — base character declaration
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MPTDARPGCharacter.generated.h"

UCLASS()
class AMPTDARPGCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AMPTDARPGCharacter();
};
```

```cpp
// AMPTDARPGCharacter.cpp — base character implementation
#include "MPTDARPGCharacter.h"

AMPTDARPGCharacter::AMPTDARPGCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
}
```

---

## **Diagrams**
```
Project Structure:
Source/MPTDARPG/
├── Player/       → Character, Controller, PlayerState
├── AI/           → NPC AI, Player AI
├── Combat/       → GAS, Targeting
├── Spawning/     → Spawner, Pooling, Groups
└── Multiplayer/  → GameMode, GameState, Steam
```

---

## **Common Pitfalls**
- Forgetting to generate project files after adding C++ classes
- Using the wrong project template (Blank, not Third Person or Top Down)
- Not having C++ compiler installed (VS 2022 with Game Development workload)

---

## **Episode Checklist**
- [ ] Project created and compiles
- [ ] Folder structure created
- [ ] Base C++ classes created and compiling
- [ ] Architecture overview explained on screen
- [ ] Tested in PIE

---

## **Public Repo Notes**
- This episode's snapshot is the clean foundation
- Remove any auto-generated content folders (StarterContent)
- Remove any default maps that aren't needed

---

## **Recording Script**

**Intro:**
"Welcome to the Top-Down ARPG AI series. Over the next 36 episodes we're going to build a complete multiplayer-ready combat demo from scratch in Unreal Engine, using C++, StateTrees, GAS, dedicated servers, and Steam integration. In this first episode, we set up the project and lay the foundation."

**Body:**
- Walk through the architecture overview document
- Create the project step by step
- Show the folder structure convention
- Stub out the base C++ classes
- Explain why each class exists

**Outro:**
"In [the next episode](Episode02_TopDown_Camera.md), we'll set up our top-down camera — the first real gameplay system."

---

## **Next Episode Preview**
"[Next time](Episode02_TopDown_Camera.md), we implement the top-down camera with smoothing and collision."
