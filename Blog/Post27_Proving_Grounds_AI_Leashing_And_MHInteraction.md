# 🎬 Post 27 — World Sandbox & Interactions: Proving Grounds, Leashing AI, and the MHInteraction Framework

*4 commits · Oct 23, 2026 · covering Devil Square Arena, Isometric Camera Rig, StateTree AI Leash & Evade Return, Dead-Target Retention Fixes, MHInteraction Framework Plugin, Spatial Query Focus Systems, Branching Dialogue Trees, Interactive Doors & Triggers*

---

## Introduction

An Action RPG without world interactivity is little more than a tech demo running in a void. Once abilities, inventory, multiplayer networking, and character appearances are stable, gameplay requires **living spaces**: colosseums where combat pacing can be stressed to its limits, non-player characters with compelling dialogue, interactive environmental doors, and AI behaviors that respect territorial boundaries.

On October 23, 2026, I executed a major dual-track sprint across 4 high-impact commits:
1. **The Proving Grounds & AI Combat Hardening:** Built the Devil Square arena map (`ProvingGrounds.umap`), tuned our isometric camera framing, and solved critical StateTree combat AI bugs—specifically target retention on player death and home-spawner leash returns.
2. **The MHInteraction Plugin:** Architected an independent, reusable Unreal Engine framework plugin (`MHInteraction`) providing modular interaction components, spatial focus queries, dynamic HUD interaction prompts, physical doors, triggers, and branching NPC dialogue trees.

---

## Technical Exposition

### 1. The Devil Square Proving Grounds & Isometric Camera

To stress-test mob density, group aggro, and ability visualizers under real gameplay conditions, we needed a dedicated battleground free from level geometry clutter. We constructed the **Devil Square Proving Grounds**:

* **Geometry & Spawning:** Circular Roman-style arena featuring tiered concentric steps, centralized mob spawner clusters, and perimeter player spawn gates.
* **Camera Geometry:** Updated the top-down isometric spring-arm camera:
  * Arm Length: `1400.0f`
  * Pitch: `-52.0f`
  * Rotation: Fixed isometric yaw (`45.0f`) with soft lag dampening.
  * Field of View: `65.0f` to prevent peripheral fisheye distortion.
* **Automation Support:** Extended `Test_All.ps1` with a `-Map ProvingGrounds` argument, allowing CI scripts to boot headless dedicated servers and spin up automated combat simulations in the arena.

```
       +---------------------------------------------+
       |             Devil Square Arena              |
       |  +---------------------------------------+  |
       |  |          Outer Perimeter Gate         |  |
       |  |  +---------------------------------+  |  |
       |  |  |        Middle Tier Steps        |  |  |
       |  |  |  +---------------------------+  |  |  |
       |  |  |  |    Central Boss Spawner   |  |  |  |
       |  |  |  +---------------------------+  |  |  |
       |  |  +---------------------------------+  |  |
       |  +---------------------------------------+  |
       +---------------------------------------------+
```

### 2. StateTree AI Hardening: Leashing, Evade, and Death Cleansing

During intensive combat playtests, two critical AI flaws emerged:
1. **The Undead Grudge (Corpse Targeting):** When a player died, nearby enemies retained the dead player as their primary target. Enemies remained locked in attack montages, repeatedly swinging at the ragdolled corpse rather than resetting.
2. **Infinite Kiting (Missing Leash Return):** Enemies chased players across the entire map without ever disengaging.

We hardened the enemy StateTree logic in commit `82160ee`:

```
               [Player Enters Perception]
                           |
                           v
                  +-----------------+
                  |  Engage Target  |
                  +-----------------+
                     |           |
       Target Health <= 0        | Target Dist > LeashRadius (1800u)
                     |           |
                     +-----+-----+
                           |
                           v
                  +-----------------+
                  |   Evade State   |
                  | - Clear Threat  |
                  | - Invulnerable  |
                  | - Full Heal     |
                  +-----------------+
                           |
                           v
               [Navigate to Spawner Home]
                           |
                           v
                  +-----------------+
                  |    Idle/Patrol  |
                  +-----------------+
```

When a player dies (`Health <= 0`), the enemy's perception evaluator immediately strips the target from the AI Blackboard, clears the threat table, triggers the `Evade` state, heals the enemy back to 100% health, and navigates the enemy back to its home spawner coordinate.

---

## The MHInteraction Plugin Architecture

Following our plugin modularity philosophy established in the MH Framework (Post 24), we avoided placing generic interaction code directly into the monolithic `Onset` game module. Instead, we created **`MHInteraction`**—a standalone plugin providing generic world interaction primitives.

```
+-------------------------------------------------------------+
|                        Onset Game                           |
|       (AOnsetNPC, AOnsetDoor, AOnsetInteractableTrigger)    |
+-------------------------------------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|                      MHInteraction                          |
|  +-------------------------------------------------------+  |
|  | IMHInteractableTarget                                 |  |
|  | - CanInteract(), GetInteractionPrompt(), Interact()   |  |
|  +-------------------------------------------------------+  |
|  +-------------------------------------------------------+  |
|  | UMHInteractableComponent                              |  |
|  | - Proximity Radius, Priority, Highlight Materials     |  |
|  +-------------------------------------------------------+  |
|  +-------------------------------------------------------+  |
|  | UMHInteractionDetectorComponent                       |  |
|  | - Performs forward cone & sphere traces               |  |
|  | - Tracks best interactable target                     |  |
|  | - Broadcasts OnInteractableFocusChanged               |  |
|  +-------------------------------------------------------+  |
+-------------------------------------------------------------+
```

