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

## A3.0 AI Profile System (data-driven controller) — Refactored E21
- [x] Create `UAIProfile` — `UDataAsset` subclass (`Enemy/Profile/AIProfile.h`), behaviour-only: `StateTreeAsset`, `Aggression`, `FleeThreshold`, `AssistRadius`, `AttackRange`, `ChaseRange`
- [x] Create `UVisualProfile` — `UDataAsset` (`Enemy/Profile/VisualProfile.h`): `SkeletalMesh`, `CorpseMesh`, `AnimBlueprintClass`, `OverrideMaterial`
- [x] Create `UPerceptionProfile` — `UDataAsset` (`Enemy/Profile/PerceptionProfile.h`): `SightRange`, `SightAngle`, `HearingRange`
- [x] Split old monolithic `UAIProfile` into three focused data assets (Single Responsibility Principle)
- [x] Move profiles from `AI/` to `Enemy/Profile/` directory
- [x] Add three `UPROPERTY` profile refs to `AOnsetEnemy`: `UAIProfile* Profile`, `UVisualProfile* VisualProfile`, `UPerceptionProfile* PerceptionProfile`
- [x] Add `ApplyProfile(UVisualProfile*)` to `AOnsetEnemy` — sets mesh, anim BP, material on spawn; clears on `nullptr`
- [x] Add `ApplyAIProfile(const UAIProfile*)` to `AOnsetAIController` — loads StateTree asset, stops/restarts logic
- [x] Add `ApplyPerceptionProfile(const UPerceptionProfile*)` to `AOnsetAIController` — configures sight/hearing configs
- [x] On `OnPossess(APawn*)`, caches `TargetingComponent` from pawn (not owned by controller)
- [x] Update `FSpawnConfig` with `EnemyAIProfile`, `EnemyVisualProfile`, `EnemyPerceptionProfile`
- [x] Update `SpawnEnemyAtSlot` to apply all three profiles on spawn + possess
- [x] Add `HasAuthority()` guard in `BeginPlay()` — stop tree on clients
- [x] **Bug fix (E22):** `ApplyPerceptionProfile` was missing from `SpawnEnemyAtSlot` — targeting was broken

## A3.1 StateTree Setup + Schema
- [x] Create `UNPCStateTreeSchema` with context data *(created as `UOnsetStateTreeSchema`)*
- [x] Bind NPC context (self, target, group data, health) *(via `FOnsetStateTreeContextTask` Global Task; group moved to per-task read — not universal context)*
- [x] Verify StateTree compiles and runs on NPC spawn
- [x] Add on-screen debug display for current AI state

## A3.2 AI Perception
- [x] Add `AIPerceptionComponent` to AOnsetAIController *(done as part of A3.0)*
- [x] Configure sight config (range, angle, lose sight time) *(profile-driven, done in A3.0)*
- [x] Configure hearing config (range) *(profile-driven, done in A3.0)*
- [x] Implement `OnPerceptionUpdated()` handler
- [x] Feed perception data into StateTree context
- [x] Verify perception triggers on player proximity
- [x] Verify perception hearing triggers on assist

