# 🎬 Post 28 — The Next-Gen Authoring Suite: Appearance Studio, Academy Hub, and Viewport Combat Balancers

*3 commits · Oct 24, 2026 · covering UOnsetAuthoredAppearance & UOnsetAnimationProfile Assets, Turntable Preview Rigs, Academy Hub Level, Military Rank Hierarchy Formulas, Multi-Tier Combat Scaling Matrix (Mitigation %, EHP, TTK), Viewport Ability Casting Wireframe Visualizers (SingleTarget, AoE, PBAoE, Cone), Live DPS/HPS Telemetry, Montage Desync Warnings, Niagara & Audio Cues*

---

## Introduction

In game development, the speed at which content can be authored, balanced, and verified directly determines production velocity. If an artist has to launch a game session just to preview how a uniform looks on an NPC, or if a combat designer has to guess whether an AoE ability's radius matches its particle effects, iteration crawls to a halt.

Having built our modular appearance pipeline (Post 26) and generic world interaction framework (Post 27), we dedicated October 24, 2026, to empowering content creators with a **world-class in-editor authoring suite**.

Across 3 comprehensive commits, we delivered:
1. **The Authored Appearance Studio:** Decoupled appearances into first-class, standalone assets (`UOnsetAuthoredAppearance`) with dedicated turntable controls, animation scrubbing, and visual authoring widgets (`UOnsetAppearanceStudioWidget`).
2. **The Academy Hub & Military Rank Systems:** Built our primary narrative hub map (`Academy.umap`), established a 12-tier military rank hierarchy with authority calculation formulas, and scaffolded the Quest Editor widget (`UOnsetQuestEditorWidget`).
3. **The Multi-Tier Enemy Balance Matrix:** Connected the Enemy Creator directly to authored appearances and GAS ability loadouts, calculating live DEF mitigation %, Effective Health Pools (EHP), and Time-to-Kill (TTK) across Tiers 0–3 with automated pacing hazard detection.
4. **Live Ability Builder & Viewport Casting:** Upgraded the Ability Editor with real-time 3D wireframe visualizers in the editor viewport (SingleTarget, AoE, PBAoE, Cone), live DPS/HPS pacing telemetry cards, montage hit-desync warnings, and Niagara/audio triggers.

---

## Technical Exposition

### 1. Decoupled Visual Identity: The Authored Appearance Asset

Previously, character appearances were coupled to player creation recipes. Enemies and NPCs relied on monolithic skeletal mesh overrides, making it impossible to share modular armor, haircuts, or headgear across factions.

We decoupled appearance data into standalone `UPrimaryDataAsset` types:

```
+-------------------------------------------------------------+
|                UOnsetAuthoredAppearance                     |
|  - Display Name, Category, Tags                             |
|  - FOnsetAppearanceRecipe (Modular Mesh IDs, Colors, Hair)  |
|  - Soft Thumbnail Reference                                 |
+-------------------------------------------------------------+
                               ^
                               | Referenced By
       +-----------------------+-----------------------+
       |                                               |
+-----------------------+                     +-----------------------+
|   AOnsetNPC / Dialog  |                     |   FOnsetEnemyStats    |
| (Academy Instructors) |                     |  (Dungeon Encounters) |
+-----------------------+                     +-----------------------+
```

