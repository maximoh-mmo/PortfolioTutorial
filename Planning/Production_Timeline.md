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

### Mobile / Touch Input Note

All core systems are designed with **dual input** (mouse + touch) from the start. UE's raycast pipeline handles both inputs identically, so the architecture is unaffected. The additional work is limited to:

- **~0.5 day** — Unified input action bindings (A1.3)
- **~1 day** — Touch-friendly UI controls (A6.1)
- **~0.5 day** — Mobile performance tuning (A6.3)

**Total impact:** ~+2 days to Phase A. No changes to episode count, system architecture, or release cadence.

---

# 🔷 PHASE A: PRIVATE DEMO DEVELOPMENT

Goal: Build all 13 systems and the final demo loop. Each system is implemented, tested, and stable before any recording begins.

## Phase A1 — Core Player Systems
| System | Est. Days | Dependencies |
|--------|-----------|--------------|
| Project Setup + Base Classes | 1 | None |
| Top-Down Camera | 1 | Project Setup |
| Click-to-Move | 1 | Camera |
| Targeting Component | 1 | Click-to-Move |
| Ability Targeting (AoE, directional) | 2 | Targeting |
| PvP Toggle | 1 | Targeting |
| **Subtotal** | **7 days** | |

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
| **Subtotal** | **10 days** | |

## Phase A4 — GAS Combat
| System | Est. Days | Dependencies |
|--------|-----------|--------------|
| GAS Setup (ASC, AttributeSet, Tags) | 2 | Player System |
| Basic Attack Ability | 1 | GAS Setup |
| Hit Reaction Ability | 1 | Basic Attack |
| NPC Attack Integration | 1 | GAS, StateTree |
| Damage + Death + Pool Return | 2 | GAS, Pooling |
| Multiple Abilities (Dash, AoE, Projectile) | 3 | GAS Setup |
| **Subtotal** | **10 days** | |

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
| UI System (Health, Cooldowns, Target Highlight) | 3 | GAS, Targeting |
| Final Demo Loop (Waves, Respawn, Combat Flow) | 2 | All systems |
| Performance Pass + Polish | 2 | Final Demo |
| **Subtotal** | **7 days** | |

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
| A1 — Core Player (incl. touch input) | 7 |
| A2 — NPC Lifecycle | 6 |
| A3 — AI Systems | 10 |
| A4 — GAS Combat | 10 |
| A5 — Multiplayer & Steam | 10 |
| A6 — UI & Final Demo (incl. touch controls) | 8 |
| A7 — Integration & Buffer | 11 |
| **Total** | **~62 days (12.5 weeks)** |

---

# 🔷 PHASE B: EPISODE PRODUCTION

Each episode requires:
- **Script prep** (if not yet drafted)
- **Recording** (1-2 hours)
- **Editing** (2-3 hours)
- **Export + README** (1 hour)

## B1 — Scripting (Episodes 6-36)

Episodes 1-5 are already scripted. Remaining 31 episodes need scripts:

| Batch | Episodes | Est. Hours Each | Total Hours |
|-------|----------|-----------------|-------------|
| Scripts 6-13 | 8 | 3 | 24 |
| Scripts 14-23 | 10 | 4 | 40 |
| Scripts 24-36 | 13 | 3 | 39 |
| **Total** | **31 scripts** | | **~103 hours (2.5 weeks)** |

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
| Phase 3: GAS Combat | 14-20 | 42 |
| Phase 3.5: Player AI | 21-23 | 18 |
| Phase 4: Advanced AI | 24-27 | 24 |
| Phase 5: Multiplayer | 28-32 | 30 |
| Phase 6: Final Demo | 33-36 | 24 |
| **Total** | **36 episodes** | **~216 hours (5.5 weeks)** |

## Phase B Total
| Activity | Time |
|----------|------|
| Scripting (6-36) | 2.5 weeks |
| Recording + Editing + Export | 5.5 weeks |
| **Total** | **~8 weeks** |

---

# 📊 FULL PROJECT SUMMARY

| Phase | Duration | Notes |
|-------|----------|-------|
| ✅ Planning (Phases 1-5) | Complete | Outlines, docs, scripts 1-5, workflow |
| ✅ Pre-Production Review | Complete | Consistency scan, risk docs, timeline |
| **Phase A: Private Demo** | **12.5 weeks** | Build all systems off-camera (incl. touch input) |
| **Phase B: Episode Production** | **8 weeks** | Record, edit, export 36 episodes |
| **Total Remaining** | **~20.5 weeks (~5 months)** | |

## Parallel Work

Scripting (Phase B1) can overlap with Phase A development:
- Script Episodes 6-13 during Phase A2/A3 development
- Script Episodes 14-23 during Phase A4/A5 development
- Script Episodes 24-36 during Phase A6/A7

This reduces total calendar time:

| Scenario | Calendar Time |
|----------|---------------|
| Sequential (Phase A → Phase B) | ~20 weeks |
| **With script overlap** | **~16 weeks (~4 months)** |

## Weekly Cadence (for release)

If releasing one episode per week:

| Phase | Episodes | Release Cadence | Duration |
|-------|----------|-----------------|----------|
| Phase 0: Player Core | 1-5 | 1/week | 5 weeks |
| Phase 1: NPC Lifecycle | 6-8 | 1/week | 3 weeks |
| Phase 2: AI Foundations | 9-13 | 1/week | 5 weeks |
| Phase 3: GAS Combat | 14-20 | 1/week | 7 weeks |
| Phase 3.5: Player AI | 21-23 | 1/week | 3 weeks |
| Phase 4: Advanced AI | 24-27 | 1/week | 4 weeks |
| Phase 5: Multiplayer | 28-32 | 1/week | 5 weeks |
| Phase 6: Final Demo | 33-36 | 1/week | 4 weeks |
| **Total release window** | **36 episodes** | **1/week** | **36 weeks (~9 months)** |

The 3-month private demo build + 9-month release window means a **~12-month project** from start to finish of the last episode release. Batch-recording (recording multiple episodes per week) compresses the production phase but not the release schedule.

---

# ⚡ KEY MILESTONES

| Milestone | Target | What's True |
|-----------|--------|-------------|
| Private demo playable | Week 8 | Core player + NPC + basic combat working |
| Private demo complete | Week 12 | All 13 systems, multiplayer, Steam, final loop |
| First episode released | Week 13 | Episode 1 on public repo |
| Targeting + enemies milestone | Week 17 | Episodes 1-5 released (playable combat) |
| AI milestone | Week 22 | Episodes 9-13 released (full NPC AI) |
| GAS milestone | Week 29 | Episodes 14-20 released (full ability system) |
| Multiplayer milestone | Week 34 | Episodes 28-32 released (MP + Steam) |
| Series complete | Week 48 | Episode 36 released |

---

**Total estimated effort:** ~20 weeks of work, ~12 months calendar (with weekly release cadence)
**Next step:** Begin Private Demo Development
