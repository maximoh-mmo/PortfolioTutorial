# 🏃 Sprint A3-A4: AI & Combat Completion

**Goal:** Complete all remaining A3 (AI Systems) + A4 (GAS Combat) tasks from `Private_Demo_Checklist.md`.
**Target:** Functional single-player combat loop — spawn, fight, die, respawn, with player abilities and full NPC behaviour.
**Estimate:** ~12 working days (Wave 4 + Wave 5 + Wave 2 + Wave 6)

---

## 📊 Current State

| Section | Tasks | Done | % | Remaining |
|---------|-------|------|---|-----------|
| A3 AI Systems | 75 | 69 | 92% | 6 |
| A4 GAS Combat | 58 | 45 | 78% | 13 |
| **Sprint Total** | **133** | **114** | **86%** | **19** |

### A3 Remaining (6 items)

**Verification (4 items — ~0.5d):** ✅ COMPLETE
- A3.1: [x] Add on-screen debug display for current AI state
- A3.2: [x] Verify perception hearing triggers on assist
- A3.4: [x] Verify assist triggers when nearby ally is attacked
- A3.4: [x] Verify no assist when attacker is out of hearing range

**Threat System (9 items — ~5.5d) — NEW:**
See [Threat System](../Docs/AI/Threat_System.md) for full design.
- A3.6: [ ] Create `UOnsetThreatSubsystem` (world subsystem, threat table)
- A3.6: [ ] Wire damage → threat feed in `PostGameplayEffectExecute`
- A3.6: [ ] Wire pool return + NPC death → threat cleanup
- A3.6: [ ] Add threat helpers to `FOnsetStateTreeTask` base
- A3.6: [ ] Modify AgroTask to prefer threat target over perception
- A3.6: [ ] Create AttackPositionTask with angular spread
- A3.6: [ ] Modify ChaseTask offset to use threat
- A3.6: [ ] AI LOD — disable/ throttle NPC ticks based on distance
- A3.6: [ ] Verify: threat drives targeting, angular spread prevents bunching

### A4 Remaining (13 items)

**Anim Montage (1 item — ~0.5d):**
- A4.2: [ ] Add animation montage support (simple melee swing)

**Stagger Effect (1 item — ~0.5d):**
- A4.3: [ ] Apply hitstop / stagger effect

**Multiple Abilities — Wave 6 (11 items — ~4d):**
- A4.6: [ ] Create **AoE** ability (target-centered damage volume) — activated GA, slot 1
- A4.6: [ ] Create **Cone** ability (directional frontal cone) — activated GA, slot 2
- A4.6: [ ] Create **Shadowstep** passive (on-kill blink behind nearest enemy, cooldown-gated) — replaces dash
- A4.6: [ ] Wire `OnAbility1` → AoE, `OnAbility2` → Cone in PlayerController
- A4.6: [ ] Create ability bar UI stub (for testing)
- A4.6: [ ] Verify all abilities activate from input
- A4.6: [ ] Verify AoE respects PvP rules (per-target filtering)
- A4.6: [ ] Verify Cone respects PvP rules (per-target filtering)
- A4.6: [ ] Verify Shadowstep triggers on kill
- A4.6: [ ] Verify Shadowstep respects distance gate
- A4.6: [ ] Verify Shadowstep cooldown prevents spam

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

### Wave 2 — A4.2 Anim Montage + A4.3 Stagger (~1d)
**Combat polish.**

- [ ] Simple melee swing montage (`Content/Game/Combat/AM_MeleeSwing`)
- [ ] Wire montage in `GA_BasicAttack` — `PlayMontageOnActivate`
- [ ] `GE_Stagger` hitstop/stagger application in `GA_HitReaction`
- [ ] Verify montage plays on attack, stagger animates on hit

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

### Wave 5 — Threat System (~5.5d)
**Threat-driven targeting, angular spread, AI LOD.**

See [Threat System Doc](../Docs/AI/Threat_System.md) for full design.

