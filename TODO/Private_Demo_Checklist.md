# 🧱 PRIVATE DEMO DEVELOPMENT CHECKLIST

Track progress for Phase A — building all 13 systems off-camera before recording begins.
Estimated: ~12 weeks full-time (see [Production Timeline](../Planning/Production_Timeline.md) for details).

---

# A1 — CORE PLAYER SYSTEMS (est. 7 days)

## A1.1 Project Setup & Base Classes
- [ ] Create blank C++ project (`Onset`)
- [ ] Create folder structure under `Source/Onset/`:
  - `Player/`, `AI/`, `Combat/`, `Spawning/`, `Multiplayer/`
- [ ] Create `AOnsetCharacter` base
- [ ] Create `AOnsetPlayerController` stub
- [ ] Create `AOnsetPlayerState` stub
- [ ] Create `AOnsetGameMode` stub
- [ ] Create `AOnsetGameState` stub
- [ ] Verify project compiles and runs in PIE

## A1.2 Top-Down Camera
- [ ] Add `SpringArmComponent` to character
- [ ] Add `CameraComponent` to character
- [ ] Configure arm length (1000-1200), pitch (−55 to −65°)
- [ ] Enable camera collision (`bDoCollisionTest = true`)
- [ ] Enable camera lag (`CameraLagSpeed = 8.0`)
- [ ] Verify camera follows character in PIE
- [ ] Verify collision push-back from walls

## A1.3 Click-to-Move (Mouse + Touch)
- [ ] Enable mouse cursor on PlayerController
- [ ] Enable touch input in Project Settings
- [ ] Create `IA_ClickMove` input action mapping
- [ ] Implement `OnClickMove()` with raycast + `MoveToLocation` (works for both mouse + touch)
- [ ] Set collision channel for ground trace (ECC_Visibility)
- [ ] Verify click moves character to location
- [ ] Verify touch tap moves character (mobile viewport)
- [ ] Verify movement updates on successive taps/clicks
- [ ] Verify navigation around obstacles

## A1.4 Targeting Component
- [ ] Create `UTargetingComponent` class
- [ ] Add `CurrentTarget` storage + accessors
- [ ] Implement `SetCurrentTarget()` with validation
- [ ] Implement `ClearTarget()`
- [ ] Modify click handler to branch: enemy click → target, ground click → move
- [ ] Add `IA_BasicAttack` input action mapping
- [ ] Stub `OnBasicAttack()` that logs target name
- [ ] Verify target selection works
- [ ] Verify attack key routes to current target

## A1.5 Ability Targeting System
- [ ] Create `UAbilityTargetingComponent`
- [ ] Create `FAbilityTargetData` struct
- [ ] Implement `GetTargetUnderCursor()` (single-target)
- [ ] Implement `GetGroundLocationUnderCursor()` (AoE)
- [ ] Implement `GetDirectionFromPlayerToCursor()` (directional)
- [ ] Implement `ShowTargetIndicator()` / `HideTargetIndicator()`
- [ ] Verify single-target selection
- [ ] Verify AoE location selection
- [ ] Verify directional aiming

## A1.6 PvP Toggle
- [ ] Add `bIsPvPEnabled` to `APlayerState` (replicated)
- [ ] Add `Server_SetPvPEnabled(bool)` RPC on PlayerController
- [ ] Add `OnRep_PvPEnabled()` callback
- [ ] Implement `IsActorValidTarget()` with PvP filtering in TargetingComponent
- [ ] Verify toggle replicates to client
- [ ] Verify players filtered when PvP OFF
- [ ] Verify players targetable when PvP ON
- [ ] Verify auto-target fallback on toggle (PvP OFF while targeting player)

---

# A2 — NPC LIFECYCLE (est. 6 days)

