# 📘 RISK IDENTIFICATION
**File:** `Planning/Risk_Identification.md`

Identified risks for the Top-Down ARPG AI Demo tutorial series, organized by category.

---

# ⚠ TECHNICAL RISKS

## R1 — GAS Complexity Overwhelming Viewers
**Severity:** High | **Likelihood:** High
The Gameplay Ability System is the most complex system in the project. Episodes 14–20 depend entirely on it. If viewers struggle, the series loses momentum mid-way.

## R2 — StateTree Learning Curve
**Severity:** Medium | **Likelihood:** High
StateTrees are newer than Behavior Trees with fewer community resources. Episodes 9–13 lay the AI foundation — if StateTree setup is confusing, the entire AI pipeline is at risk.

## R3 — Multiplayer Replication Desyncs
**Severity:** High | **Likelihood:** Medium
Replicating NPCs, abilities, targeting, PvP flag, and health across server/client creates many synchronization points. A single missed replication can cause hard-to-debug desyncs.

## R4 — Steam Auth Edge Cases
**Severity:** Medium | **Likelihood:** Medium
Steam not running, expired auth tickets, ticket validation timeout, dedicated server registration failure — these edge cases are hard to test systematically.

## R5 — Pool Exhaustion Under Load
**Severity:** Low | **Likelihood:** Low
If all pooled NPCs are active and a new spawn is requested, the system needs a fallback (create new or queue). Only surfaces under heavy combat.

## R6 — Per-NPC Respawn Timer Cascade
**Severity:** Medium | **Likelihood:** Medium
Multiple NPCs dying simultaneously creates many independent timers. Performance spike when all timers fire in close succession.

## R7 — Server-Only AI Enforcement
**Severity:** High | **Likelihood:** Medium
Any AI logic accidentally running on clients causes desyncs, wasted CPU, and potential exploits. Requires discipline across all NPC AI episodes.

## R8 — NPC State Reset on Pool Reuse
**Severity:** High | **Likelihood:** Medium
Forgetting to reset health, AI state, group membership, or StateTree context when an NPC returns from the pool causes stale behaviour.

## R9 — Client-Side Targeting Exploit
**Severity:** Medium | **Likelihood:** Low
Targeting is client-side with server validation. A malicious client could send fabricated target data. Server must validate all target data during ability execution.

## R10 — AoE PvP Damage Filtering
**Severity:** Medium | **Likelihood:** Medium
AoE abilities overlapping both enemies and players when PvP is disabled. The damage execution must check each target individually, not just the source.

## R11 — PvP Toggle Mid-Projectile
**Severity:** Low | **Likelihood:** Low
A projectile is fired before PvP toggle, arrives after. Should damage apply based on toggle at fire-time or impact-time? Need a clear design decision.

## R12 — Character Movement Replication
**Severity:** Medium | **Likelihood:** Medium
Getting smooth replicated movement in a top-down game with click-to-move. Client prediction, correction, and jitter handling.

## R13 — Ability Cooldown Desync
**Severity:** Medium | **Likelihood:** Medium
Client-predicted cooldown UI may desync from server state. Need clear cooldown replication strategy.

## R14 — Projectile Replication
**Severity:** Medium | **Likelihood:** Medium
Projectile movement, hit detection, and damage application across the network. Server-authoritative projectiles vs client-side visuals.

## R15 — DB Corruption on DS Crash
**Severity:** High | **Likelihood:** Low
Dedicated server crashes mid-write could corrupt SQLite DB. WAL mode + transaction wrapping required.

## R16 — Schema Migration Failure
**Severity:** High | **Likelihood:** Low
A migration script that works in dev fails on a clean DB or after multiple migrations. CI must test from v0 to current.

## R17 — SteamID Extraction Failure
**Severity:** Medium | **Likelihood:** Medium
`BeginAuthSession()` may fail to return a valid SteamID. Need fallback (hash of auth ticket) and clear error message to client.

## R18 — Save Data Staleness
**Severity:** Medium | **Likelihood:** Medium
Player disconnects mid-session; last auto-save was 4 minutes ago. 5-min auto-save window means up to 5m progress loss. Acceptable for demo, document for production.

## R19 — Character Select Race Condition
**Severity:** Medium | **Likelihood:** Low
Two clients try to create a character in the same empty slot simultaneously. DB unique constraint prevents corruption; handle gracefully in UI.

## R20 — PgSQL Connection String Misconfiguration
**Severity:** Medium | **Likelihood:** Medium
DS fails to start if connection string is wrong. Validate on startup, fall back to SQLite with warning log.

