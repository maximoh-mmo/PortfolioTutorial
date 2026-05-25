# 📘 RISK MITIGATION PLAN
**File:** `Planning/Risk_Mitigation_Plan.md`

Mitigation strategies for all 37 identified risks (see [Risk_Identification](Risk_Identification.md)).

---

# 🛡 TECHNICAL RISK MITIGATIONS

## R1 — GAS Complexity Overwhelming Viewers
**Strategy:** Scaffold + Simplify
- **Prototype early:** Build a standalone GAS test map (no AI, no multiplayer) in Episode 0/private prep to validate the ability pipeline before Episode 14
- **Simplify:** Use a single `UAttributeSet` with only Health, MaxHealth, and Damage — no secondary attributes, no meta attributes
- **Episode design:** Dedicate Episode 14 entirely to GAS setup (ASC, AttributeSet, GameplayTags) before adding any abilities
- **Fallback:** If GAS proves too complex for viewers, provide a downloadable starter project with GAS pre-configured

## R2 — StateTree Learning Curve
**Strategy:** Reference + Visual Guide
- **Prototype early:** Build a minimal StateTree (Idle → Chase) in Episode 0 before Episode 9 to know the API cold
- **Mitigation:** Include a StateTree schema diagram in each AI episode's README
- **Episode design:** Episode 9 is setup only (schema, component, context data) — no behaviour logic until Episode 10
- **Fallback:** Keep Behavior Tree as a fallback skill mention in case StateTree has engine bugs

## R3 — Multiplayer Replication Desyncs
**Strategy:** Test Harness + Checklist
- **Prototype early:** Build a multiplayer test map with minimal replicated actor to validate RPC/replication patterns before Episode 28
- **Mitigation:** After each multiplayer episode, run a 2-client + dedicated server test with the [Public Release Checklist](../Planning/Workflow/PUBLIC_RELEASE_CHECKLIST.md)
- **Design rule:** Every replicated property must have an `OnRep` function that validates state
- **Fallback:** If desyncs are persistent, simplify to listen-server only for that episode and mark DS as advanced

## R4 — Steam Auth Edge Cases
**Strategy:** Graceful Degradation
- **Prototype early:** Test auth flow with AppID 480 (Spacewar) before Episode 32
- **Mitigation:** Implement fallback modes: "Steam Offline" (skip auth) and "Steam Error" (show clear error message)
- **Testing:** Create a test matrix: Steam running, Steam not running, invalid ticket, expired ticket, timeout
- **Fallback:** Episode can demonstrate auth flow even if DS registration doesn't work — focus on client→server ticket flow

## R5 — Pool Exhaustion Under Load
**Strategy:** Simple Fallback
- **Mitigation:** `GetNPC()` falls back to `SpawnActor` if the pool is empty — no crash, just a warning log
- **Design rule:** Pool initial size = max expected NPCs × 1.5
- **Simplify:** Don't implement queueing — if pool is empty, spawn new and log

## R6 — Per-NPC Respawn Timer Cascade
**Strategy:** Batch Tolerance
- **Mitigation:** Cap concurrent respawn timers at a configurable max (e.g. 10 per frame). Queue excess to next frame
- **Simplify:** Use a single round-robin scheduler for respawns instead of individual timers if cascade becomes an issue
- **Testing:** Stress test with 50 NPCs dying simultaneously

## R7 — Server-Only AI Enforcement
**Strategy:** Defensive Guards
- **Mitigation:** Add `if (!HasAuthority()) return;` guard at the top of every AI-related function in `ANPCAIController` and `ANPCCharacter`
- **Automation:** Add a dev-only log/warning if AI code runs on client (`ensure(!IsNetMode(NM_Client))`)
- **Code review:** Check AI authority in every PR

## R8 — NPC State Reset on Pool Reuse
**Strategy:** Reset Function + Checklist
- **Mitigation:** Create a single `ResetNPCState()` function called by the pool that resets: Health, CurrentTarget, StateTree context, GroupComponent membership, visibility, collision
- **Testing:** After pooling episode, verify an NPC returned to pool and re-spawned has zero stale state

## R9 — Client-Side Targeting Exploit
**Strategy:** Server Re-Validation
- **Mitigation:** GAS ability activation always re-validates target on server: range check, LOS check, PvP rule check, alive check
- **Simplify:** Client sends `TargetActor` and `TargetLocation`; server picks the final authoritative target