## A2.1 NPC Character + Spawner
- [ ] Create `ANPCCharacter` base class
- [ ] Create `ANPCAIController` stub
- [ ] Create `FSpawnConfig` struct (EnemyClass, GroupSize, SpawnRadius, RespawnDelay)
- [ ] Create `AEnemySpawner` actor
- [ ] Implement `SpawnGroup()` with point-based + fallback scatter spawning
- [ ] Implement `DestroyGroup()`
- [ ] Place spawner in level and verify NPCs appear
- [ ] Verify config group size is respected
- [ ] Verify spawn points override fallback scatter

## A2.2 Object Pooling
- [ ] Create `ANPCPoolManager` class
- [ ] Implement pre-allocation of NPC instances
- [ ] Implement `GetNPC()` — returns available NPC
- [ ] Implement `ReleaseNPC()` — returns NPC to pool
- [ ] Implement `ResetNPC()` — clears health, AI state, visuals
- [ ] Handle pool exhaustion fallback (`SpawnActor` if empty)
- [ ] Integrate Spawner → PoolManager flow
- [ ] Verify NPCs reset correctly on reuse
- [ ] Verify no stale targets or group data after reset
- [ ] Verify no crash when pool is exhausted

## A2.3 Group System
- [ ] Create `AGroupManager` class
- [ ] Create `UGroupComponent` for NPCs
- [ ] Create `FGroupData` struct (Center, Direction, AliveCount, AssistRadius)
- [ ] Implement `RegisterMember()` / `UnregisterMember()`
- [ ] Implement `UpdateGroupData()` (center, direction, alive count)
- [ ] Implement `NotifyMemberAttacked()` — broadcasts assist event
- [ ] Implement `GetNearbyAllies()` — find allies within assist radius
- [ ] Integrate Spawner → GroupManager registration
- [ ] Integrate Pool → reset group membership on reuse
- [ ] Verify NPCs register/unregister correctly
- [ ] Verify group center updates
- [ ] Verify assist radius triggers correctly

---

# A3 — AI SYSTEMS (est. 10 days)

## A3.1 StateTree Setup + Schema
- [ ] Add `StateTreeComponent` to ANPCCharacter
- [ ] Create `UNPCStateTreeSchema` with context data
- [ ] Bind NPC context (self, target, group data, health)
- [ ] Verify StateTree compiles and runs on NPC spawn
- [ ] Add on-screen debug display for current AI state

## A3.2 AI Perception
- [ ] Add `AIPerceptionComponent` to ANPCAIController
- [ ] Configure sight config (range, angle, lose sight time)
- [ ] Configure hearing config (range)
- [ ] Implement `OnPerceptionUpdated()` handler
- [ ] Feed perception data into StateTree context
- [ ] Verify perception triggers on player proximity

## A3.3 Behaviour States (Idle → Flee)
- [ ] Implement **Idle** state (timer-based, stand still)
- [ ] Implement **Roam** state (Brownian motion, group cohesion)
- [ ] Implement **Agro** state (face target, prepare to chase)
- [ ] Implement **Chase** state (MoveToActor, distance checks)
- [ ] Implement **Attack** state (trigger GA_Attack, cooldown)
- [ ] Implement **Flee** state (retreat when low health + isolated)
- [ ] Implement **Lost** state (target lost → return to Roam)
- [ ] Wire up StateTree transitions between all states
- [ ] Verify full behaviour loop: Idle → Roam → Agro → Chase → Attack → (repeat/retreat)

## A3.4 Group Assist Integration
- [ ] Add assist event input to StateTree context
- [ ] Implement **Assist** state transition (Agro on assist event)
- [ ] Verify assist triggers when nearby ally is attacked
- [ ] Verify no assist when attacker is out of assist radius

## A3.5 Player AI Autoplay
- [ ] Create `APlayerAIController` class
- [ ] Create `UPlayerAIStateTreeComponent`
- [ ] Implement player AI StateTree: Idle → SeekTarget → MoveToTarget → Attack
- [ ] Implement `EnableAutoplay(bool)` — possession switching
- [ ] Ensure PvP rules respected (ignore players when PvP OFF)
- [ ] Verify clean toggle on/off
- [ ] Verify AI picks reasonable targets
- [ ] Verify abilities fire correctly under AI control