---

# 🎯 DESIGN RISKS

## R21 — Episode Scope Creep
**Severity:** High | **Likelihood:** High
Individual episodes taking longer than expected, pushing the 38-episode target. Each system doc lists 6-8 testing items — real implementation always reveals more.

## R22 — Episode Dependency Chain Breaks
**Severity:** High | **Likelihood:** Medium
Later episodes depend on earlier systems. If Episode 4 (Spawner) has issues, Episodes 5-8 cascade. No buffer or catch-up episodes planned.

## R23 — Player AI vs NPC AI Interaction Bugs
**Severity:** Medium | **Likelihood:** Medium
Both use StateTrees but have different schemas, targets, and rules (PvP awareness). Edge cases where they interact unexpectedly.

## R24 — Autoplay Possession Switching
**Severity:** Medium | **Likelihood:** Low
Switching between PlayerController and PlayerAIController mid-combat. State cleanup, input routing, and UI must all transition cleanly.

## R25 — Camera Collision Edge Cases
**Severity:** Low | **Likelihood:** Medium
Camera clipping through geometry in tight spaces or when the character is near walls. SpringArm collision handles most cases, but edge cases remain.

## R26 — Group Assist Trigger Accuracy
**Severity:** Medium | **Likelihood:** Medium
The assist radius must feel right — too small and assist never fires, too large and the entire group agros instantly. Requires tuning.

## R27 — Multiple Assist Events Overlapping
**Severity:** Low | **Likelihood:** Medium
Multiple NPCs in the same group attacked simultaneously. Each sends an assist event — the target NPC must not stack transitions.

## R28 — Flee State Isolation Detection
**Severity:** Medium | **Likelihood:** Medium
Flee triggers on "low health + isolated." Defining "isolated" (distance to allies, number of nearby allies) is subjective and may need tuning.

---

# 🎬 PRODUCTION RISKS

## R23 — Recording Requires Perfect Execution
**Severity:** High | **Likelihood:** High
Each episode must be recorded in one take or heavily edited. Mistakes, compile errors, or forgotten steps during recording waste hours.

## R24 — UE Version Compatibility
**Severity:** Medium | **Likelihood:** Medium
Viewers may be on a different UE version. GAS, StateTree, or networking APIs may differ. Providing version-specific notes adds overhead.

## R25 — Episode Export Errors
**Severity:** Medium | **Likelihood:** Medium
The manual episode export workflow (strip features, copy, verify) is error-prone. Forgetting to strip Steam config or leaving spoiler code.

## R26 — Public Repo Spoilers
**Severity:** High | **Likelihood:** Medium
Accidentally exposing future episode content (systems, classes, config) in an early episode snapshot.

## R27 — Asset Creation Bottleneck
**Severity:** Medium | **Likelihood:** Medium
The project needs placeholder meshes, UI textures, ability icons, and audio. Creating or sourcing these takes time away from code.

## R28 — Steam AppID Leak
**Severity:** High | **Likelihood:** Low
Steam AppID or SDK files accidentally committed to the public repo. The README explicitly warns about this, but it's still a risk.

## R29 — Audio/Video Quality vs Code Depth Tradeoff
**Severity:** Low | **Likelihood:** Medium
Balancing production polish (editing, transitions, audio) with technical depth. Too much polish delays release; too little hurts viewership.

## R30 — Tutorial Series Fatigue
**Severity:** Medium | **Likelihood:** Medium
38 episodes is a long series. Viewer drop-off after Episode 10-15 is common. Early episodes must deliver value to sustain engagement.

---

# 🧪 TESTING & QA RISKS

## R31 — Multiplayer Testing Complexity
**Severity:** High | **Likelihood:** High
Testing requires dedicated server, multiple clients, Steam auth setup. Cannot fully verify multiplayer systems in a single PIE session.

## R32 — No Automated Test Suite
**Severity:** Medium | **Likelihood:** High
The project has no unit tests or automated integration tests. All verification is manual (checklist-based). Regressions are easy to miss.

## R33 — AI Behavior Hard to Verify Deterministically
**Severity:** Medium | **Likelihood:** Medium
StateTree AI is inherently non-deterministic (perception timings, navigation paths). Hard to write reproducible test cases.

## R34 — Network Latency Not Tested
**Severity:** Medium | **Likelihood:** Medium
All development likely happens locally or on LAN. Real-world latency, packet loss, and jitter are not part of the test plan.

