# 🏃 Sprint A3-A4: AI & Combat Completion

**Goal:** Complete all remaining A3 (AI Systems) + A4 (GAS Combat) tasks from `Private_Demo_Checklist.md`.
**Target:** Functional single-player combat loop — spawn, fight, die, respawn, with player abilities and full NPC behaviour.
**Estimate:** ~10 working days

---

## 📊 Current State

| Section | Tasks | Done | % | Remaining |
|---------|-------|------|---|-----------|
| A3 AI Systems | 66 | 54 | 82% | 12 |
| A4 GAS Combat | 58 | 43 | 74% | 15 |
| **Sprint Total** | **124** | **97** | **78%** | **27** |

### A3 Remaining (12 items)

**Verification (4 items — ~0.5d):**
- A3.1: [ ] Add on-screen debug display for current AI state
- A3.2: [ ] Verify perception hearing triggers on assist
- A3.4: [ ] Verify assist triggers when nearby ally is attacked
- A3.4: [ ] Verify no assist when attacker is out of hearing range

**Player AI Autoplay (8 items — ~2d):**
- A3.5: [ ] Create `APlayerAIController` class
- A3.5: [ ] Create `UPlayerAIStateTreeComponent`
- A3.5: [ ] Implement player AI StateTree: Idle → SeekTarget → MoveToTarget → Attack
- A3.5: [ ] Implement `EnableAutoplay(bool)` — possession switching
- A3.5: [ ] Ensure PvP rules respected (ignore players when PvP OFF)
- A3.5: [ ] Verify clean toggle on/off
- A3.5: [ ] Verify AI picks reasonable targets
- A3.5: [ ] Verify abilities fire correctly under AI control

### A4 Remaining (15 items)

**Anim Montage (1 item — ~0.5d):**
- A4.2: [ ] Add animation montage support (simple melee swing)

**Stagger Effect (1 item — ~0.5d):**
- A4.3: [ ] Apply hitstop / stagger effect

**Corpse Verification (2 items — ~0.5d):**
- A4.5b: [ ] Verify corpse spawned for player kills and NPC-kills-NPC deaths
- A4.5b: [ ] Verify no performance regression under rapid death cascade

**Multiple Abilities (11 items — ~4d):**
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

### Wave 0 — Build & Verify (~0.5d)
**E22 compilation + bug verification.**

Deliverables:
- [ ] Project compiles with E22 changes
- [ ] Perception targeting works (ApplyPerceptionProfile fix)
- [ ] MovementSpeed attribute initialises from CDO defaults
- [ ] Speed modifiers (flee/investigate/search) apply and stack
- [ ] Pool return clears GEs — no speed leak
- [ ] `bMovementSpeedInitialized` dead code removed from `OnsetBaseCharacter`

### Wave 1 — A4.6 Abilities (~4d)
**Core player combat — AoE, Cone, Shadowstep.**

Wave 1a — AoE (day 1–2):
- [ ] `GA_OnsetAoE.h/.cpp` — C++ GA
- [ ] Implementation: spawn temporary overlap volume at target location on activate, apply GE to overlapping enemies, destroy after delay
- [ ] PvP filter per target
- [ ] BP asset `Content/Game/Combat/GA_AoE` (tune radius, damage, cooldown)
- [ ] Slot 1 binding in PlayerController
- [ ] Verify: activates from input, damages enemies, respects PvP

Wave 1b — Cone (day 2–3):
- [ ] `GA_OnsetCone.h/.cpp` — C++ GA
- [ ] Implementation: trace/fan cone from character forward on activate, apply GE to hit enemies
- [ ] PvP filter per target
- [ ] BP asset `Content/Game/Combat/GA_Cone` (tune angle, range, damage, cooldown)
- [ ] Slot 2 binding in PlayerController
- [ ] Verify: activates from input, damages in correct shape, respects PvP

Wave 1c — Shadowstep (day 3–4):
- [ ] `GA_OnsetShadowstep.h/.cpp` — C++ GA (passive, triggered by `FGameplayEvent` on enemy death)
- [ ] Implementation: on kill, find nearest enemy within distance gate, teleport behind, brief invulnerability window
- [ ] Cooldown gating (cannot chain kills)
- [ ] BP asset `Content/Game/Combat/GA_Shadowstep`
- [ ] Verify: triggers on kill, respects distance gate, cooldown prevents spam