## A3.3 Behaviour States (Idle → Flee)
- [x] Implement **Idle** state (timer-based, stand still) — `FOnsetStateTreeIdleTask`
- [x] Implement **Roam** state (nav-reachable territory patrol, home-anchor) — `FOnsetStateTreeRoamTask`
- [x] Implement **Agro** state (face target via `SetFocus`, facing-angle check + timer) — `FOnsetStateTreeAgroTask`
- [x] Implement **Chase** state (MoveToActor, exit on arrival, transitions gated by DistanceCondition) — `FOnsetStateTreeChaseTask`
- [x] Implement **Attack** state (timer-based cooldown stub, ready for GAS) — `FOnsetStateTreeAttackTask`
- [x] Implement **Flee** state (retreat when low health + isolated)
- [x] **E22:** FleeTask refactored — replaced direct `MaxWalkSpeed` with GAS GE (`ApplyMovementSpeedModifier` handle), remove/re-apply per tick for dynamic health-ratio speed
- [x] Implement **Lost** state (target lost → clear focus, random pause 2-4s, → Roam) — `FOnsetStateTreeLostTargetTask`
- [x] Create **FOnsetStateTreeTaskBase** — shared helpers (GetController, GetTarget, HasMoveCompleted, GetSelfBaseCharacter, GetSelfEnemyCharacter, GetPathFollowingComponent); all helpers moved from inline header to `.cpp`
- [x] **E22:** Add `ApplyMovementSpeedModifier(Self, Magnitude)` to base — creates infinite GE with `MultiplyCompound` on `MovementSpeed`, returns `FActiveGameplayEffectHandle`
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
- [x] **E22:** InvestigateTask refactored — replaced direct `MaxWalkSpeed` with GAS GE (apply on EnterState, remove on ExitState)
- [x] Add ChaseRange + AttackRange to AIProfile (1000 / 250)
- [x] Update DistanceCondition with AttackRange/ChaseRange source types (reads from profile)
- [x] Fix DistanceCondition inverted pawn check bug
- [x] Create HasTarget + HasNoTarget conditions (shared empty instance data, inline)
- [x] Add ClearFocus to FleeTask EnterState
- [x] Wire full StateTree: Idle→Agro→Chase→Attack→Flee + Investigate + LostTarget
- [x] Implement **Search** state (Alerted: yaw sweep scan at noise origin)
- [x] **E22:** SearchTask refactored — replaced direct `MaxWalkSpeed` with GAS GE (**fixes speed leak:** ExitState previously called base which did nothing)
- [x] Wire StateTree Search state: Investigate → Search → Idle
- [x] Verify assist triggers when nearby ally is attacked
- [x] Verify no assist when attacker is out of hearing range
- [x] Assist radius testing moved here from A2.3 — assist now flows through perception hearing, not Group System

## A3.6 Threat System
- [x] Create `UOnsetThreatSubsystem` — world subsystem with threat table
- [x] Wire damage feed: `PostGameplayEffectExecute` → `AddThreat(InstigatorPlayerState, Victim, Damage)`
- [x] Wire cleanup: `DeferredDeathCleanup` + `ReturnToPool` → `RemoveEnemy()`
- [x] Add `GetThreatSubsystem()` + `GetThreatAngularOffset()` to `FOnsetStateTreeTask` base
- [x] **Architecture simplified:** AgroTask, ChaseTask, AttackTask, AttackPositionTask, IdleTask, RoamTask deleted. Replaced by:
  - `PatrolTask` — 50/50 idle vs roam in one task
  - `EngageTask` — single combat state: target switching, angular offset positioning, ability firing, AIProfile-driven ranges, crowd avoidance
- [x] AI LOD — `UpdateLodTier` in `AOnsetAIController`: 3 tiers (full, throttled 0.2s, paused)
- [x] AIProfile-driven ranges — `EngageTask::EnterState` reads `AttackRange`/`ChaseRange` from profile at runtime
- [x] Sight-based threat — `OnPerceptionUpdated` adds base threat (1.0) on visual contact if not already engaged
- [x] `IsEnemyEngagedWithPlayer()` — query method for engagement safety
- [x] `ClearFocus()` on lost target — AI stops staring at null
- [x] StateTree reduced to 6 top-level subtrees (no Selector — event-driven transitions)
- [x] Debug logging stripped from EngageTask and ThreatSubsystem
- [x] `Subsystem/` directory migration (plural → singular)
- [x] Verify: threat drives targeting, angular spread prevents bunching, corpses ignore pawn collision

## A3.5 Player AI Autoplay
- [x] Create `AOnsetPlayerAIController` class (created as `APlayerAIController.h/.cpp`)
- [x] ~~Create `UPlayerAIStateTreeComponent`~~ — N/A, uses `UStateTreeAIComponent` (same component as NPCs)
- [x] Implement player AI StateTree: AcquireTargetTask → EngageTask (combined movement+attack)
- [x] Implement `EnableAutoCombat()` / `DisableAutoCombat()` — possession switching
- [x] Ensure PvP rules respected (targets filtered by `IsAlive()`, ignores players)
- [x] Verify clean toggle on/off
- [x] Verify AI picks reasonable targets
- [x] Verify abilities fire correctly under AI control

