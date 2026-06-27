# 🏃 Sprint A3-A4: AI & Combat Completion

**Goal:** Complete all remaining A3 (AI Systems) + A4 (GAS Combat) tasks from `Private_Demo_Checklist.md`.
**Target:** Functional single-player combat loop — spawn, fight, die, respawn, with player abilities and full NPC behaviour.
**Estimate:** ~12 working days (Wave 4 + Wave 5 + Wave 2 + Wave 6)
**Status:** ✅ SPRINT COMPLETE — remaining 13 A4 items deferred for full combat design pass.

---

## 📊 Current State

| Section | Tasks | Done | % | Remaining |
|---------|-------|------|---|-----------|
| A3 AI Systems | 75 | 75 | 100% | 0 |
| A4 GAS Combat | 58 | 45 | 78% | 13 deferred |
| **Sprint Total** | **133** | **120** | **90%** | **13 deferred** |

### A3 Remaining (0 items) ✅ COMPLETE

All A3 items are complete. See details below.

**Threat System (9+ items — ~5.5d):** ✅ COMPLETE

**Architecture simplified:** AgroTask, ChaseTask, AttackTask, AttackPositionTask, IdleTask, RoamTask deleted. Replaced by `EngageTask` (single combat state) + `PatrolTask` (idle/roam). StateTree reduced from 9 to 6 top-level subtrees with event-driven transitions (no Selector).

- A3.6: [x] Create `UOnsetThreatSubsystem` (world subsystem, threat table)
- A3.6: [x] Wire damage → threat feed in `PostGameplayEffectExecute`
- A3.6: [x] Wire pool return + NPC death → threat cleanup
- A3.6: [x] Add threat helpers to `FOnsetStateTreeTask` base
- A3.6: [x] Create **EngageTask** — replaces Agro/Chase/Attack/AttackPosition; angular spread, AIProfile-driven ranges, crowd avoidance, target switching
- A3.6: [x] Create **PatrolTask** — 50/50 idle vs roam replaces separate IdleTask + RoamTask
- A3.6: [x] AI LOD — 3 tick tiers (full, throttled 0.2s, paused) via `UpdateLodTier`
- A3.6: [x] Sight-based threat — `OnPerceptionUpdated` adds base threat on visual contact
- A3.6: [x] `IsEnemyEngagedWithPlayer()`, `ClearFocus()` on lost target, `HasSameIndexAndSerialNumber` fix for RegisterEngaged
- A3.6: [x] Strip all debug logging from EngageTask and ThreatSubsystem
- A3.6: [x] Subsystem directory migration `Subsystems/` → `Subsystem/`
- A3.6: [x] Verify: threat drives targeting, angular spread prevents bunching

### A4 Remaining — All Deferred ⏳

The remaining 13 A4 items have been deferred pending a full combat design pass before implementation.

**Anim Montage (1 item):**
- A4.2: ~~Add animation montage support (simple melee swing)~~ ⏳ **DEFERRED**

**Stagger Effect (1 item):**
- A4.3: ~~Apply hitstop / stagger effect~~ ⏳ **DEFERRED**

**Multiple Abilities — Wave 6 (11 items):**
- A4.6: ~~Create **AoE**, **Cone**, **Shadowstep** abilities~~ ⏳ **DEFERRED**
- A4.6: ~~Wire ability slots + UI stub~~ ⏳ **DEFERRED**
- A4.6: ~~All verification items~~ ⏳ **DEFERRED**

---

## 📋 Sprint Waves

### Wave 0 — Build & Verify (~0.5d) ✅ COMPLETE
**E22 compilation + bug verification.**

Deliverables:
- [x] Project compiles with E22 changes
- [x] Perception targeting works (ApplyPerceptionProfile fix)
- [x] MovementSpeed attribute initialises from CDO defaults
- [x] Speed modifiers (flee/investigate/search) apply and stack
- [x] Pool return clears GEs — no speed leak
- [x] `bMovementSpeedInitialized` dead code removed from `OnsetBaseCharacter`

### Wave 1 — ~~A4.6 Abilities~~ ⏳ DEFERRED TO WAVE 6
**Moved to Wave 6 (after Threat System) to avoid blocking Multiplayer.**

### Wave 2 — A4.2 Anim Montage + A4.3 Stagger (~1d) ⏳ DEFERRED
**Combat polish — deferred pending full design pass.**

- [ ] ~~Simple melee swing montage~~ ⏳
- [ ] ~~Wire montage in `GA_BasicAttack`~~ ⏳
- [ ] ~~`GE_Stagger` hitstop/stagger application~~ ⏳
- [ ] ~~Verify montage + stagger~~ ⏳

### Wave 3 — A3.5 Player AI Autoplay (~2d) ✅ COMPLETE
**AI-vs-AI testing capability.**