---

# A4 — GAS COMBAT (est. 10 days)

## A4.1 GAS Setup
- [ ] Add `AbilitySystemComponent` to player and NPC
- [ ] Create `UAttributeSet` (Health, MaxHealth, Damage)
- [ ] Set up `GameplayTags` (damage type, state tags, cooldown tags)
- [ ] Initialize attributes on BeginPlay
- [ ] Verify attributes replicate
- [ ] Verify ASC initializes correctly on both player and NPC

## A4.2 Basic Attack Ability
- [ ] Create `UGameplayAbility_MeleeAttack` (C++ GA)
- [ ] Implement targeting from TargetingComponent/TargetData
- [ ] Apply damage via GameplayEffect
- [ ] Add animation montage support (simple melee swing)
- [ ] Verify player attack hits target
- [ ] Verify NPC attack hits player

## A4.3 Hit Reaction Ability
- [ ] Create `UGameplayAbility_HitReaction`
- [ ] Apply hitstop / stagger effect
- [ ] Trigger on damage received
- [ ] Verify hit reaction plays on damage
- [ ] Verify cooldown prevents hit-reaction spam

## A4.4 NPC Attack Integration
- [ ] Trigger `GA_Attack` from NPC StateTree Attack state
- [ ] Add cooldown handling in StateTree
- [ ] Verify NPC attacks player in range
- [ ] Verify NPC exits Attack state on cooldown

## A4.5 Damage, Death, and Pool Return
- [ ] Implement health depletion on damage
- [ ] Implement death event (OnDeath())
- [ ] Notify spawner on death → start respawn timer
- [ ] Return NPC to pool on death
- [ ] Verify NPC dies when health ≤ 0
- [ ] Verify respawn timer fires
- [ ] Verify NPC re-enters pool cleanly
- [ ] Verify multiple NPCs dying simultaneously creates independent timers

## A4.6 Multiple Abilities
- [ ] Create dash ability (movement-based GA)
- [ ] Create AoE ground-target ability
- [ ] Create projectile ability
- [ ] Create ability bar UI stub (for testing)
- [ ] Verify all abilities activate from input
- [ ] Verify AoE respects PvP rules (per-target filtering)
- [ ] Verify projectile spawns and hits

---

# A5 — MULTIPLAYER & STEAM (est. 10 days)

## A5.1 Server/Client Authority Setup
- [ ] Define server authority rules per system
- [ ] Add `HasAuthority()` guards to all server-only logic
- [ ] Set up GameMode + GameState for multiplayer
- [ ] Verify PIE with `NetMode` switch (standalone → listen server + client)

## A5.2 Replication Pass
- [ ] Replicate NPC movement (CharacterMovementComponent defaults)
- [ ] Replicate NPC health/attributes (via GAS)
- [ ] Replicate targeting data (client target → server validation)
- [ ] Replicate PvP flag (via PlayerState `OnRep`)
- [ ] Replicate abilities + cooldowns (via GAS)
- [ ] Verify AI runs only on server (guard check)
- [ ] Verify 2-client + server session works

## A5.3 Dedicated Server Build
- [ ] Create dedicated server build configuration
- [ ] Build DS target
- [ ] Create launch script for DS + client
- [ ] Test DS + 1 client connection
- [ ] Test DS + 2+ client connection
- [ ] Verify AI behaves identically on DS vs PIE
- [ ] Verify no client-side authority exploits