- [ ] Create `UOnsetThreatSubsystem` — world subsystem, `TMap<APlayerState*, TMap<TWeakObjectPtr<AOnsetEnemy>, float>>` threat table
- [ ] API: `AddThreat()`, `RemovePlayer()`, `RemoveEnemy()`, `GetPrimaryTarget()`, `GetTargetRank()`, `GetTargetCount()`, `GetEnemiesTargeting()`, `ClearAll()`
- [ ] Wire damage feed: `OnsetAttributeSet::PostGameplayEffectExecute` → if damage > 0 to `AOnsetEnemy` → `Subsystem->AddThreat(InstigatorPlayerState, Victim, Damage)`
- [ ] Wire cleanup: `OnsetEnemy::DeferredDeathCleanup` + `PoolSubsystem::ReturnToPool` → `Subsystem->RemoveEnemy(NPC)`
- [ ] Add helpers to `FOnsetStateTreeTask`: `GetThreatSubsystem()`, `GetThreatAngularOffset(Count, Rank, Radius)` → `FVector`
- [ ] Modify `AgroTask::Tick` — check `GetPrimaryTarget()` first, fall back to `GetTarget()`
- [ ] Modify `ChaseTask::EnterState` — replace random lateral offset with threat angular position at `ChaseRange`
- [ ] Create `AttackPositionTask` — compute angular offset at `AttackRange` via threat rank, nav-project, `MoveToLocation`, re-evaluate at 3s or on target move > 200 units. Fire abilities at throttle.
- [ ] AI LOD: `AOnsetAIController` tick interval tiers based on distance to nearest player. Far NPCs tick at 0.5s or paused.
- [ ] Verify: threat target = highest damage dealer, angular spread prevents bunching, LOD stops far NPC work

### Wave 6 — A4.6 Multiple Abilities (~4d)
**Core player combat — AoE, Cone, Shadowstep, ability bar UI.**

Previously deferred from Wave 1; moved here to avoid blocking Multiplayer. Delivered after Threat so NPC threats/positioning are stable.

Wave 6a — AoE (day 1–2):
- [ ] `GA_OnsetAoE.h/.cpp` — C++ GA
- [ ] Implementation: sphere overlap at target location on activate, apply GE to overlapping enemies, destroy after delay
- [ ] PvP filter per target
- [ ] BP asset `Content/Game/Combat/GA_AoE` (tune radius, damage, cooldown)
- [ ] Slot 1 binding
- [ ] Verify: activates from input, damages enemies, respects PvP

Wave 6b — Cone (day 2–3):
- [ ] `GA_OnsetCone.h/.cpp` — C++ GA
- [ ] Implementation: `ConeOverlapActors` from character forward, apply GE to hit enemies
- [ ] PvP filter per target
- [ ] BP asset `Content/Game/Combat/GA_Cone` (tune angle, range, damage, cooldown)
- [ ] Slot 2 binding
- [ ] Verify: activates from input, damages in correct shape, respects PvP

Wave 6c — Shadowstep (day 3–4):
- [ ] `GA_OnsetShadowstep.h/.cpp` — C++ GA (passive, triggered by `FGameplayEvent` on enemy death)
- [ ] Implementation: on kill, find nearest enemy within distance gate, teleport behind, brief invulnerability window
- [ ] Cooldown gating
- [ ] BP asset `Content/Game/Combat/GA_Shadowstep`
- [ ] Verify: triggers on kill, respects distance gate, cooldown prevents spam

Wave 6d — UI Stub (day 4):
- [ ] Basic ability bar widget (4 slot icons, cooldown overlay text)
- [ ] Bind to GAS cooldown tags
- [ ] Verify abilities display and cooldowns update

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

**Combined EngageTask:** Movement and ability usage live in one task, not split Chase→Attack. `Tick` checks distance: within attack range → `StopMovement` + `SetFocus` + fire abilities at throttle (0.25s); outside → `MoveToActor`. `EnterState` validates `IsAlive()` and clears dead targets. `ExitState` clears focus + stops movement.

**Null guards on GetTarget/SetTarget:** Both guard against missing `TargetingComponent` — returns null target instead of crashing.

**Threat subsystem over component:** `UOnsetThreatSubsystem` (world subsystem) instead of per-NPC `UThreatComponent`. Zero per-NPC allocation, single map for pool/player cleanup, server-only data.

**Threat table structure:** `TMap<APlayerState*, TMap<TWeakObjectPtr<AOnsetEnemy>, float>>`. Outer key by player for fast `RemovePlayer()`. `TWeakObjectPtr` for inner key so dead NPCs don't leak. AddThreat sorts the inner map for O(1) rank reads.

**Angular spread:** `angle = (rank / count) * 360°` at `AttackRange` radius. Nav-projected. Re-evaluated on timer (3s) or when target moves > 200 units. Single enemy → rank 0, count 1 → angle 0° → directly in front (correct).

**Threat + perception coexistence:** Perception feeds `TargetingComponent` (sight) and noise data (hearing) as before. Threat `GetPrimaryTarget()` is checked first by AgroTask. If non-null, it overrides the perception target. If null, perception target is used. Assist still flows through hearing events.

**AI LOD tiers:** `AOnsetAIController` uses `SetComponentTickInterval()` per NPC based on distance to nearest player. Three tiers: full tick (< `SightRange`), throttled (0.2s up to `ChaseRange * 2`), paused (beyond). StateTree stops ticking first; pathfinding naturally quiesces.
