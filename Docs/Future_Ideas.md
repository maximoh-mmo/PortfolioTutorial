# Future Ideas & Design Reference

## Intent
Capture architectural patterns, system extensions, and design decisions that fall outside the current scope but are valuable reference material for future tutorial series, personal projects, or architectural exploration.

This is not a roadmap — it is a design journal. Ideas live here without scope-creep pressure.

---

## Corpse System

### Lootable Container Extension
- **Why deferred:** The corpse actor system (A4.5b) provides the container architecture; loot gameplay (walk-over-detect, random drops, pickup feedback) is a self-contained extension on top of it.
- **How it would work:** `AOnsetCorpse` gains an overlap collision box. When a player overlaps, a UI prompt appears. On interact input, the corpse spawns a drop (resource, ammo, etc.) and despawns immediately.
- **Architecture impact:** Adds `UCorpseLootComponent` or a loot table DataAsset. Touch interaction requires the virtual interact button.
- **Relevant to:** Extension to the tutorial series; personal reference for looter-game prototypes.

---

## AI LOD (Level of Detail)

### Distance-Based Tick Reduction
- **Why deferred:** Currently planned as A6.3, but the depth of the system (graduated tick intervals, perception disable, StateTree skip) merits a full design doc.
- **How it would work:** NPCs beyond a distance threshold reduce Tick interval, disable perception updates, and skip StateTree evaluation every N frames. At extreme range, the NPC is replaced by a non-Ticking ghost.
- **Architecture impact:** Needs a distance manager (engine subsystem or world subsystem) that classifies NPCs into LOD bands each frame. StateTree must support tick-rate gating (done via `bTickEnabled` or interval overrides).
- **Relevant to:** Both the tutorial series (A6.3) and general UE performance reference.

---

## AbilitySet DataAssets

### Modular Ability Composition
- **Why deferred:** The current `GrantDefaultAbilities()` is a flat C++ function. As the number of abilities grows, a data-driven composition pattern becomes valuable but is not yet required.
- **How it would work:** A `UOnsetAbilitySet` DataAsset holds `TArray<TSubclassOf<UGameplayAbility>>` and optional tag requirements. The character DataAsset references multiple ability sets. This is the industry standard for shipped GAS titles (Fortnite, Paragon).
- **Architecture impact:** `GrantDefaultAbilities()` iterates ability sets instead of hardcoded `GiveAbility` calls. Each set can be shared across character types or overridden per archetype.
- **Relevant to:** Follow-up tutorial series on data-driven architecture.

---

## DataTable Balance Pass

### CSV-Exportable Enemy Stats
- **Why deferred:** The current AI Profile approach (one DataAsset per enemy type) works for the demo's 2-3 enemy types. With 5+ types, a DataTable wrapper enables batch editing in a spreadsheet format.
- **How it would work:** Create `FEnemyStatsRow` struct (Health, Damage, Speed, AbilityReferences). Populate a DataTable. A balance pass script exports to CSV for tuning in Google Sheets, then reimports.
- **Architecture impact:** The profile system remains the runtime authority. The DataTable is a parallel authoring tool — it can either generate profiles or override them at spawn time.
- **Relevant to:** Episode planning for a "data-driven design" follow-up mini-series.

---

## Procedural Spawn Points

### EQS-Driven Spawn Selection
- **Why deferred:** Current spawner uses fixed spawn points or a ring-scatter fallback. Not flexible enough for dynamic encounter design.
- **How it would work:** The spawner runs an EQS query to find valid spawn locations near the player (or group center) that are on navmesh, out of line-of-sight, and respect minimum/maximum distance constraints.
- **Architecture impact:** Adds EQS dependency, optional. The current slot-based system remains as the simpler alternative for tutorial viewers.
- **Relevant to:** Future tutorial episode on advanced spawning techniques.

---

## Animation Montage Integration

