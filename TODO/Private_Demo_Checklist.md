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
- [x] Create `FAbilityTargetData` — `USTRUCT(BlueprintType)` (TargetActor, TargetLocation, TargetDirection)
- [x] Create `UAbilityTargetingLibrary` — static `GetTargetData(TargetingComponent*, SourceActor)`
- [x] Update ability stubs to call `GetTargetData()` and log
- [x] Verify target data returns correct actor/location/direction when target set
- [x] Verify empty data when no target set

## A1.6 PvP Toggle
- [x] Add `bIsPvPEnabled` to `APlayerState` (replicated)
- [x] Add `Server_SetPvPEnabled(bool)` RPC on PlayerController
- [x] Add `OnRep_PvPEnabled()` callback
- [x] `IsActorTargetPVPValid()` in TargetingComponent includes PvP filtering
- [x] Verify toggle replicates to client
- [x] Verify players filtered when PvP OFF
- [x] Verify players targetable when PvP ON
- [x] Verify auto-target fallback on toggle (PvP OFF while targeting player)

---

# A2 — NPC LIFECYCLE (est. 6 days)

## A2.1 NPC Character + Spawner
- [x] Create `AOnsetEnemy` — `UCLASS()`, inherits `AOnsetBaseCharacter` (BP-child activated via reparent)
- [x] Create `AOnsetAIController` — `UCLASS()`, inherits `ADetourCrowdAIController`
- [x] Create `FSpawnConfig` — `USTRUCT(BlueprintType)` (EnemyProfile, GroupSize, SpawnRadius, RespawnDelay)
- [x] Create `AOnsetSpawner` — `UCLASS(Blueprintable)`, inherits `AActor`
- [x] Implement slot‑based spawning:
  - [x] Create `FSpawnerSlot` struct (SpawnTransform, Occupant)
  - [x] Implement `InitSlots()` — pre‑computes transforms from `SpawnPoints` or fallback ring scatter
  - [x] Implement `SpawnGroup()` — fills empty slots via `SpawnEnemyAtSlot()`
- [x] Implement `SpawnEnemyAtSlot(int32)` — retrieves NPC from `PoolManager`, calls `ApplyProfile()`, registers with group
- [x] Implement `DestroyGroup()` — iterates slots, destroys all occupants
- [x] Implement `DebugKillLast()` — kills the most recently spawned occupant
- [x] Remove direct SpawnActor fallback — spawner requires PoolManager for all spawning
- [x] Place spawner in level and verify NPCs appear
- [x] Verify config group size is respected
- [x] Verify spawn points override fallback scatter

## A2.2 Object Pooling
- [x] Create `AOnsetPoolManager` — `UCLASS()`, inherits `AActor`, lazy init via `bPoolInitialized`
- [x] Remove `PoolClass` property — hardcode `AOnsetEnemy::StaticClass()` for pre-allocation
- [x] Implement pre-allocation — `InitializePool()` spawns `PoolSize` NPCs, `ReturnToPool()` deactivates
- [x] Implement `GetPooledEnemy()` — returns deactivated NPC from pool, `ActivateEnemy()` re-enables
- [x] Implement `ReleasePooledEnemy()` — calls `ReturnToPool()`, deactivates + stores
- [x] Implement `ReturnToPool()` — resets location, profile, collision, tick/input; calls `ApplyProfile(nullptr)`
- [x] Handle pool exhaustion fallback — `SpawnActor` + add to pool if all pooled NPCs in use
- [x] Integrate Spawner → PoolManager — `AOnsetSpawner.PoolManager` ref; spawner always goes through pool
- [x] Verify NPCs reset correctly on reuse — collision/hidden/tick state toggled correctly
- [x] Verify no stale targets or group data after reset
- [x] Verify no crash when pool is exhausted