We paired this with [`UOnsetAnimationProfile`](file:///e:/Unreal%20Projects/PortfolioTutorial/Project/Source/OnsetAppearance/Public/OnsetAnimationProfile.h)—a modular data asset bundling locomotion blendspaces, idle variants, combat montages, and hit reacts into reusable archetypes (e.g. `CivilianCitizen`, `ConfederateMilitary`, `SubstrateScholar`).

### 2. The Multi-Tier Enemy Combat Balance Matrix

Balancing enemy encounters often degrades into guesswork: a designer boosts Defense by 50 points without knowing whether it turns the mob into a damage sponge or how many hits a player will require to defeat it.

In [`UOnsetEnemyEditorWidget`](file:///e:/Unreal%20Projects/PortfolioTutorial/Project/Source/OnsetEditor/Private/UI/OnsetEnemyEditorWidget.cpp), we embedded an automated mathematical simulation engine powered by [`UOnsetEquipmentLibrary`](file:///e:/Unreal%20Projects/PortfolioTutorial/Project/Source/Onset/Public/Combat/OnsetEquipmentLibrary.h):

#### Core Mathematical Formulas:

$$\text{Mitigation} = \frac{\text{DEF}}{\text{DEF} + K_{\text{DEF}}} \quad \text{where } K_{\text{DEF}} = \text{GetKDefense}()$$

$$\text{EHP} = \text{Health} \times \left(1 + \frac{\text{DEF}}{K_{\text{DEF}}}\right)$$

$$\text{Tier Multiplier} = (1 + d)^T \quad \text{where } d = 0.15 \text{ and } T \in \{0, 1, 2, 3\}$$

Whenever an enemy row is selected or edited, the widget computes a live 4-tier matrix:

```
📊 Combat Balance Matrix (Reference Player DPS: 45.0 | ATK Rate: 1.2s):
Tier 0 [Base]: HP 100 | DEF 10 | Mit: 9.1% | EHP: 110.0 | Weapon CD: 1.5s | Enemy DPS: 16.7 | Player TTK: 2.4s | Enemy TTK: 12.0s
Tier 1 [Vanguard]: HP 115 | DEF 11 | Mit: 10.3% | EHP: 128.2 | Weapon CD: 1.5s | Enemy DPS: 19.2 | Player TTK: 2.8s | Enemy TTK: 10.4s
Tier 2 [Elite]: HP 132 | DEF 13 | Mit: 11.6% | EHP: 149.6 | Weapon CD: 1.5s | Enemy DPS: 22.1 | Player TTK: 3.3s | Enemy TTK: 9.0s
Tier 3 [Champion]: HP 152 | DEF 15 | Mit: 13.0% | EHP: 174.8 | Weapon CD: 1.5s | Enemy DPS: 25.4 | Player TTK: 3.9s | Enemy TTK: 7.9s
```

The matrix features automated pacing hazard alerts:
* `⚠️ High burst hazard`: Warns when Enemy DPS exceeds player survivability thresholds.
* `⚠️ Sponge warning`: Warns when Player TTK exceeds 15 seconds for a non-boss mob.

---

## Viewport Casting & Live Ability Visualizers

The updated [`UOnsetAbilityEditorWidget`](file:///e:/Unreal%20Projects/PortfolioTutorial/Project/Source/OnsetEditor/Private/UI/OnsetAbilityEditorWidget.cpp) introduces real-time ability testing directly in the Level Editor viewport without entering Play-In-Editor (PIE).

```
+-------------------------------------------------------------+
|                Level Editor Viewport Preview                |
|                                                             |
|           [Preview Actor / Selected Character]              |
|                             |                               |
|        +--------------------+--------------------+          |
|        | (SingleTarget)     | (Cone Shape)       | (AoE)    |
|        v                    v                    v          |
|   ------> (Sphere)     \  Cone Area  /      (Ground Cyl)    |
|   Range: 300u          Angle: 90°           Radius: 300u    |
|                        Radius: 500u         CastDist: 800u  |
+-------------------------------------------------------------+
```

### 1. Real-Time Targeting Geometry Visualizers

Clicking **"Preview in Viewport"** evaluates the ability's targeting shape and renders 3D debug primitives for 6 seconds with exact distance/angle annotations:

* **Single Target:** `DrawDebugDirectionalArrow` from caster to max attack range, capped by a target hit sphere (`DrawDebugSphere`).
* **AoE (Targeted):** Cast trajectory line to destination point with a 3D ground impact circle and bounding cylinder (`DrawDebugCylinder`).
* **Point-Blank AoE (PBAoE):** Ground blast perimeter and radial cylinder centered on the caster.
* **Cone:** Directional 3D debug cone (`DrawDebugCone`) visualising aperture half-angle and max range.

### 2. Live DPS & Pacing Telemetry

Powered by `UOnsetNotifyDetailsView`, every change to an ability row triggers instant recalculation of combat metrics:

$$\text{Sustained DPS} = \frac{\text{Direct Damage} + \sum (\text{DoT Tick Magnitude} \times \text{NumTicks})}{\max(\text{CooldownSeconds}, 0.05)}$$

$$\text{Threat Generated / sec} = \text{Sustained DPS} \times \text{ThreatMultiplier}$$

```
⚡ Live Pacing: CD 2.50s | Direct Dmg: 60.0 | DoT Dmg: 20.0 | Total Dmg/Cast: 80.0 | DPS: 32.0 | HPS: 0.0 | Threat/s: 48.0 (Mult: 1.5x)
Status: Stun (1.5s), Snare (50% spd, 3.0s)
```

### 3. Automated Animation Desync Warnings

If an attack montage is set, the tool loads the asset synchronously and checks its play length against gameplay parameters:
* **Clipping Hazard:** `⚠️ Clipping Hazard: Montage length (1.80s) > Cooldown (1.00s). Ability can be interrupted before anim completes.`
* **Hit Desync:** `⚠️ Hit Desync: DamageTime (1.20s) > Montage length (0.90s). Hit resolves after animation finishes!`

---

## Academy Hub & Military Hierarchy

In commit `5d4bf01`, we built the **Academy Hub** (`Academy.umap`)—our central civilian and military narrative zone.

To govern faction standing, officer dialogues, and restricted area permissions, we designed a formal 12-tier military hierarchy ([`EOnsetRankId`](file:///e:/Unreal%20Projects/PortfolioTutorial/Project/Source/Onset/Public/Data/OnsetRankTypes.h)):

| Tier | Rank ID | Display Title | Hierarchy Branch |
| :---: | :--- | :--- | :--- |
| **0** | `Cadet` | Academy Cadet | Common / Training |
| **1** | `Ensign` | Junior Ensign | Confederate Military |
| **2** | `Lieutenant` | Tactical Lieutenant | Confederate Military |
| **3** | `Captain` | Strike Captain | Confederate Military |
| **4** | `Major` | Defense Major | Confederate Military |
| **5** | `Commander` | Battle Commander | High Command |
| **6** | `Colonel` | Regiment Colonel | High Command |
| **7** | `Brigadier` | Brigadier General | Star Fleet |
| **8** | `General` | Fleet General | Star Fleet |
| **9** | `HighGeneral` | Sector High General | Star Fleet |
| **10** | `Marshal` | Grand Marshal | Sovereign Council |
| **11** | `SupremeCommander` | Supreme Fleet Commander | Sovereign Council |

Ranks govern dialogue branching: higher-ranking players unlock classified dialogue nodes, issue orders to subordinate NPCs, and bypass security checkpoints.

---

## Results & Validation

1. **Appearance Studio Workflow:**
   * Authored `App_AcademyCadreInstructor`, `App_AegisQuartermaster`, and `App_MeridianAnalyst`.
   * Adjusted hairstyle, facial textures, and modular armor pieces in real time.
   * Rotated the preview turntable smoothly in Slate without frame drops.
2. **Combat Balancing Efficiency:**
   * Tuned enemy defense values and immediately observed mitigation % and player TTK update live in the details panel.
   * Tested viewport ability visualizers: confirmed single-target arrows, AoE cylinders, and cones aligned precisely with character forward vectors.
3. **Audio/Visual Cue Triggers:**
   * Cast abilities with Niagara systems and sounds configured; observed particles and audio cues triggering accurately in the editor viewport.

---

## Key Challenges & Solutions

### Challenge 1: Real-Time Details View Property Notifications

**Problem:** Standard Unreal Engine `UDetailsView` widgets in Editor Utility Widgets do not broadcast property modification events to Blueprint or widget classes when inner struct fields change. Designers had to manually click "Save" before metrics updated.

**Solution:** Implemented `UOnsetNotifyDetailsView`, wrapping Slate's underlying `IDetailsView::OnFinishedChangingProperties` delegate. Any edit immediately dispatches `OnPropertyEdited`, automatically writing changes back to the transient wrapper and refreshing live calculations.

### Challenge 2: Transient Actor Leaks in Editor Viewports

**Problem:** Spawning preview actors in editor viewports for ability wireframes or appearance previews can accidentally save temporary actors into the level map package.

**Solution:** Flagged all spawned editor preview actors with `RF_Transient`, excluded them from level serialization, and implemented automated lifecycle cleanup on editor tab destruction.

---

## Reflection & Looking Ahead

Building content for an Action RPG should never feel like navigating a spreadsheet blindfolded. By embedding real-time combat formulas, live 3D viewport visualizers, and decoupled appearance studios into the Unreal Editor, we transformed content creation into an intuitive, visually rewarding process.

With our character pipeline, interaction framework, Proving Grounds arena, Academy hub, and authoring tools fully operational, Onset is ready for **end-to-end alpha playtesting**.

---

*Arc commits: `6ea6f6b`, `5d4bf01`, `ae844fd`.*