## R35 — PvP Toggle Abuse (Rapid Toggling)
**Severity:** Low | **Likelihood:** Low
Player rapidly toggling PvP on/off during combat to avoid damage. Noted as a future extension (cooldown timer), not addressed in v1.

## R36 — StateTree Debugging Tooling Immaturity
**Severity:** Medium | **Likelihood:** Medium
StateTree debugger exists but has fewer features than Behavior Tree debugger. Troubleshooting AI issues during recording may be harder.

## R37 — Pooled NPC Death During StateTree Evaluation

## R38 — Threat Angular Spread Collides with Navmesh Obstacles
**Severity:** Low | **Likelihood:** Low
The angular spread computes a position at `AttackRange` from the player, but that position may be inside a wall, off the navmesh, or on an unreachable ledge. Need navmesh projection and a fallback (clamp to nearest navmesh point, or default to current position).

## R39 — Threat Table Memory Leak on Long Sessions
**Severity:** Low | **Likelihood:** Low
`TWeakObjectPtr` prevents dead NPC entries from pinning actors, but the map entries themselves persist. Over a very long session with many NPC spawn/death cycles, the inner `TMap` accumulates dead entries. Mitigation: `RemoveEnemy()` cleans up on every pool return; periodic `Compact()` or `ClearAll()` on level transition.

## R40 — Threat Override Breaks Perception-Driven Assist Flow
**Severity:** Medium | **Likelihood:** Low
Assist hearing events are processed by `OnPerceptionUpdated`, which sets the noise location and triggers the Investigate/Search chain. If threat immediately overrides the target on arrival, the assist NPC may never investigate the noise. Mitigation: threat only active once the NPC has threat entries. Assist NPCs start with zero threat — they investigate first, build threat on their first attack.
**Severity:** Low | **Likelihood:** Low
An NPC dies while the StateTree is mid-evaluation. The NPC returns to pool mid-frame. Needs careful handling of death + pooling ordering.

---

# 📱 MOBILE / TOUCH RISKS

## R38 — Touch Input Precision & Occlusion
**Severity:** Medium | **Likelihood:** Medium
Touch lacks mouse-level precision. Fingers occlude the target area. Tap vs tap-hold differentiation needs care. Single tap must disambiguate between move and target intent.

## R39 — Touch UI Sizing on Different Screens
**Severity:** Low | **Likelihood:** Medium
Ability buttons and the PvP toggle must be large enough for touch (44×44 px minimum) but not dominate the screen on small devices. Responsive layout needed.

## R40 — Mobile Performance Constraints
**Severity:** Medium | **Likelihood:** Medium
Mobile devices have less CPU/GPU headroom. Object pooling, AI LOD, and draw calls need mobile-friendly thresholds. The demo targets desktop first but must not hard-crash on mobile.

---

# 💀 CORPSE SYSTEM RISKS

## R41 — Corpse-Pool Desync
**Severity:** Low | **Likelihood:** Low
The two-tier architecture (NPC returns to pool on death while corpse persists in world) creates a split lifecycle that must remain synchronized. If the corpse fails to spawn (bad location, null reference, world cleanup), the NPC still returns to pool correctly — no crash, but a visual gap. The reverse (corpse spawns but NPC doesn't return to pool) would cause an actor leak.

## R42 — Corpse Accumulation Under Heavy Combat
**Severity:** Low | **Likelihood:** Medium
If NPCs die faster than the corpse despawn timer (e.g., AoE kill-cascade), corpses accumulate. Without a hard cap, this causes world-bloat, increased draw calls, and eventual performance degradation on all clients.

---

# 🌐 MULTIPLAYER & STEAM RISKS

## R46 — DS Build Configuration Issues (UE 5.8)
**Severity:** Medium | **Likelihood:** Medium
UE 5.8 may have changed DS build paths, module requirements, or platform file references. First-time DS build often requires iteration.

## R47 — Client Authority Exploit via RPC Spoofing
**Severity:** High | **Likelihood:** Low
A malicious client could send crafted RPCs to manipulate server state. Requires discipline in all Server_ RPC handlers.

## R48 — StateTree Task Authority Guards Missed
**Severity:** High | **Likelihood:** Medium
With 9+ StateTree task classes, it's easy to miss one during the authority audit. A single unguarded task running on client causes silent desyncs.

## R49 — GAS Replication Prediction Conflicts
**Severity:** Low | **Likelihood:** Low
UE's GAS handles most replication automatically, but prediction of gameplay effects on simulated proxies can cause visual desyncs under latency.

---

**Total: 49 risks identified**
**Next step:** Mitigation strategies in Risk Mitigation Plan.