## R10 — AoE PvP Damage Filtering
**Strategy:** Per-Target Check
- **Mitigation:** Damage execution iterates all affected actors and checks `ShouldApplyDamage(source, target)` individually — not a single global PvP gate
- **Testing:** AoE that overlaps 2 enemies + 1 player (PvP OFF) — only enemies take damage

## R11 — PvP Toggle Mid-Projectile
**Design Decision:** Check at impact time
- **Mitigation:** Damage is evaluated when the projectile hits, not when it was fired. This is consistent and simple
- **UE implementation:** Projectile's `OnHit` → GAS apply — PvP check happens in damage execution

## R12 — Character Movement Replication
**Strategy:** Default UE + Tuning
- **Mitigation:** Use default Unreal character movement replication (no custom movement). Tune `NetworkSimulatedProxy` and `MaxMoveDeltaTime`
- **Testing:** Run with simulated lag (Settings → Network emulation) to verify feel

## R13 — Ability Cooldown Desync
**Strategy:** Server Auth + Client Side
- **Mitigation:** Cooldown is authoritative on server. Client predicts cooldown UI but server corrects on `OnRep`
- **Simplify:** If desync is persistent, don't show cooldown timer — show a greyed-out icon that updates on server response

## R14 — Projectile Replication
**Strategy:** Server-Authoritative Projectiles
- **Mitigation:** Spawn projectile on server. Client predicts spawn for immediate visual feedback, server corrects
- **Simplify:** For base episode, use instant-hit abilities (no projectile). Add projectile as Episode 20 extension
- **Fallback:** If projectile replication is too complex, skip and use beam/hitscan only

---

# 🛡 DESIGN RISK MITIGATIONS

## R15 — Episode Scope Creep
**Strategy:** Hard Timebox
- **Mitigation:** Each episode has a strict time budget (est. 20-30 min video ≈ 2-4 hours dev). If an episode exceeds budget, cut features to a "Minimum Viable Episode" and defer rest
- **Tracking:** After each episode script, estimate recording time. If >30 min, split into two episodes or trim
- **Buffer:** Reserve 2-3 buffer episodes in the 38-count for overflow content

## R16 — Episode Dependency Chain Breaks
**Strategy:** Episode 0 / Prep Work
- **Mitigation:** Build the full private demo first (off-camera). Record episodes from the working demo. This ensures no broken chain
- **Fallback:** If a mid-series bug surfaces, record a "Fixing [Bug]" mini-episode instead of re-recording

## R17 — Player AI vs NPC AI Interaction Bugs
**Strategy:** Separation of Concerns
- **Mitigation:** Player AI and NPC AI use separate StateTree schemas, separate controller classes, and separate target selection logic. Keep them isolated
- **Testing:** Run Player AI autoplay vs NPC AI group combat as a test scenario

## R18 — Autoplay Possession Switching
**Strategy:** Clean Transition
- **Mitigation:** On autoplay toggle:
  1. Stop all player input processing
  2. Copy current target from TargetingComponent to PlayerAIController
  3. AI takes over via `Possess()`
  4. On disable: reverse
- **Simplify:** Require player to be idle (not mid-combat) to toggle. Disable toggle during combat

## R19 — Camera Collision Edge Cases
**Strategy:** Acceptable Defaults
- **Mitigation:** SpringArm collision + `bDoCollisionTest = true` handles 95% of cases. Document known edge cases (tight corridors) in the episode
- **Simplify:** Don't add custom camera collision logic — engine defaults are sufficient

## R20 — Group Assist Trigger Accuracy
**Strategy:** Configurable + Tune Early
- **Mitigation:** `AssistRadius` is a configurable float on the GroupManager. Default to 500 units. Tune during Episode 7 recording prep
- **Testing:** Test with group of 5 NPCs spread at varying distances. Assist should feel responsive but not trigger-happy

## R21 — Multiple Assist Events Overlapping
**Strategy:** Debounce
- **Mitigation:** GroupManager tracks which NPCs are currently in "assist transition" and ignores duplicate events within a 1-second window
- **Simplify:** If debounce is too complex, simply allow overlapping transitions — StateTree handles redundant transitions gracefully

## R22 — Flee State Isolation Detection
**Strategy:** Simple Heuristic
- **Mitigation:** "Isolated" = no allies within `FleeIsolationRadius` (configurable, default 800 units) AND health < 30% of max
- **Testing:** Verify NPC flees when alone at low health, does not flee when allies are nearby