## A2.3 Group System
- [x] Create `FGroupData` — `USTRUCT(BlueprintType)` (Center, AliveCount). Direction deferred to A3.3 Roam  
- [x] Create `UGroupComponent` — on `AOnsetEnemy`, stores ref to `UGroupManagerComponent`
- [x] Create `UGroupManagerComponent` — on `AOnsetSpawner`, manages members + metrics (component, not actor)
- [x] Implement `RegisterMember()` / `UnregisterMember()` on `UGroupManagerComponent`
- [x] Implement `GetGroupData()` — computes center + alive count on demand (no manual `UpdateGroupData()`)
- [x] Implement `GetNearbyAllies()` — filters group members by distance (not part of assist flow; kept for cohesion queries)  
- [x] ~~NotifyMemberAttacked~~ — **Removed.** Assist now flows through AI Perception: damage emits `FAINoiseEvent` → each AI controller's hearing picks it up within its `HearingRange` → `OnPerceptionUpdated` checks group membership → sets StateTree assist flag  
- [x] Integrate Spawner → Group — `SpawnGroup()` calls `RegisterMember`, `DestroyGroup()` calls `UnregisterMember`
- [x] Integrate Pool → Group — `ReturnToPool()` calls `UnregisterFromGroup()` on NPC's component
- [x] Verify NPCs register/unregister correctly — tested via editor
- [x] Verify group center updates — tested (had `IsHidden` polarity bug, now fixed)
- [ ] ~~Verify assist radius triggers correctly~~ — **Moved to A3.4.** Now handled by AI Perception hearing range (noise event → `OnPerceptionUpdated`)  

---

# A3 — AI SYSTEMS (est. 10 days)

## A3.0 AI Profile System (data-driven controller)
- [x] Create `UAIProfile` — `UDataAsset` subclass (`Onset/Source/Onset/Public/AI/AIProfile.h`)
- [x] Add profile fields: `SkeletalMesh`, `AnimBlueprintClass`, `OverrideMaterial`, `StateTreeAsset`, sight range/angle, hearing range, aggression, flee threshold, assist radius
- [x] Create `AIProfile.cpp` with default values
- [x] Add `UPROPERTY(EditAnywhere) UAIProfile* Profile` to `AOnsetEnemy`
- [x] Add `ApplyProfile(UAIProfile*)` to `AOnsetEnemy` — sets mesh, anim BP, material from profile on spawn; clears all on `nullptr`
- [x] Refactor `AOnsetAIController` to be data‑driven:
  - [x] Add `UStateTreeComponent` and `UAIPerceptionComponent` as subobjects
  - [x] Implement `ApplyProfile(const UAIProfile*)` — loads StateTree asset, configures perception
  - [x] On `OnPossess(APawn*)`, read pawn's `UAIProfile` and call `ApplyProfile()`
  - [x] Add `HasAuthority()` guard in `BeginPlay()` — stop tree on clients
- [x] Update `FSpawnConfig.EnemyClass` → `EnemyProfile` (profile reference)

## A3.1 StateTree Setup + Schema
- [x] Create `UNPCStateTreeSchema` with context data *(created as `UOnsetStateTreeSchema`)*
- [x] Bind NPC context (self, target, group data, health) *(via `FOnsetStateTreeContextTask` Global Task; group moved to per-task read — not universal context)*
- [x] Verify StateTree compiles and runs on NPC spawn
- [ ] Add on-screen debug display for current AI state

## A3.2 AI Perception
- [x] Add `AIPerceptionComponent` to AOnsetAIController *(done as part of A3.0)*
- [x] Configure sight config (range, angle, lose sight time) *(profile-driven, done in A3.0)*
- [x] Configure hearing config (range) *(profile-driven, done in A3.0)*
- [x] Implement `OnPerceptionUpdated()` handler
- [x] Feed perception data into StateTree context
- [x] Verify perception triggers on player proximity
- [ ] Verify perception hearing triggers on assist (deferred until GAS/attack emits noise events — A4)

