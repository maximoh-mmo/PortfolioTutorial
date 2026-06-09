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
- The [38-episode road map](../Outlines/Episode_List.md)
- Enhanced Input system: Input Actions, Input Mapping Contexts, per-device key bindings
- Cursor abstraction layer (mouse, touch, gamepad R-Stick → unified screen position)

---

## **Technical Breakdown**

### **1. Project Creation**
- Create new Blank C++ project via Unreal Editor
- Name: `Onset`
- Choose appropriate project settings (scalability, target platform)

### **2. Folder Structure**
```
Project/
├── Source/Onset/
│   ├── Player/
│   ├── AI/
│   ├── Combat/
│   ├── AI/
│   └── Multiplayer/
├── Content/
│   ├── Characters/
│   ├── Abilities/
│   ├── UI/
│   └── Maps/
└── Config/
```

### **3. Base C++ Classes (Stubs)**
- `AOnsetBaseCharacter` — shared base for player and NPC characters, inherits `ACharacter`
- `AOnsetPlayerCharacter` — player character, inherits `AOnsetBaseCharacter`
- `AOnsetPlayerController` — input handling stub
- `AOnsetPlayerState` — player state stub
- `AOnsetGameMode` — game mode stub
- `AOnsetGameState` — game state stub

### **4. Enhanced Input Architecture**
All input for the project goes through UE5's Enhanced Input system. We create the Input Actions and Mapping Contexts here so every future episode can reference them directly:

**Input Actions** (defined in C++ or via Data Assets):
- `IA_Move` (Axis2D) — virtual joystick, WASD, gamepad L-Stick
- `IA_Cursor` (Axis2D) — gamepad R-Stick cursor emulation
- `IA_Primary` (Digital) — tap, left-click, R-Stick click, A button — primary interaction, context resolves move/attack/interact
- `IA_Ability1-4` (Digital) — number keys, virtual buttons, face buttons
- `IA_PvPToggle` (Digital) — P key, virtual button, D-pad down

**Input Mapping Contexts** (priority-ordered per device):
- `IMC_Touch` (priority 0) — virtual joystick + tap + virtual buttons
- `IMC_Desktop` (priority 1) — mouse clicks + WASD + number keys
- `IMC_Gamepad` (priority 0) — L-Stick movement + R-Stick cursor + button abilities

**Cursor Abstraction** — a `UCursorManager` component provides the active cursor screen position regardless of source:
- Mouse: `GetMousePosition()`
- Touch: last touch event location
- Gamepad: accumulated R-Stick delta (software cursor)

All downstream raycasts (click-to-move, targeting, ability targeting) read from this single source.

### **5. Architecture Walkthrough**
Show the [Architecture Overview](../../Docs/Architecture/Architecture%20Overview.md) diagram and explain how each system connects:
- [Player](../../Docs/Player/Player_System.md) → [Targeting](../../Docs/Gameplay/Targeting_System.md) → [GAS](../../Docs/GAS/GAS_System.md) → Attributes
- [NPC AI](../../Docs/AI/NPC_AI_System.md) ↔ [Group](../../Docs/AI/Group_System.md) ↔ [Spawner](../../Docs/AI/Spawner_System.md) ↔ [Pooling](../../Docs/AI/Pooling_System.md)
- [PvP](../../Docs/Gameplay/PVP_System.md) toggle as a cross-cutting rules layer
- [Multiplayer](../../Docs/Multiplayer/Multiplayer_System.md) + [Steam](../../Docs/Steam/Steam_Integration_System.md) as the outer authority layer

### **6. Testing**
- Open the project
- Verify it compiles
- Press Play in Editor — empty world with default pawn

---

## **Code Snippets**

```cpp
// AOnsetBaseCharacter.h — shared character base for player and NPC
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AOnsetBaseCharacter.generated.h"

UCLASS()
class AOnsetBaseCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AOnsetBaseCharacter();
};
```

```cpp
// AOnsetBaseCharacter.cpp — shared character base implementation
#include "AOnsetBaseCharacter.h"

AOnsetBaseCharacter::AOnsetBaseCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
}
```

```cpp
// AOnsetPlayerCharacter.h — player character inherits AOnsetBaseCharacter
#pragma once

#include "CoreMinimal.h"
#include "Player/AOnsetBaseCharacter.h"
#include "OnsetPlayerCharacter.generated.h"

UCLASS()
class AOnsetPlayerCharacter : public AOnsetBaseCharacter
{
    GENERATED_BODY()

public:
    AOnsetPlayerCharacter();
};
```

```cpp
// AOnsetPlayerCharacter.cpp — player character implementation
#include "Player/OnsetPlayerCharacter.h"

AOnsetPlayerCharacter::AOnsetPlayerCharacter()
{
    // Player-specific setup here
}
```

---

## **Diagrams**
```
Project Structure:
Source/Onset/
├── Player/       → BaseCharacter, PlayerCharacter, Controller, PlayerState
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
- Not adding `EnhancedInput` to `PublicDependencyModuleNames` in `.Build.cs`
- Forgetting to push Mapping Contexts onto the subsystem in PlayerController `BeginPlay`

---

## **Episode Checklist**
- [ ] Project created and compiles
- [ ] Folder structure created
- [ ] Base C++ classes created (AOnsetBaseCharacter, AOnsetPlayerCharacter, PC, PS, GM, GS) and compiling
- [ ] Enhanced Input plugin enabled and `.Build.cs` updated
- [ ] All Input Actions created (IA_Move, IA_Cursor, IA_Primary, IA_Ability1-4, IA_PvPToggle)
- [ ] All Mapping Contexts created (IMC_Touch, IMC_Desktop, IMC_Gamepad)
- [ ] Cursor Manager component created
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
"Welcome to the Top-Down ARPG AI series. Over the next 38 episodes we're going to build a complete multiplayer-ready combat demo from scratch in Unreal Engine, using C++, StateTrees, GAS, dedicated servers, and Steam integration. In this first episode, we set up the project and lay the foundation."

**Body:**
- Walk through the architecture overview document
- Create the project step by step
- Show the folder structure convention
- Stub out the base C++ classes
- Set up Enhanced Input: explain the device-agnostic input philosophy
- Create all Input Actions and Mapping Contexts
- Build the Cursor Manager abstraction
- Explain why each class and input asset exists

**Outro:**
"In [the next episode](Episode02_TopDown_Camera.md), we'll set up our top-down camera — the first real gameplay system."

---

## **Next Episode Preview**
"[Next time](Episode02_TopDown_Camera.md), we implement the top-down camera with smoothing and collision."