Wave 1d — UI Stub (day 4):
- [ ] Basic ability bar widget (4 slot icons, cooldown overlay text)
- [ ] Verify abilities display and cooldowns update

### Wave 2 — A4.2 Anim Montage + A4.3 Stagger (~1d)
**Combat polish.**

- [ ] Simple melee swing montage (`Content/Game/Combat/AM_MeleeSwing`)
- [ ] Wire montage in `GA_BasicAttack` — `PlayMontageOnActivate`
- [ ] `GE_Stagger` hitstop/stagger application in `GA_HitReaction`
- [ ] Verify montage plays on attack, stagger animates on hit

### Wave 3 — A3.5 Player AI Autoplay (~2d)
**AI-vs-AI testing capability.**

- [ ] `APlayerAIController` — inherits `AAIController`, owns `UStateTreeAIComponent`, caches `TargetingComponent`
- [ ] `UPlayerAIStateTreeSchema` — SelfActor + TargetActor context
- [ ] Player AI tasks: FindTargetTask (C++ navpath-cost loop), MoveToTask, AttackTask
- [ ] Player AI StateTree asset: Idle → FindTarget → MoveToTarget → Attack
- [ ] `EnableAutoplay(bool)` — UnPossess from player controller → Possess with AI controller
- [ ] PvP auto-disabled during autoplay (target class filter = `AOnsetEnemy` only)
- [ ] Verify: clean toggle, reasonable targets, abilities fire, target re-acquisition on death

### Wave 4 — Verification Pass (~1d)
**Close out all remaining unchecked items.**

- [ ] A3.1: On-screen debug display for AI state (simple string on NPC or on controller)
- [ ] A3.2: Verify perception hearing triggers on assist
- [ ] A3.4: Verify assist triggers when nearby ally is attacked
- [ ] A3.4: Verify no assist when attacker out of hearing range
- [ ] A4.5b: Verify corpse spawned for both player kills and NPC-kills-NPC deaths
- [ ] A4.5b: Verify no performance regression under rapid death cascade
- [ ] Cross-system: Player AI vs NPC AI combat test (A3.5 + A3.3)
- [ ] Update progress tracking table in `Private_Demo_Checklist.md`
- [ ] Tag all completed items

---

## ⚠ Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| AoE volume perf with many enemies | Perf drop | Medium | Use simple sphere overlap, not spawned actors; delay/rate-limit checks |
| Shadowstep target resolution edge cases | Buggy | Medium | Log target selection; fallback to no-teleport if no valid target |
| StateTree task regression from refactoring | Broken AI | Low | Test each NPC state after E22 changes before starting Wave 1 |
| Montage integration with GAS activation | GA fails to play anim | Medium | Test independently — wire montage first, then integrate with GA |
| Controller swap desyncs ASC state | Broken abilities | Low | Test `GrantDefaultAbilities` guard (`bAbilitiesGranted` already exists) |

---

## 📐 Design Decisions for This Sprint

**AoE volume:** Use `UWorld::OverlapMultiByChannel` (sphere) on activation, don't spawn a persistent actor. Simpler, cheaper, no clean-up needed. Visual feedback via VFX in BP child.

**Cone shape:** `UKismetSystemLibrary::ConeOverlapActors` — built-in, handles `FCollisionQueryParams`. If perf concerns, switch to repeated line traces.

**Shadowstep target:** Nearest alive enemy by straight-line distance within gate range, not navpath. Teleport to `Target->GetActorLocation() + Target->GetActorForwardVector() * -BehindOffset`. If no valid target, don't teleport (ability "fizzles").

**Controller swap viability:** Both `AOnsetPlayerController` and `APlayerAIController` cache `TargetingComponent` on `OnPossess`. Pawn side `GrantDefaultAbilities` has a `bAbilitiesGranted` guard — safe to call on multiple possessions. The swap should be transparent to the pawn.