## A3.3 Behaviour States (Idle → Flee)
- [x] Implement **Idle** state (timer-based, stand still) — `FOnsetStateTreeIdleTask`
- [x] Implement **Roam** state (nav-reachable territory patrol, home-anchor) — `FOnsetStateTreeRoamTask`
- [x] Implement **Agro** state (face target via `SetFocus`, facing-angle check + timer) — `FOnsetStateTreeAgroTask`
- [x] Implement **Chase** state (MoveToActor, exit on arrival, transitions gated by DistanceCondition) — `FOnsetStateTreeChaseTask`
- [x] Implement **Attack** state (timer-based cooldown stub, ready for GAS) — `FOnsetStateTreeAttackTask`
- [x] Implement **Flee** state (retreat when low health + isolated)
- [x] Implement **Lost** state (target lost → clear focus, random pause 2-4s, → Roam) — `FOnsetStateTreeLostTargetTask`
- [x] Create **FOnsetStateTreeTaskBase** — shared helpers (GetController, GetTarget, HasMoveCompleted, GetSelfBaseCharacter, GetPathFollowingComponent); all 5 tasks migrated
- [x] Create **FOnsetStateTreeDistanceCondition** — reusable transition condition (Target or HomeLocation, DistSquared, UE::StateTree::EComparisonOperator)
- [x] Create **Marooned** state — same ChaseTask in asset, no leash transition
- [x] Move **HomeLocation** from AOnsetEnemy → AOnsetBaseCharacter (shared with player for respawn)
- [x] Wire up StateTree transitions between all states
- [x] Verify full behaviour loop: Idle → Roam → Agro → Chase → Attack → (repeat/retreat)

## A3.4 Group Assist Integration
- [x] Emit FAINoiseEvent from PostGameplayEffectExecute on damage taken
- [x] Store noise info on AOnsetAIController (HeardNoiseLocation, bHasPendingNoise, etc.)
- [x] Split OnPerceptionUpdated: sight sets TargetingComponent, hearing stores noise info only
- [x] Create FOnsetStateTreeHearingCondition — gates Idle→Investigate by pending noise + remembrance time
- [x] Create FOnsetStateTreeInvestigateTask — moves to noise location, exits on sight/arrival/expiry, speed varies by group membership
- [x] Add ChaseRange + AttackRange to AIProfile (1000 / 250)
- [x] Update DistanceCondition with AttackRange/ChaseRange source types (reads from profile)
- [x] Fix DistanceCondition inverted pawn check bug
- [x] Create HasTarget + HasNoTarget conditions (shared empty instance data, inline)
- [x] Add ClearFocus to FleeTask EnterState
- [x] Wire full StateTree: Idle→Agro→Chase→Attack→Flee + Investigate + LostTarget
- [x] Implement **Search** state (Alerted: yaw sweep scan at noise origin)
- [x] Wire StateTree Search state: Investigate → Search → Idle
- [ ] Verify assist triggers when nearby ally is attacked
- [ ] Verify no assist when attacker is out of hearing range
- [x] Assist radius testing moved here from A2.3 — assist now flows through perception hearing, not Group System

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
- [x] Add `AbilitySystemComponent` to player and NPC — `CreateDefaultSubobject` on `AOnsetBaseCharacter` (was already done)
- [x] Create `UOnsetAttributeSet` (Health, MaxHealth, clamp in PostGameplayEffectExecute)
- [x] Set up `GameplayTags` (Damage.Physical, Damage.Magical, State.Dead, State.Staggered, State.Invulnerable, Cooldown.Melee) — native tags via UE_DEFINE_GAMEPLAY_TAG macros
- [x] Initialize attributes on BeginPlay/PossessedBy — `InitAbilityActorInfo` in PossessedBy
- [x] Verify attributes replicate — `DOREPLIFETIME_CONDITION_NOTIFY` + `GAMEPLAYATTRIBUTE_REPNOTIFY` in OnRep
- [x] Verify ASC initializes correctly on both player and NPC — PossessedBy fires for both
- [x] Actually verify via runtime test

