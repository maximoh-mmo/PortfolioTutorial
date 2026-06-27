# 📅 PRODUCTION TIMELINE
**File:** `Planning/Production_Timeline.md`

Estimated timeline for the Top-Down ARPG AI Demo tutorial series.
Assumes a single developer working full-time on the project.

---

# 🧱 OVERVIEW

The project follows a two-phase approach:

1. **Phase A — Private Demo** (build all systems off-camera first)
2. **Phase B — Episode Production** (record, edit, export each episode from the working demo)

This ensures no broken episode dependencies, no rewrites, and no recording dead-ends.

---

### Multi-Device Input Note

The project targets **touch as primary input** (virtual joystick, tap-to-move, virtual buttons), with mouse/keyboard and gamepad as testing/fallback. Enhanced Input with per-device Mapping Contexts is established in A1.1. A unified cursor abstraction layer feeds all raycast systems regardless of device.

The additional work over a single-device approach:

- **~1 day** — Enhanced Input architecture + cursor abstraction (A1.1)
- **~1.5 days** — Multi-device movement system (A1.3): virtual joystick, tap-to-move, WASD, gamepad stick + R-Stick cursor
- **~1.5 days** — Touch/gamepad UI widgets (A6.1): virtual joystick, ability buttons, gamepad cursor overlay
- **~0.5 day** — Mobile performance tuning (A6.3)

**Total impact:** ~+4.5 days to Phase A (already reflected in per-system estimates below).

---

# 🔷 PHASE A: PRIVATE DEMO DEVELOPMENT

Goal: Build all 13 systems and the final demo loop. Each system is implemented, tested, and stable before any recording begins.

## Phase A1 — Core Player Systems
| System | Est. Days | Dependencies |
|--------|-----------|--------------|
| Project Setup + Base Classes + Enhanced Input | 1.5 | None |
| Top-Down Camera | 1 | Project Setup |
| Movement System (joystick + tap + WASD + gamepad) | 1.5 | Camera |
| Targeting Component | 1 | Movement System |
| Ability Targeting (AoE, directional) | 2 | Targeting |
| PvP Toggle | 1 | Targeting |
| **Subtotal** | **8 days** | |

## Phase A2 — NPC Lifecycle
| System | Est. Days | Dependencies |
|--------|-----------|--------------|
| NPC Character + Spawner | 2 | Core Player Systems |
| Object Pooling | 2 | Spawner |
| Group System | 2 | Spawner |
| **Subtotal** | **6 days** | |

## Phase A3 — AI Systems
| System | Est. Days | Dependencies |
|--------|-----------|--------------|
| StateTree Setup + Schema | 2 | NPC Character |
| AI Perception | 1 | StateTree |
| Behaviour States (Idle → Flee) | 4 | Perception |
| Group Assist Integration | 1 | StateTree, Group |
| Player AI Autoplay | 2 | StateTree, Targeting |
| Threat System (threat table, angular spread, AI LOD) | 5.5 | StateTree, Targeting, GAS |
| **Subtotal** | **15.5 days** | |

## Phase A4 — GAS Combat
| System | Est. Days | Dependencies |
|--------|-----------|--------------|
| GAS Setup (ASC, AttributeSet, Tags) | 2 | Player System |
| Basic Attack Ability | 1 | GAS Setup |
| Hit Reaction Ability | 1 | Basic Attack |
| NPC Attack Integration | 1 | GAS, StateTree |
| Damage + Death + Pool Return (A4.5a) | 1.5 | GAS, Pooling |
| Corpse Actor System (A4.5b) | 1 | GAS, Pooling |
| Multiple Abilities (AoE, Cone, Shadowstep, Montage, Stagger) | 5 | GAS Setup |
| **Subtotal** | **10.5 days** | |

> Note: Multiple Abilities (5 days) deferred to post-A5 — pending full combat design pass.
> A4 actual completed: ~5.5 days (GAS Setup, Basic Attack, Hit Reaction, NPC Attack, Damage/Death, Corpse).