---

# A4 — GAS COMBAT (est. 10 days)

## A4.1 GAS Setup
- [x] Add `AbilitySystemComponent` to player and NPC — `CreateDefaultSubobject` on `AOnsetBaseCharacter`
- [x] Create `UOnsetAttributeSet` (Health, MaxHealth, clamp in PostGameplayEffectExecute)
- [x] Set up `GameplayTags` (Damage.Physical, Damage.Magical, State.Dead, State.Staggered, State.Invulnerable, Cooldown.BasicAttack) — native tags via UE_DEFINE_GAMEPLAY_TAG macros
- [x] Initialize attributes on BeginPlay/PossessedBy — `InitAbilityActorInfo` in PossessedBy
- [x] Verify attributes replicate — `DOREPLIFETIME_CONDITION_NOTIFY` + `GAMEPLAYATTRIBUTE_REPNOTIFY` in OnRep
- [x] Verify ASC initializes correctly on both player and NPC — PossessedBy fires for both
- [x] Actually verify via runtime test
- [x] **E22:** Create `UOnsetMovementAttributeSet` (dedicated movement set, industry pattern) — `MovementSpeed` attribute with own `PostGameplayEffectExecute` (clamp ≥ 0, sync `MaxWalkSpeed`)
- [x] **E22:** All GAS files migrated from `Combat/` to `GAS/` directory
- [x] **E22:** Base value initialises from CDO default (`InitMovementSpeed(600.0f)`), overridable per BP Class Defaults

## A4.2 Basic Attack Ability
- [x] Create `UOnsetGA_BasicAttack` (C++ GA)
- [x] Implement targeting from TargetingComponent/TargetData
- [x] Apply damage via GameplayEffect
- [ ] ~~Add animation montage support (simple melee swing)~~ ⏳ **DEFERRED** — pending full design pass
- [x] Verify player attack hits target
- [x] Verify NPC attack hits player

## A4.3 Hit Reaction Ability
- [x] Create `UOnsetGA_HitReaction` (C++ GA)
- [ ] ~~Apply hitstop / stagger effect~~ ⏳ **DEFERRED** — pending full design pass
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
- [x] **E22:** Pool `ReturnToPool` now calls `RemoveActiveEffects` to clear all GEs on return (prevents speed leak across pool cycles)
- [x] Verify NPC returns to pool immediately (corpse lifecycle independent)
- [x] Verify spawner respawn timer starts at death, not corpse despawn
- [x] Verify corpse spawned for player kills and NPC-kills-NPC deaths
- [x] Verify no performance regression under rapid death cascade

## A4.6 Multiple Abilities
- [x] Create **AoE** ability (target-centered damage volume) — activated GA, slot 1
- [x] Create **Cone** ability (directional frontal cone) — activated GA, slot 2
- [x] Create **Shadowstep** passive (on-kill blink behind nearest enemy, cooldown-gated) — replaces dash
- [x] Wire `OnAbility1` → AoE, `OnAbility2` → Cone in PlayerController
- [x] Create ability bar UI stub (for testing)
- [x] Verify all abilities activate from input
- [x] Verify AoE respects PvP rules (per-target filtering)
- [x] Verify Cone respects PvP rules (per-target filtering)
- [x] Verify Shadowstep triggers on kill
- [x] Verify Shadowstep respects distance gate
- [x] Verify Shadowstep cooldown prevents spam

---

# A5 — MULTIPLAYER & STEAM (est. 10 days)