## A4.2 Basic Attack Ability
- [x] Create `UOnsetGA_BasicAttack` (C++ GA)
- [x] Implement targeting from TargetingComponent/TargetData
- [x] Apply damage via GameplayEffect
- [ ] Add animation montage support (simple melee swing)
- [x] Verify player attack hits target
- [x] Verify NPC attack hits player

## A4.3 Hit Reaction Ability
- [x] Create `UOnsetGA_HitReaction` (C++ GA)
- [ ] Apply hitstop / stagger effect
- [x] Trigger on damage received (HandleGameplayEvent in PostGameplayEffectExecute)
- [x] Verify hit reaction plays on damage
- [x] Verify cooldown prevents hit-reaction spam
- [x] Apply stagger effect (GE_Stagger applied in ActivateAbility)

## A4.4 NPC Attack Integration
- [x] Trigger `GA_Attack` from NPC StateTree Attack state (TryActivateAbilityByClass in AttackTask)
- [x] Add cooldown handling in StateTree (cooldown driven by GAS GE, not StateTree timer)
- [x] Verify NPC attacks player in range
- [x] Verify NPC exits Attack state on cooldown

## A4.5a — Damage, Death, Pool Return, Respawn (baseline)
- [x] Implement health depletion on damage
- [x] Implement death event (OnDeath())
- [x] Notify spawner on death → start respawn timer
- [x] Return NPC to pool on death
- [x] Verify NPC dies when health ≤ 0
- [x] Verify respawn timer fires
- [x] Verify NPC re-enters pool cleanly
- [x] Verify multiple NPCs dying simultaneously creates independent timers
- [x] Create `UOnsetCheatManager` — God() toggles invulnerability tag, Heal() restores health
- [x] Wire CheatClass in PlayerController, add god mode guard to PostGameplayEffectExecute

## A4.5b — Corpse Actor System
- [x] Create `AOnsetCorpse` — minimal actor, static mesh, timed self-destruct
- [x] Create `UOnsetCorpseSubsystem` — world subsystem with cap + lazy sweep
- [x] On NPC death: spawn corpse at death location via subsystem
- [x] Corpse despawns after configurable lifespan (`SetLifeSpan`)
- [x] Hard cap on active corpses (oldest evicted via SweepDeadCorpses)
- [x] Migrated `AOnsetPoolManager` → `UOnsetPoolSubsystem` (subsystem pattern)
- [x] Verify NPC returns to pool immediately (corpse lifecycle independent)
- [x] Verify spawner respawn timer starts at death, not corpse despawn
- [ ] Verify corpse spawned for player kills and NPC-kills-NPC deaths
- [ ] Verify no performance regression under rapid death cascade

## A4.6 Multiple Abilities
- [ ] Create **AoE** ability (target-centered damage volume) — activated GA, slot 1
- [ ] Create **Cone** ability (directional frontal cone) — activated GA, slot 2
- [ ] Create **Shadowstep** passive (on-kill blink behind nearest enemy, cooldown-gated) — replaces dash
- [ ] Wire `OnAbility1` → AoE, `OnAbility2` → Cone in PlayerController
- [ ] Create ability bar UI stub (for testing)
- [ ] Verify all abilities activate from input
- [ ] Verify AoE respects PvP rules (per-target filtering)
- [ ] Verify Cone respects PvP rules (per-target filtering)
- [ ] Verify Shadowstep triggers on kill
- [ ] Verify Shadowstep respects distance gate
- [ ] Verify Shadowstep cooldown prevents spam

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
| A1 Core Player | 38 | 38 | 100% |
| A2 NPC Lifecycle | 35 | 35 | 100% |
| A3 AI Systems | — | 36 | — |
| A4 GAS Combat | 51 | 38 | 75% |
| A5 Multiplayer & Steam | — | — | — |
| A6 UI & Final Demo | — | — | — |
| A7 Integration & Harden | — | — | — |
| **Total** | — | — | **—** |