## A5.4 Steam Auth Integration
- [ ] Initialize Online Subsystem Steam
- [ ] Implement `RequestAuthTicket()` on client
- [ ] Implement `Server_SendAuthTicket()` RPC
- [ ] Implement `ValidateAuthTicket()` on server
- [ ] Handle Steam not running (graceful fallback/error)
- [ ] Handle invalid/expired ticket
- [ ] Handle ticket validation timeout
- [ ] Verify auth flow with AppID 480 (Spacewar)
- [ ] Verify invalid tickets rejected
- [ ] Verify clients can join Steam-authenticated session

---

# A6 — UI & FINAL DEMO (est. 7 days)

## A6.1 UI System
- [ ] Create `UHUDWidget` (main HUD container)
- [ ] Implement player health bar
- [ ] Implement `UEnemyHealthBarWidget` (world-space attached to NPCs)
- [ ] Implement `UAbilityBarWidget` (cooldown display)
- [ ] Implement `UTargetIndicatorWidget` (targeting reticles)
- [ ] Implement debug overlay (`UAutoplayDebugWidget`)
- [ ] Add PvP toggle UI element
- [ ] Wire up GAS attribute changes → HUD updates
- [ ] Verify health updates correctly
- [ ] Verify enemy health bars appear/disappear
- [ ] Verify ability cooldowns update
- [ ] Verify target indicators match ability behaviour
- [ ] Verify debug UI toggles correctly

## A6.2 Final Demo Loop
- [ ] Implement wave spawning (multiple spawners, progressive difficulty)
- [ ] Implement full combat flow: spawn → fight → die → respawn
- [ ] Integrate player abilities into demo loop
- [ ] Integrate multiplayer + Steam auth into final demo
- [ ] Verify demo loop plays from start to finish
- [ ] Verify no crashes during extended play session

## A6.3 Performance Pass
- [ ] Profile NPC tick cost
- [ ] Optimize AI LOD (disable AI when far from player)
- [ ] Profile pooling vs direct spawn
- [ ] Profile network replication bandwidth
- [ ] Address any performance bottlenecks found

---

# A7 — INTEGRATION & HARDENING (est. 11 days)

## A7.1 Cross-System Bugfixing
- [ ] Player AI vs NPC AI combat test (autoplay + group of NPCs)
- [ ] PvP toggle mid-combat edge cases
- [ ] AoE + PvP damage filtering test
- [ ] Pool exhaustion + stress test (50+ NPCs)
- [ ] Respawn timer cascade test (many simultaneous deaths)
- [ ] Steam auth + DS stress test
- [ ] Network emulation test (50ms, 100ms, 200ms, 5% loss)
- [ ] Death mid-StateTree-evaluation test
- [ ] Verify PvP toggle mid-projectile (impact-time check)

## A7.2 Edge Case Hardening
- [ ] Spawner disabled mid-respawn (timers cancel)
- [ ] No pooled NPCs available (fallback to SpawnActor)
- [ ] Multiple assist events overlapping (debounce)
- [ ] Player targeting a player when PvP turned OFF (auto-select nearest NPC)
- [ ] Player toggles PvP mid-ability
- [ ] NPC loses sight while in assist mode
- [ ] GroupManager destroyed before cleanup
- [ ] Client disconnects mid-combat

## A7.3 Final Testing Pass
- [ ] Run through each system doc's testing checklist
- [ ] Full demo loop: single-player
- [ ] Full demo loop: multiplayer (2 clients)
- [ ] Full demo loop: multiplayer (DS + 2 clients)
- [ ] Export test snapshot to verify workflow
- [ ] Verify all 37 risks mitigated or accepted

---

# ✅ PROGRESS TRACKING

| Section | Tasks | Done | % |
|---------|-------|------|---|
| A1 Core Player | — | — | 0% |
| A2 NPC Lifecycle | — | — | 0% |
| A3 AI Systems | — | — | 0% |
| A4 GAS Combat | — | — | 0% |
| A5 Multiplayer & Steam | — | — | 0% |
| A6 UI & Final Demo | — | — | 0% |
| A7 Integration & Harden | — | — | 0% |
| **Total** | — | — | **0%** |