## A5.1 Server/Client Authority Setup
- [x] Define server authority rules per system
- [x] Add `HasAuthority()` guards to all server-only logic (21 files — spawner, pool, corpse, threat, controllers, characters, all 11 StateTree tasks)
- [x] Set up GameMode + GameState for multiplayer (PlayerControllerClass, GameStateClass, HUDClass=nullptr)
- [x] Add `bReplicates = true` to `AOnsetBaseCharacter` constructor
- [x] Set `SetReplicateMovement(true)` on `AOnsetEnemy` constructor
- [x] Verify PIE with `NetMode` switch (standalone → listen server + client)

## A5.2 Replication Pass
- [x] Replicate NPC movement — `SetReplicateMovement(true)` on `AOnsetEnemy`
- [x] Replicate NPC health/attributes — GAS replicates natively, verified no errors
- [x] Replicate `bIsAlive` — `GetLifetimeReplicatedProps` with `OnRep_bIsAlive`
- [x] **NEW: Replicate NPC visuals** — `VisualProfile` now `ReplicatedUsing=OnRep_VisualProfile`; client applies skeletal mesh, anim BP, material locally
- [x] **NEW: Fix player death/respawn** — `OnRespawn()` in `RespawnPlayer` restores `bIsAlive=true` → pawn visible again → camera no longer falls
- [x] **NEW: Remove cube fallback** — no runtime `NewObject<UStaticMeshComponent>` for enemy visuals; corpse actor stripped of default cube mesh
- [x] **NEW: VisualProfile validation** — `IsDataValid()` blocks save/cook of incomplete profiles; `checkf` catches runtime null/incomplete
- [x] Replicate targeting data (client target → server validation) — clients run own traces, server validates on activation
- [x] Replicate PvP flag — `bIsPvPEnabled` on `AOnsetPlayerState` was already replicating
- [x] Replicate abilities + cooldowns — GAS handles natively
- [x] Verify AI runs only on server — all controllers + StateTree tasks guarded with `HasAuthority()`
- [x] Verify 2-client + server session works

## A5.3 Dedicated Server Build
- [x] Create dedicated server build configuration (Game target + -server; Server target unsupported)
- [x] Build DS target (Onset.exe via Game target)
- [x] Fix unity build log category conflict
- [x] Create launch script (Scripts/RunDS.ps1)
- [x] Test DS + 1 client connection — WASD, auto-combat, click-to-move verified
- [x] Test DS + 2+ client connection — 3 clients connected, full combat loop functional
- [x] Verify AI behaves identically on DS vs PIE — StateTree, auto-combat, pathfinding all match
- [x] Security audit: 2 Server_ RPCs, 0 Client_ RPCs, both validated *(point-in-time at A5.3; the RPC surface grew in A5b/A5c with persistence + token RPCs — re-run the audit during A7 hardening)*

## A5.4 Steam Auth Integration
- [x] Initialize Online Subsystem Steam
- [x] Implement `RequestAuthTicket()` on client
- [x] Implement `Server_SendAuthTicket()` RPC
- [x] Implement `ValidateAuthTicket()` on server
- [x] Handle Steam not running (graceful fallback/error)
- [x] Handle invalid/expired ticket — `BeginAuthSession` failure rejects client
- [x] Handle ticket validation timeout — `Client_ClearAuthTimeout()` RPC on success; timeout → disconnect
- [x] Verify auth flow with AppID 480 (Spacewar)
- [x] Verify invalid tickets rejected
- [x] Verify clients can join Steam-authenticated session

---
# A5b — PLAYER PERSISTENCE & ACCOUNT SYSTEM (est. 6 days)

## A5b.1 Foundation & Schema Design
- [x] Add SQLite amalgamation to `Source/Onset/ThirdParty/SQLite/`
- [x] Update `Onset.Build.cs` — add SQLite include path and lib
- [x] Create `IPlayerDataStore` abstract interface
- [x] Create `FSQLiteStore` implementing `IPlayerDataStore`
- [x] Create `FPgSQLStore` stub implementing `IPlayerDataStore`
- [x] Create `UOnsetPlayerDataSubsystem` (world subsystem, DS only)
- [x] Schema: `accounts` table with composite PK `(platform, platform_id)`
- [x] Schema: `characters` table with slot_index (0-2), JSON blobs for inventory/equipment/quests
- [x] Schema: `_schema_version` table with migration runner
- [x] WAL mode enabled on SQLite connections
- [x] Config key for store selection (`DataStore=SQLite|Postgres`)

