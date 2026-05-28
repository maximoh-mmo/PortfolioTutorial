# 🧱 PRIVATE DEMO DEVELOPMENT CHECKLIST

Track progress for Phase A — building all 13 systems off-camera before recording begins.
Estimated: ~12 weeks full-time (see [Production Timeline](../Planning/Production_Timeline.md) for details).

---

# A1 — CORE PLAYER SYSTEMS (est. 8 days)

## A1.1 Project Setup & Base Classes
- [x] Create blank C++ project (`Onset`)
- [x] Create folder structure under `Source/Onset/`:
  - `Player/`, `AI/`, `Combat/`, `Spawning/`, `Multiplayer/`
- [x] Create `AOnsetBaseCharacter` — `UCLASS(Blueprintable)`, inherits `ACharacter`; shared base for player and NPC
- [x] Create `AOnsetPlayerCharacter` — `UCLASS(Blueprintable)`, inherits `AOnsetBaseCharacter`
- [x] Create `AOnsetPlayerController` — `UCLASS(Blueprintable)`, inherits BP-able `APlayerController`
- [x] Create `AOnsetPlayerState` — `UCLASS(Blueprintable)`, inherits BP-able `APlayerState`
- [x] Create `AOnsetGameModeBase` — `UCLASS(Blueprintable)`, inherits BP-able `AGameModeBase`
- [x] Create `AOnsetGameState` — `UCLASS(Blueprintable)`, inherits BP-able `AGameStateBase`
- [x] Enable Enhanced Input plugin in `.Build.cs`
- [x] Create `UCursorManager` — `UCLASS(BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))`, provides active cursor screen position from mouse, touch, or gamepad R-Stick
- [x] Create `UTargetingComponent` — data-holder, no Tick
- [x] Create all Input Actions:
  - `IA_Move` (2D Axis — virtual joystick, WASD, gamepad L-Stick)
  - `IA_Cursor` (2D Axis — gamepad R-Stick emulates mouse; mouse/touch use OS)
  - `IA_Primary` (Action — tap, left-click, R-Stick click, A button) — primary interaction, context resolves move/attack/interact
  - `IA_Ability1-4` (Action — number keys, virtual buttons, face buttons)
  - `IA_PvPToggle` (Action — P key, virtual button, D-pad down)
- [x] Create Input Mapping Contexts:
  - `IMC_Touch` (virtual joystick + tap + virtual buttons)
  - `IMC_KbMouse` (mouse + keyboard bindings)
  - `IMC_Gamepad` (gamepad bindings)
- [x] Add `UEnhancedInputLocalPlayerSubsystem` initialization to PlayerController
- [x] Wire up all 3 IA bindings in PlayerController (`IA_Primary`, `IA_Move`, `IA_Cursor`)
- [x] Rename `AOnsetCharacter` → `AOnsetPlayerCharacter` for explicitness
- [x] Verify project compiles and runs in PIE

## A1.2 Top-Down Camera
- [x] Add `SpringArmComponent` to character
- [x] Add `CameraComponent` to character
- [x] Configure arm length (1000), pitch (−60°)
- [x] Enable camera collision (`bDoCollisionTest = true`)
- [x] Enable camera lag (`CameraLagSpeed = 8.0`)
- [x] Set `DefaultPawnClass` in `AOnsetGameModeBase` constructor
- [x] Verify camera follows character in PIE
- [x] Verify collision push-back from walls

## A1.3 Movement System (Touch Joystick + Tap + WASD + Gamepad)
- [x] Enable mouse cursor on PlayerController
- [x] Enable touch input in Project Settings
- [x] Implement **virtual joystick widget** (touch): 2D axis → `IA_Move` → character movement
- [x] Implement **tap-to-move** (touch): `IA_Primary` → raycast → `MoveToLocation`
- [x] Implement **WASD movement** (keyboard): `IA_Move` → character movement
- [x] Implement **gamepad L-Stick movement**: `IA_Move` → character movement
- [x] Implement **gamepad R-Stick cursor**: `IA_Cursor` → software cursor overlay
- [x] Build **cursor abstraction** layer: mouse OS cursor, touch tap position, gamepad R-Stick cursor → unified screen position for all raycasts
- [x] Implement direct-input / pathfinding hand-off: joystick/WASD interrupts active `MoveToLocation`
- [x] Set collision channel for ground trace (ECC_Visibility)
- [x] Add `UEnhancedInputComponent` bindings in PlayerController
- [x] Verify tap-to-move (touch mobile viewport)
- [x] Verify virtual joystick moves character (touch mobile viewport)
- [x] Verify click-to-move (mouse)
- [x] Verify WASD movement (keyboard)
- [x] Verify gamepad L-Stick movement
- [x] Verify gamepad R-Stick cursor moves and stays within viewport
- [x] Verify navigation around obstacles
- [x] Verify joystick/WASD interrupts tap-to-move pathfinding