---

# 🛡 PRODUCTION RISK MITIGATIONS

## R23 — Recording Requires Perfect Execution
**Strategy:** Prep + Retake
- **Mitigation:** Before recording, run through the entire episode script off-camera once. Record code snippets separately (text overlay) to avoid typing mistakes
- **Simplify:** If a mistake happens, don't restart — use a "Take 2" cut. Edit out the error in post
- **Buffers:** Record 2 episodes ahead of release schedule to absorb retakes

## R24 — UE Version Compatibility
**Strategy:** Targeted Version
- **Mitigation:** Pick one specific UE version (e.g. 5.5) and state it clearly in every episode's description and README
- **Simplify:** Note API differences only for major systems (GAS, StateTree) in a pinned comment

## R25 — Episode Export Errors
**Strategy:** Script the Export
- **Mitigation:** Automate the [Episode Export Workflow](Workflow/EPISODE_EXPORT_WORKFLOW.md) as a PowerShell script before Episode 1 is released
- **Simplify:** Manual export with checklist. Run the [Public Release Checklist](Workflow/PUBLIC_RELEASE_CHECKLIST.md) before every push

## R26 — Public Repo Spoilers
**Strategy:** Export Script + Tree Diff
- **Mitigation:** The export script should strip: all C++ files not used in this episode, Steam config, server config, experimental content
- **Verification:** After export, run `git diff origin/episode/XX --name-only` to verify only episode-XX files changed

## R27 — Asset Creation Bottleneck
**Strategy:** Use Engine Content + Store Assets
- **Mitigation:** Use UE starter content (Mannequin, Basic shapes) for placeholder meshes. Use simple colored materials for UI
- **Simplify:** No custom art until Final Demo phase (Episode 35+). Everything is whitebox/prototype quality. Explicitly state this to viewers

## R28 — Steam AppID Leak
**Strategy:** .gitignore + Export Filter
- **Mitigation:** Add Steam SDK directory to `.gitignore` at root level. Export script explicitly excludes anything under `/Steam/`
- **Automation:** CI check on public repo: fail if any file contains "SteamAppId" or "steam_appid"

## R29 — Audio/Video Quality vs Code Depth Tradeoff
**Strategy:** Content-First
- **Mitigation:** Prioritize code clarity and explanation over production polish. Simple screen capture + good microphone is sufficient
- **Simplify:** No intro animations, no background music, minimal editing. Release early, iterate on format based on feedback

## R30 — Tutorial Series Fatigue
**Strategy:** Milestone Episodes + Value Density
- **Mitigation:** Every 5th episode should be a milestone (playable combat, working AI, multiplayer test) that gives viewers a sense of progress
- **Episode design:** Each episode delivers at least one "aha moment" — a visible, testable result by the end
- **Marketing:** Promote milestone episodes separately to re-engage dropped viewers

---

# 🛡 TESTING & QA RISK MITIGATIONS

## R31 — Multiplayer Testing Complexity
**Strategy:** CI + Local Scripts
- **Prototype early:** Create a PowerShell script that launches DS + 2 clients locally before Episode 28
- **Mitigation:** After each multiplayer episode, run the test script and verify the feature works. Document the process in `Server/LAUNCH_NOTES.md`
- **Simplify:** For episodes 28-30, test in PIE with `NetMode` switch first, then validate on DS

## R32 — No Automated Test Suite
**Strategy:** Manual Checklists + Smoke Tests
- **Mitigation:** Each episode's "Testing Checklist" in the script IS the test plan. Run through it before recording
- **Long-term:** If the private demo grows complex, add a simple C++ functional test for the final demo loop only
- **Simplify:** Accept manual testing — 38 episodes of checklist-based verification is sufficient for this scope

## R33 — AI Behavior Hard to Verify Deterministically
**Strategy:** Debug States + Logs
- **Mitigation:** Add a "debug AI state" display (on-screen text) that shows the current StateTree state, target, and health. Use during testing
- **Simplify:** Don't write automated AI tests. Rely on visual verification + the on-screen debugger

## R34 — Network Latency Not Tested
**Strategy:** Emulated Conditions
- **Mitigation:** UE has built-in network emulation (Settings → Network emulation). Test with 50ms, 100ms, 200ms latency + 5% packet loss
- **Simplify:** Document that the demo works under ideal conditions; multiplayer polish is a future extension