## A5b.2 Data Structs & Serialization
- [x] Create `FOnsetCharacterSlotData` (BlueprintType) — SlotIndex, CharacterName, Level, bOccupied
- [x] Create `FOnsetAccountData` (BlueprintType) — PlatformID, Platform, Slots[3]
- [x] Create `FOnsetFullCharacterData` — full character state (progression, attributes, position, JSON blobs)
- [x] `UOnsetPlayerDataSubsystem` serialization: DB row → struct / struct → DB bind

## A5b.3 SteamID Extraction & Auth Integration
- [x] Include Steamworks SDK headers in GameMode for `ISteamGameServer`
- [x] `ValidateAuthTicket()` calls `SteamGameServer()->BeginAuthSession()` to get SteamID
- [x] Store `PlayerPlatformID` (FString) and `PlayerPlatform` (FString) on `AOnsetPlayerState`
- [x] PostLogin: `LoadAccount(Platform, PlatformID)` → auto-create if first login
- [x] Send `Client_AccountData(FOnsetAccountData)` to client

## A5b.4 Login → Auto-Create → Enter World
- [x] Add `Client_AccountData`, `Client_CharacterData`, `Server_SelectCharacter`, `Server_CreateCharacter`, `Server_SaveCharacter` RPCs
- [x] `Server_SelectCharacter(int32 SlotIndex)` — loads character, spawns pawn, applies save data, sends `Client_CharacterData`
- [x] `Server_CreateCharacter(int32 SlotIndex, FString Name)` — creates default character, auto-selects
- [x] Save-on-disconnect in `AOnsetPlayerController::EndPlay` / `Logout`
- [x] Periodic auto-save: 5-min timer in `UOnsetPlayerDataSubsystem` saves all connected players
- [x] Save on death placeholder (full state saved)

## A5b.5 Lobby Map & Character Select UI → CommonUI Migration
- [x] Create `/Game/Maps/MainMenu` — serves as lobby + main menu, no NPCs/combat
- [x] Set as default map for DS launch (`ServerDefaultMap` in config)
- [x] Implement CommonUI framework (replaces canvas HUDs):
  - `UOnsetScreenBase` (CommonActivatableWidget base)
  - `UOnsetRootLayout` (3 layer stacks: Game, Menu, Modal)
  - `UOnsetUISubsystem` (PushScreen/PopScreen)
  - `UOnsetGameViewportClient` (CommonGameViewportClient)
  - `UOnsetActivatableWidgetStack` + `EOnsetUILayer` enum
  - `UOnsetButtonBase` (hover/click sounds)
- [x] Implement `UMainMenuScreen` — `ConnectToServer()` pushes CharacterSelectScreen
- [x] Implement `UCharacterSelectScreen` — `SelectSlot` handles occupied (enter world) / empty (create character)
- [x] Convert `AOnsetMenuGameMode` to inherit `AOnsetGameModeBase` (gets PostLogin)
- [x] Simplify `AOnsetPlayerController::Client_AccountData` — cache data, no widget creation
- [x] Delete old canvas HUDs: `AOnsetLobbyHUD`, `AOnsetMenuHUD`, `AOnsetLobbyGameMode`
- [x] Delete old widget screens: `UMainMenuWidget`, `UCharacterSelectWidget`
- [x] Content assets: `WBP_RootLayout`, `WBP_MainMenu`, `WBP_CharacterSelect`, `WBP_CharacterCreation`, `WBP_CharacterSlot`, `WBP_LoadingScreen`, `M_Spinner`, styles, fonts, textures
- [x] Wire create → `Server_CreateCharacter` → slot fills
- [x] Wire select → highlight slot → Enter → `Server_SelectCharacter`
- [x] Wire Enter World → `Server_SelectCharacter` → server `ServerTravel` to DemoLevel (replaced post-A5c by two-server token travel; loading screen covers the transition)

