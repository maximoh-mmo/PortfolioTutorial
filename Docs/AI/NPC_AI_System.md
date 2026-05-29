# 📘 **NPC AI SYSTEM — SYSTEM DOCUMENT**  
**File:** `/Docs/AI/NPC_AI_System.md`

---

# **NPC AI System**

## **Purpose**
The NPC AI System controls enemy behaviour using:

 - **StateTrees** (via `UStateTreeComponent`) for high‑level logic  
- **AI Perception** for detecting the player  
- **[Group System](Group_System.md)** for assist behaviour  
- **[GAS](../GAS/GAS_System.md)** for combat abilities  

It provides responsive, deterministic, multiplayer‑safe enemy behaviour.

---

## **Responsibilities**
- Manage NPC behaviour states:
  - Idle  
  - Roam  
  - Agro  
  - Chase  
  - Attack  
  - Flee  
  - Assist  
- Handle perception events  
- Select targets  
- Trigger abilities (GAS)  
- Move using AI navigation  
- React to damage and death  
- Integrate with Group System for assist logic  

---

## **Non‑Responsibilities**
- Spawning (handled by [Spawner System](Spawner_System.md))  
- Pooling (handled by [Pooling System](Pooling_System.md))  
- Ability definitions (handled by [GAS System](../GAS/GAS_System.md))  
- UI (handled by [UI System](../Gameplay/UI_System.md))  
- Animation montages  
- Multiplayer replication (handled by engine + [GAS System](../GAS/GAS_System.md) · see [Multiplayer System](../Multiplayer/Multiplayer_System.md))  

---

## **Key Classes**

### **`AOnsetEnemy`** (inherits `AOnsetBaseCharacter` — shared base with `AOnsetPlayerCharacter`)
- Base NPC pawn (`Onset/Source/Onset/Public/Enemy/`)
- Holds ASC, AttributeSet *(future)*
- Holds `UGroupComponent`
- Stores a `UAIProfile` reference, read by the controller on possession
- `ApplyProfile(UAIProfile*)` applies profile-driven visual/config to the pawn:
  - **Skeletal mesh path** — loads `SkeletalMesh` synchronously, sets mesh + anim BP + material; auto-sizes the capsule to `GetImportedBounds()` so targeting (`ECC_Visibility` traces) hits the capsule regardless of physics assets
  - **Cube fallback path** — when the profile has no `SkeletalMesh`, creates a `UStaticMeshComponent` (CubeVis) with `/Engine/BasicShapes/Cube.Cube`, sets `QueryAndPhysics` + `ECR_Block` on all channels so the cube itself blocks targeting traces and physics movement
  - **Pool return** — called with `nullptr`; destroys any existing CubeVis via `FindComponentByClass` + `DestroyComponent`, clears skeletal mesh/anim/material, hides the actor

### **`AOnsetAIController`** (`Onset/Source/Onset/Public/AI/`)
- Data‑driven base controller — no hardcoded enemy/player logic
- Owns `UStateTreeComponent` and `UAIPerceptionComponent` as subobjects
- On `OnPossess`, reads the possessed pawn's `UAIProfile` and self‑configures:
  - Loads the profile's `StateTree` asset
  - Sets sight radius/angle and hearing range from the profile
- Includes authority guard — stops the StateTree on clients

### **`UAIProfile`** (`Onset/Source/Onset/Public/AI/AIProfile.h`)
- `UDataAsset` subclass — created per enemy type in-editor
- Contains: `SkeletalMesh`, `AnimBlueprintClass`, `OverrideMaterial`, `StateTreeAsset`, sight range/angle, hearing range, aggression, flee threshold, assist radius
- Designers create new enemy types without C++ changes
- All visual variation (mesh, animation, material) driven by profile — no Blueprint subclassing needed

### **`UNPCStateTreeSchema`** *(future)*
- Defines context data for the StateTree  

---

## **Key Functions (AOnsetAIController)**

### **`ApplyProfile(const UAIProfile*)`**
Reads the profile and self‑configures: loads the StateTree asset, sets perception configs, binds `OnPerceptionUpdated`.

### **`OnPerceptionUpdated(const TArray<AActor*>&)`** *(stub)*
Handles sight/hearing events — feeds data into StateTree context.

### **`SetTarget(AActor*)`** *(future)*
Assigns a target for chase/attack.

### **`OnDeath()`** *(future)*
Notifies spawner/pool.

---

## **StateTree Diagram**

```mermaid
stateDiagram-v2
    [*] --> Idle

    Idle --> Roam: Timer
    Roam --> Agro: Perception sees player
    Agro --> Chase: Target acquired
    Chase --> Attack: In range
    Attack --> Chase: Cooldown finished
    Chase --> Lost: Target lost
    Lost --> Roam

    Agro --> Assist: Group assist event
    Assist --> Chase

    Any --> Flee: Low health + isolated
    Flee --> Idle: Safe
```

## **Data Flow Diagram**

```mermaid
flowchart TD
    Profile[UAIProfile Data Asset] -->|OnPossess| AIC[AOnsetAIController]
    AIC -->|Configures| STComp[UStateTreeComponent]
    AIC -->|Configures| PComp[UAIPerceptionComponent]

    PE[Perception Event] --> PComp
    AE[Assist Event] --> STComp
    DE[Damage Event] --> STComp

    STComp --> Eval[StateTree Evaluators]
    Eval --> Cond[Conditions]
    Cond --> Trans[Transitions]
    Trans --> State[Behaviour State]
    State --> Move[Movement / Abilities / Facing]
```

---

## **Interactions With Other Systems**

### **[Group System](Group_System.md)**
- Receives assist events  
- Transitions into Agro  

### **[GAS](../GAS/GAS_System.md)**
- Executes abilities  
- Applies damage  
- Handles death  

### **[Spawner](Spawner_System.md) & [Pooling](Pooling_System.md)**
- Resets AI state  
- Reinitializes StateTree  

### **[Multiplayer](../Multiplayer/Multiplayer_System.md)**
- AI runs **server‑only**  
- Clients receive replicated movement + effects  

---

## **Replication Rules**
- AI logic runs **only on the server**  
- Target selection is server‑only  
- Movement is replicated  
- GAS handles ability replication  
- Perception is server‑only  

---

## **Edge Cases**
- NPC loses sight but is still in assist mode  
- NPC dies mid‑attack  
- NPC is pooled mid‑state  
- NPC target dies  
- Multiple assist events overlap  
- Player AI vs NPC AI interactions  

---

## **Testing Checklist**
- [ ] Perception triggers Agro  
- [ ] Assist events trigger Agro  
- [ ] Chase transitions correctly  
- [ ] Attack triggers GAS ability  
- [ ] Flee triggers at low health  
- [ ] NPC resets correctly when pooled  
- [ ] Works in multiplayer (server‑only AI)  

---

## **Future Extensions**
- EQS‑based positioning  
- Ranged enemies  
- Boss behaviours  
- Threat evaluation system  
- Cover system  

---