## Phase A5 — Multiplayer & Steam
| System | Est. Days | Dependencies |
|--------|-----------|--------------|
| Server/Client Authority Setup | 2 | All systems |
| Replication Pass (NPCs, Abilities, Targeting) | 3 | Authority Setup |
| Dedicated Server Build + Test | 2 | Replication |
| Steam Auth Integration | 3 | Multiplayer |
| **Subtotal** | **10 days** | |

## Phase A6 — UI & Final Demo
| System | Est. Days | Dependencies |
|--------|-----------|--------------|
| UI System (Health, Cooldowns, Target Highlight, touch widgets, gamepad cursor) | 4 | GAS, Targeting |
| Final Demo Loop (Waves, Respawn, Combat Flow) | 2 | All systems |
| Performance Pass + Polish | 2 | Final Demo |
| **Subtotal** | **8 days** | |

## Phase A7 — Integration & Buffer
| Activity | Est. Days |
|----------|-----------|
| Cross-system bugfixing | 5 |
| Edge case hardening | 3 |
| Testing pass | 3 |
| **Subtotal** | **11 days** |

## Phase A Total
| Section | Days |
|---------|------|
| A1 — Core Player (incl. multi-device input) | 8 |
| A2 — NPC Lifecycle | 6 |
| A3 — AI Systems | 15.5 |
| A4 — GAS Combat | 10 |
| A5 — Multiplayer & Steam | 10 |
| A6 — UI & Final Demo (incl. touch/gamepad widgets) | 8 |
| A7 — Integration & Buffer | 11 |
| **Total** | **~68.5 days (~14 weeks)** *(10 days A5 remaining, 5 days A4 deferred)* |

---

# 🔷 PHASE B: EPISODE PRODUCTION

Each episode requires:
- **Script prep** (if not yet drafted)
- **Recording** (1-2 hours)
- **Editing** (2-3 hours)
- **Export + README** (1 hour)

## B1 — Scripting (Episodes 6-43)

Episodes 1-3 need script updates for Enhanced Input + multi-device movement. Episodes 4-5 need re-ordering. Remaining episodes need scripts from scratch:

| Batch | Episodes | Est. Hours Each | Total Hours |
|-------|----------|-----------------|-------------|
| Scripts 1-3 (rewrite) | 3 | 3 | 9 |
| Scripts 4-5 (reorder) | 2 | 1 | 2 |
| Scripts 6-13 | 8 | 3 | 24 |
| Scripts 14-20 | 7 | 4 | 28 |
| Scripts 21-22 | 2 | 3 | 6 |
| Scripts 23-25 | 3 | 4 | 12 |
| Scripts 26-28 (Threat System) | 3 | 4 | 12 |
| Scripts 29-33 | 5 | 3 | 15 |
| Scripts 34-39 | 6 | 3 | 18 |
| Scripts 40-43 | 4 | 3 | 12 |
| **Total** | **43 scripts** | | **~136 hours (3.4 weeks)** |

## B2 — Recording + Editing + Export

Estimated per episode:

| Activity | Hours |
|----------|-------|
| Recording (with retakes) | 2 |
| Editing | 2.5 |
| Export + README | 1 |
| Buffer | 0.5 |
| **Total per episode** | **6 hours** |

| Phase | Episodes | Total Hours |
|-------|----------|-------------|
| Phase 0: Player Core | 1-5 | 30 |
| Phase 1: NPC Lifecycle | 6-8 | 18 |
| Phase 2: AI Foundations | 9-13 | 30 |
| Phase 3: GAS Combat | 14-20 | 48 |
| Phase 3.5: Architecture Cleanup | 21-22 | 12 |
| Phase 3.6: Player AI | 23-25 | 18 |
| Phase 3.7: Threat System | 26-28 | 18 |
| Phase 4: Advanced AI (incl. Group Assist) | 29-33 | 30 |
| Phase 5: Multiplayer (incl. PvP) | 34-39 | 36 |
| Phase 6: Final Demo | 40-43 | 24 |
| **Total** | **43 episodes** | **~258 hours (6.45 weeks)** |