## A5b.6 Postgres Store & Production Hardening
- [x] `FPgSQLStore` implementation using libpq (EDB binary distribution, ThirdParty/PostgreSQL/)
- [x] Same parametrized queries as SQLite (portable SQL, adapted for PG syntax: `$1` params, `NOW()`, upsert)
- [x] Config: `Type=Postgres` + `ConnectionString` in `[Onset.DataStore]`
- [x] Migration system from v0 to current (same schema as SQLite)
- [x] Crash recovery: transaction wrapping, `CHECKPOINT` on shutdown
- [x] Build verified — libpq links, DLLs staged, FPgSQLStore compiles

---

# A5c — AUTH EXTRACTION & LOGIN SERVER (est. 4 days)

Status: ✅ Complete (compressed ~1 day)

## A5c.1 Auth Subsystem Extraction (Day 1)
- [x] Create `UOnsetAuthSubsystem` (world subsystem, DS only)
- [x] Move `ValidateAuthTicket` from `AOnsetGameModeBase` to subsystem
- [x] Move `Server_SendAuthTicket` handling to subsystem
- [x] Add `AuthMode=Direct` config key (preserves existing behavior)
- [x] Slim `PostLogin` to delegate to `AuthSubsystem->HandlePostLogin()`
- [x] Slim `Logout` to delegate to `AuthSubsystem->HandleLogout()`
- [x] Verify regression — DirectAuth mode still works (build succeeds, flow preserved)

## A5c.2 Session Token System (Day 1-2)
- [x] Design `FOnsetSessionToken` struct (PlatformID, Platform, Expiry, HMAC Signature)
- [x] Implement `GenerateToken()` — HMAC-SHA256 payload + base64 encoding
- [x] Implement `ValidateToken()` — verify HMAC and expiry
- [x] Add `AuthTokenSecret` and `AuthTokenLifetimeSeconds` config keys
- [x] Add `Client_SessionToken(FString Token)` RPC to PlayerController
- [x] Add `Client_SessionTokenFailed(FString Reason)` RPC

## A5c.3 Login Server Target (Day 2-3)
- [x] Create `AOnsetLoginServerGameMode` — minimal: auth → token → kick
- [x] Create `Scripts/RunLoginServer.ps1` launch script
- [x] Add `[OnsetLoginServer]`, `[Onset.GameServer]` config sections
- [x] Uses existing `Onset.exe` (Server target unsupported by UE distribution)
- [x] `/Game/Maps/LoginServer` Content map — created in-editor
- [x] Fix: kick timer only fires in Token mode (Direct mode player stays until char select)
- [x] Fix: `Test_All.ps1` rewritten with interactive loop, `-NOSTEAM`, multi-client support

## A5c.4 Client & Game Server Token Flow (Day 3-4)
- [x] Game Server (TokenAuth mode): `PreLogin` extracts `?Token` → validates → caches in `PendingTokenAuthMap`
- [x] Game Server: `HandlePostLogin` reads cached platform info → loads account (existing flow)
- [x] Backward compatibility: `AuthMode=Direct` unchanged
- [x] Document replay prevention as future enhancement
- [x] Client token reconnect flow — ReconnectToGameServer() BlueprintCallable, stores token on Client_SessionToken
- [x] Fix: strip port from `GetPlayerNetworkAddress()` when matching `PendingTokenAuthMap`
- [x] Fix: travel URL uses direct IP (`IP:Port/Path?Token=`) instead of `steam://`
- [x] Fix: config lookup uses `GEngineIni` (maps to `DefaultEngine.ini`) instead of `GGameIni`