## A1.4 Targeting Component (Data Holder + Validation)
- [x] Create `UTargetingComponent` — `UCLASS(Blueprintable, BlueprintType, meta=(BlueprintSpawnableComponent))`, no Tick needed
- [x] Add `CurrentTarget` storage + accessors (`GetCurrentTarget`, `SetCurrentTarget`, `ClearTarget`)
- [x] Implement `SetCurrentTarget()` with validation (`IsActorValidTarget()`)
- [x] Implement `IsActorValidTarget()` — basic null + self check (PvP-aware deferred to A1.6, alive check deferred to A4)
- [x] Context resolution in PlayerController: `IA_Primary` → raycast → enemy = SetCurrentTarget, ground = MoveToLocation
- [x] Create `BP_OnsetBaseEnemy` (inherits `AOnsetBaseCharacter`, tag "Enemy") placed in level
- [x] Bind `IA_Ability1-4` + `InjectAbilityInput()` touch bridge in PlayerController
- [x] Stub ability handlers log target name to output log
- [x] Verify target selection works (mouse, touch tap, gamepad R-Stick cursor + A button)
- [x] Verify ability input routes to current target

## A1.5 Ability Targeting System
- [ ] Create `FAbilityTargetData` — `USTRUCT(BlueprintType)` (TargetActor, TargetLocation, TargetDirection)
- [ ] Create `UAbilityTargetingLibrary` — static `GetTargetData(TargetingComponent*, SourceActor)`
- [ ] Update ability stubs to call `GetTargetData()` and log
- [ ] Verify target data returns correct actor/location/direction when target set
- [ ] Verify empty data when no target set

## A1.6 PvP Toggle
- [ ] Add `bIsPvPEnabled` to `APlayerState` (replicated)
- [ ] Add `Server_SetPvPEnabled(bool)` RPC on PlayerController
- [ ] Add `OnRep_PvPEnabled()` callback
- [ ] `IsActorValidTarget()` in TargetingComponent includes PvP filtering
- [ ] Verify toggle replicates to client
- [ ] Verify players filtered when PvP OFF
- [ ] Verify players targetable when PvP ON
- [ ] Verify auto-target fallback on toggle (PvP OFF while targeting player)

---

# A2 — NPC LIFECYCLE (est. 6 days)

## A2.1 NPC Character + Spawner
- [ ] Create `ANPCCharacter` — `UCLASS(Blueprintable)`, inherits `AOnsetBaseCharacter`
- [ ] Create `ANPCAIController` — `UCLASS(Blueprintable)`, inherits BP-able `AAIController`
- [ ] Create `FSpawnConfig` — `USTRUCT(BlueprintType)` (EnemyClass, GroupSize, SpawnRadius, RespawnDelay)
- [ ] Create `AEnemySpawner` — `UCLASS(Blueprintable)`, inherits BP-able `AActor`
- [ ] Implement `SpawnGroup()` with point-based + fallback scatter spawning
- [ ] Implement `DestroyGroup()`
- [ ] Place spawner in level and verify NPCs appear
- [ ] Verify config group size is respected
- [ ] Verify spawn points override fallback scatter

## A2.2 Object Pooling
- [ ] Create `ANPCPoolManager` — `UCLASS(Blueprintable)`, inherits BP-able `AActor`
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
- [ ] Create `AGroupManager` — `UCLASS(Blueprintable)`, inherits BP-able `AActor`
- [ ] Create `UGroupComponent` — `UCLASS(Blueprintable, BlueprintType, meta=(BlueprintSpawnableComponent))`
- [ ] Create `FGroupData` — `USTRUCT(BlueprintType)` (Center, Direction, AliveCount, AssistRadius)
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

# A6 — UI & FINAL DEMO (est. 8 days)

## A6.1 UI System
- [ ] Create `UHUDWidget` (main HUD container)
- [ ] Implement **virtual joystick widget** (touch) — thumbstick zone with drag-outline
- [ ] Implement **virtual ability button widgets** (touch) — 4 ability hotkeys + basic attack
- [ ] Implement **gamepad cursor overlay widget** (software crosshair)
- [ ] Implement player health bar
- [ ] Implement `UEnemyHealthBarWidget` (world-space attached to NPCs)
- [ ] Implement `UAbilityBarWidget` (cooldown display)
- [ ] Implement `UTargetIndicatorWidget` (targeting reticles)
- [ ] Implement debug overlay (`UAutoplayDebugWidget`)
- [ ] Add PvP toggle UI element (virtual button for touch, widget for desktop)
- [ ] Wire up GAS attribute changes → HUD updates
- [ ] Verify health updates correctly
- [ ] Verify enemy health bars appear/disappear
- [ ] Verify ability cooldowns update
- [ ] Verify target indicators match ability behaviour
- [ ] Verify debug UI toggles correctly
- [ ] Verify virtual joystick input on touch device
- [ ] Verify virtual ability buttons trigger GAS abilities
- [ ] Verify gamepad cursor renders and follows R-Stick

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
| A1 Core Player | 38 | 33 | 87% |
| A2 NPC Lifecycle | — | — | 0% |
| A3 AI Systems | — | — | 0% |
| A4 GAS Combat | — | — | 0% |
| A5 Multiplayer & Steam | — | — | 0% |
| A6 UI & Final Demo | — | — | 0% |
| A7 Integration & Harden | — | — | 0% |
| **Total** | 38 | 33 | **87%** |