## R35 — PvP Toggle Abuse (Rapid Toggling)
**Strategy:** Accept + Future Work
- **Mitigation:** Document this as a known limitation. The PvP cooldown timer is listed in Future Extensions
- **Simplify:** For the tutorial series, rapid toggling is an edge case that won't be addressed in core episodes

## R36 — StateTree Debugging Tooling Immaturity
**Strategy:** Visual Debug + Logging
- **Mitigation:** Use UE's built-in StateTree Debugger. Supplement with on-screen text that prints current state, target, and evaluator values
- **Simplify:** If debugger is insufficient, fall back to UE_LOG with verbosity levels for AI state transitions

## R37 — Pooled NPC Death During StateTree Evaluation
**Strategy:** Deferred Pool Release
- **Mitigation:** `OnDeath` does not immediately return NPC to pool. It sets a "pending return" flag. The pool collects pending NPCs at the end of the frame
- **Simplify:** Use a timer (0s, next tick) to defer pool return by one frame, ensuring StateTree evaluation completes

---

# 🛡 MOBILE / TOUCH RISK MITIGATIONS

## R38 — Touch Input Precision & Occlusion
**Strategy:** Generous Hit Targets + Tap Branching
- **Mitigation:** Use larger collision-aware hit detection for touch (expand raycast radius). Disambiguate move vs target via a short tap timer: quick tap → move, hold briefly or tap near enemy → target
- **Simplify:** Add a "Target Lock" button on mobile that cycles through nearest enemies instead of requiring precise taps
- **Testing:** Test on tablet and phone-sized viewports in Editor

## R39 — Touch UI Sizing on Different Screens
**Strategy:** Responsive Layout + Minimum Size
- **Mitigation:** All touch buttons use `ISlateBox` with minimum 44×44 px constraint. Use UE's `DPIScaler` or safe-zone margins for different aspect ratios
- **Simplify:** Default to a fixed mobile layout that works on 16:9. Document that ultra-wide or very small screens may need adjustment

## R40 — Mobile Performance Constraints
**Strategy:** Desktop-First + Mobile Tolerable
- **Mitigation:** Pool sizes default to desktop-friendly values (100+ NPCs). For mobile, add a `bLowPerformanceMode` flag that reduces pool size, disables AI LOD, and lowers draw distance
- **Testing:** Profile on mobile-class hardware (or use `r.MobileContentScaleFactor` in Editor) to verify it doesn't hard-crash

---

# 📋 SUMMARY: WHAT TO PROTOTYPE EARLY

These systems should be built and validated in private (Episode 0 / prep work) before recording any episodes:

| Priority | System | Why |
|----------|--------|-----|
| P0 | GAS ability pipeline | Validate ASC + AttributeSet + GameplayEffect workflow |
| P0 | StateTree minimal setup | Validate schema, component, context data, transition |
| P0 | Multiplayer RPC test | Validate authority, replication, RPC patterns |
| P1 | Steam auth flow | Test ticket generation and validation (AppID 480) |
| P1 | Object pooling | Validate NPC reset and pool exhaustion fallback |
| P2 | Group assist system | Tune assist radius before recording |
| P2 | Player AI autoplay | Validate possession switching |

# 📋 SUMMARY: WHAT TO SIMPLIFY IF NEEDED

| Risk | Simplify To |
|------|-------------|
| R14 — Projectile Replication | Hitscan/instant abilities only |
| R9 — Targeting Exploit | Trust client targeting, validate only distance + PvP |
| R11 — PvP Mid-Projectile | Freeze PvP toggle while projectile is in flight |
| R13 — Cooldown Desync | Remove cooldown timer display, use greyed icon only |
| R18 — Autoplay Switching | Disable toggle during combat |
| R35 — PvP Toggle Abuse | Accept as known limitation |

# 📋 SUMMARY: WHAT TO DELAY OR CUT

| Risk | Feature | When |
|------|---------|------|
| R25 — Export Automation | PowerShell export script | After Episode 1 is scripted, before Episode 1 release |
| R32 — Automated Tests | C++ functional tests | After private demo is complete |
| R26 — Public Repo Spoilers | CI spoiler check | Before Episode 1 public push |

---

**Total: 40 risks with mitigation strategies**
**Next step:** Create Production Timeline