## A5c.5 Cleanup & Docs (Day 4)
- [x] Remove stale Steam auth references from `AOnsetGameModeBase`
- [x] Add `[Onset.Auth]` section to `DefaultEngine.ini` with defaults
- [x] Update `Planning/Sprint_Auth_Extraction.md` with completion notes
- [x] Create `TODO/DONE/07-27-26.md` sprint completion record
- [x] Update episode scripts — deferred (post-recording prep)

## A5c.6 Bugfix & Polish Pass
- [x] Custom SHA256 implementation — pure software, no `FGenericPlatformMisc` dependency
- [x] Fix `HandlePostLogin` empty PlatformID fallback (Direct mode, no Steam)
- [x] Fix login server kick timer — only fires in Token mode
- [x] Fix config file argument: `GGameIni` → `GEngineIni`
- [x] Fix `GameServerPort=7777` → `7778` in config
- [x] Fix travel URLs: `steam://` → direct IP
- [x] Fix port stripping in token address lookup
- [x] Fix config reading: `GEngineIni` + `-AuthMode=` command-line override
- [x] `Test_All.ps1` rewrite: `-NOSTEAM`, interactive multi-client loop

---

## A6.1 UI System
- [x] Create `UHUDWidget` (main HUD container)
- [x] Implement **virtual ability button widgets** (touch) — 4 ability hotkeys + basic attack — **clickable ability-bar slots** (`HandleSlotClicked` → `AbilityLocalInputPressed`)
- [x] Implement **virtual joystick widget** (touch) — thumbstick zone with drag-outline — **implemented (JoystickWidget.h/.cpp)**
- [x] Implement **gamepad cursor overlay widget** (software crosshair) — **implemented (GamepadCursorWidget.h/.cpp)**
- [x] Implement player health bar
- [x] Implement enemy health display — **target frame (`UTargetHUDWidget`) + static lifebar**, not world-space attached
- [x] Implement `UAbilityBarWidget` (cooldown display)
- [x] Implement targeting reticle — **ground decal** (`UpdateTargetReticle`, per-class material override)
- [x] ~~Implement debug overlay (`UAutoplayDebugWidget`)~~ — **SCRAPPED** — autoplay state feedback is carried by the `UCombatToggleWidget` toggle visual
- [x] Add PvP toggle UI element (virtual button for touch, widget for desktop)
- [x] Wire up GAS attribute changes → HUD updates
- [x] Verify health updates correctly
- [x] Verify enemy health bars appear/disappear
- [x] Verify ability cooldowns update
- [x] Verify target indicators match ability behaviour
- [x] ~~Verify debug UI toggles correctly~~ — **SCRAPPED** — no debug overlay exists
- [x] Verify virtual joystick input on touch device
- [x] Verify virtual ability buttons trigger GAS abilities
- [x] Verify gamepad cursor renders and follows R-Stick

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
- [ ] Verify all 49 risks mitigated or accepted

---

# ✅ PROGRESS TRACKING

| Section | Tasks | Done | % | Notes |
|---------|-------|------|---|-------|
| A1 Core Player | 38 | 38 | 100% | |
| A2 NPC Lifecycle | 35 | 35 | 100% | |
| A3 AI Systems | 75 | 75 | 100% | |
| A4 GAS Combat | 58 | 56 | 97% | 2 deferred — pending full design pass (attack montage, hitstop/stagger) |
| A5 Multiplayer & Steam | 35 | 35 | 100% | All waves complete (Steam auth + DS verified) |
| A5b Persistence & Account | 51 | 51 | 100% | All waves complete. Includes FHttpStore + Account API (Lambda + DynamoDB). Wave 5 revised: canvas HUDs → CommonUI screen stack. Post-sprint (08-03-26): create auto-enters world, C++-driven slots, loading screen transitions. |
| A5c Auth Extraction & Login Server | 42 | 42 | 100% | |
| A6 UI & Final Demo | 30 | 19 | 63% | A6.1 complete (target frame, ground reticle, clickable ability slots, combat toggle). Debug overlay scrapped — CombatToggle toggle visual covers autoplay feedback. A6.2/A6.3 not started. |
| A7 Integration & Harden | 23 | 0 | 0% | Not started |