### Death-Pose Capture on Corpse Spawn
- **Why deferred:** The project has no skeletal meshes or animation blueprints yet. When they arrive, the corpse system can capture the last frame of the death montage as a frozen pose.
- **How it would work:** On death, the NPC's skeletal mesh is copied (or its last pose is baked) onto the corpse actor before the NPC returns to pool. No animation instance needed on the corpse — it's a static pose.
- **Architecture impact:** `AOnsetCorpse` needs a skeletal mesh component (disabled by default). On spawn, it copies the NPC's mesh and pose data.
- **Relevant to:** Episode 15 (basic attack animation) extension; corpse visual polish.

---

## PvP-Influenced Corpse Loot

### Filtered Drops by Damage Source
- **Why deferred:** Requires both the corpse loot system and PvP damage tracking to be stable independently.
- **How it would work:** If the killing blow was from a PvP source (player → player), the corpse may drop no loot or reduced loot. If PvE (player → NPC), normal loot rules apply. Prevents PvP farming.
- **Architecture impact:** Damage execution stores the source type in the GameplayEffect context. The corpse spawn reads this context and applies loot rules accordingly.
- **Relevant to:** Full PvPvE game design reference; beyond current scope.

---

## Player StateTree Autoplay

### AI-vs-AI Testing Harness
- **Status:** Player autoplay itself is **implemented** (A3.5 — `AOnsetPlayerAIController`, `PlayerAutoCombat` StateTree, replicated `bAutoplayEnabled`; see [Player AI System](AI/Player_AI_System.md) and [Player System](Player/Player_System.md)).
- **Still deferred:** The AI-vs-AI harness that enables autoplay on the player and NPCs *simultaneously* for performance profiling, balance testing, and recorded demo loops. Planned as an extension of the existing autoplay work rather than new episodes 21-23.
- **How it would work:** Enable autoplay on player and NPCs simultaneously. The entire combat loop runs without human input. Used for stress testing, recorded demo loops, and balance iteration.
- **Architecture impact:** Builds on the existing `AOnsetPlayerAIController` possession-swap flow.
- **Relevant to:** internal testing workflows; demo recording.

---

## Client-Side Corpse Visibility

### Predictive Corpse Spawning for Latency Hiding
- **Why deferred:** Requires dedicated server + client infrastructure first (A5.x). The corpse system must be multiplayer-stable before adding prediction.
- **How it would work:** When a client predicts its own death (high damage taken), it spawns a local-only corpse immediately for visual feedback. The server later confirms and may correct the corpse position.
- **Architecture impact:** Adds a prediction flag to `AOnsetCorpse` (`bPredicted`). Server-authorized corpses have authority; predicted corpses are replaced on server update.
- **Relevant to:** Advanced multiplayer architecture; potential bonus episode.

---

## Global Corpse Cap Heuristics

### Smart Cleanup Beyond Simple Timer
- **Why deferred:** The current approach (hard cap + oldest-destroyed) is sufficient for the demo. Smarter heuristics add complexity without proportional value.
- **How it would work:** Prioritize corpse retention based on: distance to player (keep nearby), time since death (keep recent), whether player has line-of-sight, whether corpse contains un-looted items.
- **Architecture impact:** Adds a cleanup priority system to `UCorpseSubsystem`. The cap enforcement function sorts corpses by priority and removes lowest first.
- **Relevant to:** Reference for MMO-scale corpse management.

---

## Timed Despawn Visual Polish

### Dissolve/Fade-Out Effects
- **Why deferred:** Pure visual polish — no gameplay impact. Deferred until all core systems are stable.
- **How it would work:** During the last 2 seconds of the corpse's lifespan, a material parameter drives a dissolve effect (world-position offset, alpha fade, or particle burst). The corpse is destroyed when the effect completes.
- **Architecture impact:** Adds material parameter binding to `AOnsetCorpse`. Requires a dissolve-ready material on the static mesh.
- **Relevant to:** Visual polish pass; potential bonus episode on material effects.