### 1. Spatial Focus & Dot-Product Scoring

To make interactions intuitive when multiple objects are nearby (e.g. an NPC standing next to a door), `UMHInteractionDetectorComponent` evaluates potential targets using a weighted scoring formula:

$$\text{Score} = \left(\frac{\text{MaxDistance} - \text{Dist}}{\text{MaxDistance}}\right) \times 0.4 + \left(\frac{\vec{V}_{\text{CameraForward}} \cdot \vec{V}_{\text{ToTarget}} + 1}{2}\right) \times 0.6$$

This guarantees that an object directly in front of the player's crosshairs or facing direction wins priority over an object that is slightly closer but behind or to the side of the character.

### 2. Branching Dialogue Trees (`AOnsetNPC`)

Using `MHInteraction`, we authored [`AOnsetNPC`](file:///e:/Unreal%20Projects/PortfolioTutorial/Project/Source/Onset/Public/NPC/OnsetNPC.h):

```cpp
void AOnsetNPC::Interact_Implementation(APlayerController* InstigatorPC)
{
    if (AOnsetPlayerController* PC = Cast<AOnsetPlayerController>(InstigatorPC))
    {
        // Face the player during interaction
        FaceSpeaker(PC->GetPawn());

        // Open branching dialogue session
        PC->StartDialogue(DialogueID);
    }
}
```

Dialogue is backed by data tables (`DT_Dialogues`), supporting:
* Speaker portraits and dynamic name banners.
* Multi-stage player response options.
* Condition-gated choices (requiring specific quest progression or military rank).
* Quest initiation and turn-in triggers embedded in dialogue completion nodes.

### 3. Physical Doors and Environmental Triggers

We paired the dialogue system with physical world interactions:
* **[`AOnsetDoor`](file:///e:/Unreal%20Projects/PortfolioTutorial/Project/Source/Onset/Private/World/OnsetDoor.cpp):** Smooth timeline-driven rotation/slide animations, sound effects, lock states, and bidirectional interaction.
* **[`AOnsetInteractableTrigger`](file:///e:/Unreal%20Projects/PortfolioTutorial/Project/Source/Onset/Private/World/OnsetInteractableTrigger.cpp):** Proximity and overlap volumes for triggering cutscenes, zone banners, and ambient audio cues.

---

## Results & Validation

1. **Arena Combat Stability:**
   * Spawned 40 simultaneous enemies in Devil Square.
   * Confirmed solid 60+ FPS performance, zero pooling stalls, and accurate isometric camera framing across all arena tiers.
2. **AI Leash Verification:**
   * Kited enemies 2500+ units away from spawner origins.
   * Upon exceeding leash bounds, enemies immediately dropped aggro, activated evasion effects, and returned to home nodes.
   * Dying mid-combat cleanly reset all attacking enemies to idle states without animation loops.
3. **Dialogue & Door Interaction:**
   * Approached Academy doors and NPCs; HUD prompt dynamically displayed `[E] Open Door` or `[E] Talk`.
   * Executed multi-branch dialogue trees; choices correctly updated player quest log flags and unlocked progression stages.

---

## Key Challenges & Solutions

### Challenge 1: The "Corpse Grind" StateTree Loop

**Problem:** When a player character reached 0 HP, the `AOnsetBaseCharacter::Die()` function marked the player dead, but did not immediately broadcast death to registered AI controllers. The enemy StateTree perception task continued returning `Success` because the player actor still existed in the world.

**Solution:** Added `OnPawnDied` delegate to `AOnsetBaseCharacter`. Enemy AI controllers subscribe to this delegate upon acquiring a target. When fired, the controller immediately forces the StateTree transition into the `Evade` state and resets the target actor pointer.

### Challenge 2: Multi-Target Interaction Flickering

**Problem:** Standing equidistant between an NPC and a door caused the interaction HUD prompt to rapidly flicker between the two targets due to minor floating-point fluctuations in distance calculations.

**Solution:** Introduced **focus hysteresis** in `UMHInteractionDetectorComponent`. A new interactable must score at least 15% higher than the currently focused interactable to take over focus, providing rock-solid UI stability.

---

## Looking Ahead

With the Proving Grounds arena, robust AI leashing, and the `MHInteraction` framework active, our world was finally primed for advanced content creation. 

In **Post 28**, we explore **The Next-Gen Authoring Suite**—building the standalone Authored Appearance Studio, designing the Academy Hub level, adding real-time Viewport Ability Casting with debug wireframes, and engineering our Multi-Tier Enemy Combat Balance Matrix.

---

*Arc commits: `bc86a62`, `82160ee`, `dc57050`, `03eceec`.*