- [x] `AOnsetPlayerAIController` — inherits `AAIController`, owns `UStateTreeAIComponent`, caches `TargetingComponent`
- [x] `UOnsetStateTreeSchema` — SelfActor + TargetActor context (same schema class as NPC)
- [x] Player AI tasks: PlayerAcquireTargetTask (navmesh-projected fence, `IsAlive()` filter) + PlayerEngageTask (combined move-to-range + attack, throttle 0.25s)
- [x] Player AI StateTree asset: AcquireTarget → Engage
- [x] `EnableAutoCombat()` / `DisableAutoCombat()` — UnPossess → possess with AI controller; camera handover via `DelayedSetViewTarget`
- [x] PvP auto-respected (targets filtered by `IsAlive()` and enemy class)
- [x] Verify: clean toggle, reasonable targets, abilities fire, target re-acquisition on death

### Wave 4 — Verification Pass (~1d) ✅ COMPLETE
**Close out all remaining unchecked items.**

- [x] A3.1: On-screen debug display for AI state — added `Tick()` to `AOnsetAIController`, draws StateTree state name
- [x] A3.2: Perception hearing triggers on assist — verified in PIE
- [x] A3.4: Assist triggers when nearby ally is attacked — Investigate→Search→Agro chain confirmed
- [x] A3.4: No assist when attacker out of hearing range — confirmed
- [x] A4.5b: Corpse spawned for both player kills and NPC-kills-NPC deaths — confirmed
- [x] A4.5b: No performance regression under rapid death cascade — confirmed
- [x] Cross-system: Player AI vs NPC AI combat test — full loop verified
- [x] Update progress tracking table in `Private_Demo_Checklist.md`
- [x] Tag all completed items

### Wave 5 — Threat System (~5.5d) ✅ COMPLETE
**Threat-driven targeting, angular spread, AI LOD.**
**Architecture simplified:** Agro, Chase, Attack, AttackPosition, Idle, Roam tasks deleted. Replaced by EngageTask + PatrolTask. StateTree reduced from 9 to 6 subtrees.

See [Threat System Doc](../Docs/AI/Threat_System.md) for full design.

- [x] Create `UOnsetThreatSubsystem` — world subsystem, threat table + engagement table
- [x] API: `AddThreat()`, `RemovePlayer()`, `RemoveEnemy()`, `GetPrimaryTarget()`, `GetBestTarget()`, `GetTargetRank()`, `GetTargetCount()`, `RegisterEngaged()`, `UnregisterEngaged()`, `GetEngagedCount()`, `GetEngagedIndex()`, `SwitchTarget()`, `IsEnemyEngagedWithPlayer()`, `ClearAll()`
- [x] Wire damage feed: `OnsetAttributeSet::PostGameplayEffectExecute` → `Subsystem->AddThreat(Instigator, Victim, Damage)`
- [x] Wire cleanup: `OnsetEnemy::DeferredDeathCleanup` + `PoolSubsystem::ReturnToPool` → `Subsystem->RemoveEnemy(NPC)`
- [x] Add helpers to `FOnsetStateTreeTask`: `GetThreatSubsystem()`, `GetThreatAngularOffset(Count, Rank, Radius)` → `FVector`
- [x] Create **EngageTask** — single combat state replacing Agro/Chase/Attack/AttackPosition:
  - `EnterState`: reads `AttackRange`/`ChaseRange` from `UAIProfile`, gets best target, registers engagement, sets focus, computes angular offset, paths to it
  - `Tick`: target re-eval every 1s via `GetBestTarget()`, position re-eval every 3s or 200u move, angular offset at `SpreadRadius` (chase) or `AttackRange` (attack), dead zone 50u, crowd avoidance, abilities fire at throttle, 2s timeout on lost target
  - `ExitState`: `StopMovement()`
- [x] Create **PatrolTask** — 50/50 idle vs roam in one task (replaces IdleTask + RoamTask)
- [x] AI LOD: `UpdateLodTier` in `AOnsetAIController` — 3 tiers: full tick within sight, throttle 0.2s within hearing, StateTree paused beyond
- [x] Sight-based threat: `OnPerceptionUpdated` adds 1.0 threat on visual contact if not already engaged
- [x] Debug logging stripped from both EnemyEngageTask and OnsetThreatSubsystem
- [x] StateTree: 6 top-level subtrees with event-driven transitions (Patrol, Engage, Investigate, Search, Flee, Lost)
- [x] Subsystem directory migrated `Subsystems/` → `Subsystem/`
- [x] Corpses ignore pawn collision (ECC_Pawn → Ignore)
- [x] Verify: threat drives targeting, angular spread prevents bunching, LOD stops far NPC work

### Wave 6 — A4.6 Multiple Abilities (~4d) ⏳ DEFERRED
**Core player combat — AoE, Cone, Shadowstep, ability bar UI.**
Deferred pending full design pass on combat abilities, progression, and class system.