## Phase B Total
| Activity | Time |
|----------|------|
| Scripting (rewrites + new) | 3.4 weeks |
| Recording + Editing + Export | 6.45 weeks |
| **Total** | **~9.85 weeks** |

---

# 📊 FULL PROJECT SUMMARY

| Phase | Duration | Notes |
|-------|----------|-------|
| ✅ Planning (Phases 1-5) | Complete | Outlines, docs, scripts 1-5, workflow |
| ✅ Pre-Production Review | Complete | Consistency scan, risk docs, timeline |
| **Phase A: Private Demo** | **~15 weeks** | Build all systems off-camera (incl. multi-device input, corpse system, threat system) |
| **Phase B: Episode Production** | **~10 weeks** | Record, edit, export 43 episodes (~136h scripting + ~258h recording) |
| **Total Remaining** | **~25 weeks (~6 months)** | |

## Parallel Work

Scripting (Phase B1) can overlap with Phase A development:
- Script rewriting + Episodes 6-13 during Phase A2/A3 development
- Script Episodes 14-26 during Phase A4/A5 development
- Script Episodes 27-43 during Phase A6/A7 development

This reduces total calendar time:

| Scenario | Calendar Time |
|----------|---------------|
| Sequential (Phase A → Phase B) | ~26 weeks |
| **With script overlap** | **~22 weeks (~5.5 months)** |

## Weekly Cadence (for release)

If releasing one episode per week:

| Phase | Episodes | Release Cadence | Duration |
|-------|----------|-----------------|----------|
| Phase 0: Player Core | 1-5 | 1/week | 5 weeks |
| Phase 1: NPC Lifecycle | 6-8 | 1/week | 3 weeks |
| Phase 2: AI Foundations | 9-13 | 1/week | 5 weeks |
| Phase 3: GAS Combat | 14-20 | 1/week | 7 weeks |
| Phase 3.5: Architecture Cleanup | 21-22 | 1/week | 2 weeks |
| Phase 3.6: Player AI | 23-25 | 1/week | 3 weeks |
| Phase 3.7: Threat System | 26-28 | 1/week | 3 weeks |
| Phase 4: Advanced AI | 29-33 | 1/week | 5 weeks |
| Phase 5: Multiplayer | 34-39 | 1/week | 6 weeks |
| Phase 6: Final Demo | 40-43 | 1/week | 4 weeks |
| **Total release window** | **43 episodes** | **1/week** | **43 weeks (~10.75 months)** |

The ~3-month private demo build + ~9.5-month release window means a **~12.5-month project** from start to finish of the last episode release. Batch-recording (recording multiple episodes per week) compresses the production phase but not the release schedule.

---

# ⚡ KEY MILESTONES

| Milestone | Target | What's True |
|-----------|--------|-------------|
| Private demo playable | Week 8 | Core player + NPC + basic combat working |
| Private demo complete | Week 15.5 | All systems, corpse system, multiplayer, Steam, final loop |
| First episode released | Week 14 | Episode 1 on public repo |
| Targeting + enemies milestone | Week 18 | Episodes 1-5 released (playable combat) |
| AI milestone | Week 26 | Episodes 9-13 released (full NPC AI) |
| GAS milestone | Week 33 | Episodes 14-20 released (full ability system) |
| Architecture cleanup | Week 35 | Episodes 21-22 released (interaction component, profile split, GAS speed attribute) |
| Player AI milestone | Week 38 | Episodes 23-25 released (autoplay mode) |
| Threat System milestone | Week 41 | Episodes 26-28 released (threat, angular spread, AI LOD) |
| Advanced AI milestone | Week 46 | Episodes 29-33 released (group assist, advanced behaviour) |
| Multiplayer milestone | Week 52 | Episodes 34-39 released (MP + Steam + PvP) |
| Series complete | Week 56 | Episode 43 (Final Showcase) released — 43rd release |

---

**Total estimated effort:** ~21 weeks of work, ~12.5 months calendar (with weekly release cadence; 43 episodes over 43 weeks)
**Next step:** Continue Private Demo Development