- [ ] ~~AoE ability (C++ GA + BP + slot 1)~~ ⏳
- [ ] ~~Cone ability (C++ GA + BP + slot 2)~~ ⏳
- [ ] ~~Shadowstep passive (on-kill blink, cooldown-gated)~~ ⏳
- [ ] ~~Ability bar UI stub~~ ⏳
- [ ] ~~All verification items~~ ⏳

---

## ⚠ Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| AoE volume perf with many enemies | Perf drop | Medium | Use simple sphere overlap, not spawned actors; delay/rate-limit checks |
| Shadowstep target resolution edge cases | Buggy | Medium | Log target selection; fallback to no-teleport if no valid target |
| StateTree task regression from refactoring | Broken AI | Low | Test each NPC state after E22 changes before starting Wave 1 |
| Montage integration with GAS activation | GA fails to play anim | Medium | Test independently — wire montage first, then integrate with GA |
| Controller swap desyncs ASC state | Broken abilities | Low | Test `GrantDefaultAbilities` guard (`bAbilitiesGranted` already exists) |
| Camera handover on controller swap | Black screen | Low | Use `DelayedSetViewTarget` — defer one frame after `UnPossess` to avoid race |
| Threat angular spread ignores navmesh blockers | NPC stuck running into wall | Low | Nav-project offset position before `MoveToLocation` |
| Threat table grows unbounded on long sessions | Memory leak | Low | `TWeakObjectPtr` auto-clears dead NPCs; `ClearAll()` on level transition |
| Threat override breaks perception-driven assist | Assist never fires | Medium | Threat only overrides target when threat > 0; empty threat = perception fallback |

---

## 📐 Design Decisions for This Sprint

**AoE volume:** Use `UWorld::OverlapMultiByChannel` (sphere) on activation, don't spawn a persistent actor. Simpler, cheaper, no clean-up needed. Visual feedback via VFX in BP child.

**Cone shape:** `UKismetSystemLibrary::ConeOverlapActors` — built-in, handles `FCollisionQueryParams`. If perf concerns, switch to repeated line traces.

**Shadowstep target:** Nearest alive enemy by straight-line distance within gate range, not navpath. Teleport to `Target->GetActorLocation() + Target->GetActorForwardVector() * -BehindOffset`. If no valid target, don't teleport (ability "fizzles").

**Controller swap viability:** Both `AOnsetPlayerController` and `AOnsetPlayerAIController` cache `TargetingComponent` on `OnPossess`. Pawn side `GrantDefaultAbilities` has a `bAbilitiesGranted` guard — safe to call on multiple possessions. The swap is transparent to the pawn.

**Camera handover:** `DelayedSetViewTarget` defers camera assignment by one frame after `UnPossess` to avoid a race with `PlayerCameraManager` cleanup. Called on the player controller after re-possession.

**Combined EngageTask (NPC):** Single combat state replacing Agro/Chase/Attack/AttackPosition. `Tick` checks distance: within attack range → `StopMovement` + `SetFocus` + fire abilities at throttle; outside → path to angular offset position. `EnterState` reads ranges from `UAIProfile`, registers engagement, sets initial offset. `ExitState` clears focus + stops movement. Angular offset uses `SpreadRadius` (chase) or `AttackRange` (attack) depending on current distance. Crowd avoidance via `UCrowdFollowingComponent`. 2s timeout on sustained absence of target before exiting.

**Null guards on GetTarget/SetTarget:** Both guard against missing `TargetingComponent` — returns null target instead of crashing.

**Threat subsystem over component:** `UOnsetThreatSubsystem` (world subsystem) instead of per-NPC `UThreatComponent`. Zero per-NPC allocation, single map for pool/player cleanup, server-only data.

**Threat table structure:** `TMap<TWeakObjectPtr<AOnsetEnemy>, TMap<TWeakObjectPtr<AOnsetBaseCharacter>, float>>`. Outer key by enemy for fast `RemoveEnemy()`. `TWeakObjectPtr` for both keys so dead actors don't leak. Separate engagement table: `TMap<TWeakObjectPtr<AOnsetBaseCharacter>, TArray<TWeakObjectPtr<AOnsetEnemy>>>` for angular spread computation.

**Angular spread:** `angle = (rank / count) * 360°` at `AttackRange` radius. Nav-projected. Re-evaluated on timer (3s) or when target moves > 200 units. Single enemy → rank 0, count 1 → angle 0° → directly in front (correct).

**Threat + perception coexistence:** Perception feeds `TargetingComponent` (sight) and noise data (hearing) as before. Threat `GetPrimaryTarget()` is checked first by AgroTask. If non-null, it overrides the perception target. If null, perception target is used. Assist still flows through hearing events.

**AI LOD tiers:** `AOnsetAIController` uses `SetComponentTickInterval()` per NPC based on distance to nearest player. Three tiers: full tick (< `SightRange`), throttled (0.2s up to `ChaseRange * 2`), paused (beyond). StateTree stops ticking first; pathfinding naturally quiesces.
