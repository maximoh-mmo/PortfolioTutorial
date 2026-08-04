# 🎬 Post 10 — From Aggro to Threat: Building a Server-Side Threat Table and Simplifying the StateTree

*23 commits · Jun 22–27, 2026 · covering the Threat System arc, the Aggro→Threat rename, and a full codebase audit*

---

## Introduction

By the end of Post 9, the player could watch AI fight AI indefinitely. But the combat decision-making was still rough — enemies would chase the nearest target in a straight line, clump together, and occasionally get stuck. The A3.6 wave (planned in Post 1's pre-production review) was supposed to fix all of that with a proper threat system, angular combat spread, and AI LOD.

This arc delivers all three — and then some. It also performs the largest codebase audit of the project: renaming the system from "Aggro" to "Threat," restructuring AI sources into `Enemy/` and `StateTree/` directories, consolidating four separate StateTree tasks into a single `EnemyEngageTask`, and cleaning up every deprecated API and leftover debug log. The arc closes with Sprint A5 (Multiplayer & Steam) planned and the threat system archived to DONE — 75/75 on A3, the first fully complete AI phase.

---

## Technical Exposition

### Why Threat, Not Aggro

The original design doc used "Aggro" — a term that describes the *mechanic* (who gets attacked) but not the *data* (the accumulated numeric value that determines who gets attacked). The rename to "Threat" happened at the planning stage, before any implementation, and the codebase never contained a `UOnsetAggroSubsystem` class. The rename was a vocabulary decision that paid dividends in clarity: "threat table" immediately communicates that it's a data structure, not a behavior.

### World Subsystem over per-NPC components

The threat subsystem is a `UWorldSubsystem` — server-only, zero per-NPC overhead, single map clean-up on pool return or player death. The design doc explicitly chose this over per-NPC aggro components:

> "Zero per-NPC overhead, single map clean-up on pool return / player death, server-only data — no replication needed."

Taunt becomes a one-line call: `AddThreat(Taunter, Enemy, TauntAmount)`. No per-NPC aggro component to wire up, no replicated state to sync.

### Threat × distance scoring

Pure highest-threat would send every melee NPC halfway across the map toward a distant ranger. The scoring weights threat by distance with a graceful falloff:

```cpp
const float DistWeight = (DistSq <= AttackRangeSq) ? 1.0f
                         : (DistSq <= ChaseRangeSq)  ? 0.5f
                         : 0.1f;
const float Score = ThreatVal * DistWeight;
```

`GetPrimaryTarget` is retained for raw-value queries (future taunt/debug), while `GetBestTarget` is the workhorse for actual engagement decisions.

---

## Implementation Details

### 1. The threat subsystem

`UOnsetThreatSubsystem` stores two tables keyed by enemy pawn:

- `ThreatTable`: enemy → map of player → threat value
- `EngagementTable`: enemy → set of currently engaged players

Damage feeds into the threat table via `PostGameplayEffectExecute` in the attribute set:

```cpp
if (UOnsetThreatSubsystem* ThreatSub = GetWorld()->GetSubsystem<UOnsetThreatSubsystem>())
{
    AOnsetEnemy* TargetEnemy = Cast<AOnsetEnemy>(Data.Target.GetOwnerActor());
    AOnsetBaseCharacter* Instigator = Cast<AOnsetBaseCharacter>(Data.EffectSpec.GetContext().GetInstigator());
    if (TargetEnemy && Instigator)
        ThreatSub->AddThreat(Instigator, TargetEnemy, FMath::Abs(Data.EvaluatedData.Magnitude));
}
```

The sight-based base threat of 1.0 seeds the table when an NPC *sees* a player but hasn't been hit yet — without it, `GetBestTarget` would return null for unengaged enemies and the combat loop would never start.

### 2. The single EngageTask replacing four tasks

The original plan called for separate Agro, Chase, Attack, and AttackPosition tasks. The implementation collapsed them into one `FEnemyEngageTask` with two re-evaluation intervals:

- **Target re-evaluation:** every 1 second (threat may change)
- **Position re-evaluation:** every 3 seconds, or when the target moves >200 units (position may be stale)

```cpp
if (Best && Best != CurrentTargetPtr)
{
    Subsystem->SwitchTarget(SelfEnemy, Best);
    SetTarget(Context, Best);
    AIController->SetFocus(Best);
    Inst.CurrentTarget = Best;
    Inst.LastTargetLocation = Best->GetActorLocation();
    Inst.NextPositionReevaluateTime = 0.0f;
}
```

The `SwitchTarget` is only called when the best target *actually changed* — not every tick. This was the fix for the per-frame engagement spam that plagued the earlier multi-task design.

### 3. Angular spread positioning

Instead of random lateral offsets (Post 8), the threat system uses deterministic angular spread:

```cpp
FVector FOnsetStateTreeTask::GetThreatAngularOffset(int32 Count, int32 Rank, float Radius)
{
    if (Count <= 0 || Rank < 0 || Rank >= Count) return FVector::ZeroVector;
    float Angle = Rank / static_cast<float>(Count) * 360.0f;
    float Rad = FMath::DegreesToRadians(Angle);
    return FVector(FMath::Cos(Rad) * Radius, FMath::Sin(Rad) * Radius, 0);
}
```

Rank 0 sits directly in front of the target (angle 0°); when an NPC dies, survivors re-rank and re-space automatically. This produces natural tank formation without a formation system.

### 4. AI LOD: three distance tiers

Far enemies don't need full tick updates. The controller evaluates LOD every 30 ticks:

```cpp
if (NearestDist < CachedSightRange)
{
    SetActorTickInterval(0.0f);
    StateTreeComponent->SetComponentTickEnabled(true);
}
else if (NearestDist < CachedHearingRange)
{
    SetActorTickInterval(0.2f);
    StateTreeComponent->SetComponentTickEnabled(true);
}
else
{
    SetActorTickInterval(0.5f);
    StateTreeComponent->SetComponentTickEnabled(false);
}
```

### 5. The codebase audit

The arc's less glamorous but equally important work:

- **`48c98af`:** Restructured C++ sources — `OnsetAIController` → `AI/`, `OnsetBaseCharacter` + `TargetingComponent` → `Core/`, flat `Combat/` → `Combat/` (kept). 31 include paths updated.
- **`696bf55`:** CheatManager now works under AI controller possession — `GetActiveController()` returns the AI controller when the player pawn is possessed by it, so `CheatManager->God()` still works in autoplay.
- **`6f5bdf5`:** Renamed all enemy StateTree tasks to `Enemy*Task` prefix for consistency with `Player*Task` and to avoid reflection name collisions.
- **`adc399b` + `e225051`:** Two massive batch audits — deprecated `CastChecked` → `Cast`, const correctness, unused includes, DRY extraction (`GetSelfPawn<T>` template, `ATTRIBUTE_ACCESSORS_BASIC` macro), member init, null guards. 48+ files cleaned.
- **`0a9bf52`:** Wave 4 verification pass, StateTree debug display (`DrawDebugString` in editor), and — critically — reverted the episode count from 46 back to 43 after duplicate Ep 26–28 were removed.

### 6. The Aggro→Threat rename (before implementation)

`78efaef` renamed the entire system across 14 files *before any threat code existed* — the planning doc was written first, the implementation followed. The rename touched `AGGRO_CURRENT.md` → `THREAT_CURRENT.md`, `Aggro_System.md` → `Threat_System.md`, and all `UOnsetAggroSubsystem`/`GetAggro*` references → `Threat*`.

---

## Results & Validation

- Threat table with damage feed, sight-based base threat, and engagement tracking
- Single `EnemyEngageTask` replacing Agro/Chase/Attack/AttackPosition — fewer state transitions, fewer bug surfaces
- Deterministic angular spread positioning with automatic re-ranking
- 3-tier AI LOD (full tick / 0.2s tick / paused)
- Codebase audit: deprecated APIs removed, const correctness enforced, DRY extraction, task renames
- Episode count corrected (46 → 43 after deduplication)
- A3 = 75/75 (100%) — first fully complete AI phase
- Sprint A5 planned with 4 waves, 10 daily TODOs, risks R46–R49

---

## Challenges & Solutions

### SwitchTarget spam every tick

**Problem:** `TWeakObjectPtr<AOnsetBaseCharacter> CurrentTarget` in instance data didn't survive the EnterState→Tick boundary — weak pointers don't pin, so `Best != CurrentTargetPtr` was effectively always true, re-firing `SwitchTarget` every tick and spamming `RegisterEngaged` logs.

**Solution:** `TObjectPtr` for the cached current target + `RegisterEngaged` (not `SwitchTarget`) on initial entry. A subtle pointer-lifetime bug that only showed up in a running StateTree.

### State oscillation (Engage ↔ Selector flicker)

**Problem:** Returning `Succeeded` from `EnterState` immediately when no target existed let the selector re-enter Engage next frame — a per-frame oscillation that spammed `RegisterEngaged` and made NPCs visibly jitter.

**Solution:** Always `Running`, only `Succeeded` after a 2-second sustained no-target window (`TimeWithoutTarget`). Hysteresis prevents thrash.

### The `AI_Profile.uasset` AttackRange = 10000 bug

**Problem:** A data-asset error shipped with `AttackRange = 10000` (the finalize commit fixed it to 100). NPCs would stand ~10 km away.

**Solution:** Fixed the asset AND added runtime AIProfile override in `EnterState` so the asset default can't silently break combat again. Two-layer defense: fix the data, then make the code robust against bad data.

### The `TWeakObjectPtr` identity bug

**Problem:** `TWeakObjectPtr::Find` didn't correctly detect duplicates — fresh weak pointers to the same actor don't compare equal by identity.

**Solution:** Replaced with `HasSameIndexAndSerialNumber` loop. A subtle UE container API gotcha.

### The `f7ab81e` oversized commit message

**Problem:** The commit message claims `GetBestTarget`/`SwitchTarget`/`ClearAll` "added," but those landed in a previous commit (`4288d2e`). The diff is a single return-type change.

**Solution:** Acknowledged as a squash/commit-hygiene artifact. The actual code change was correct; the message was misleading.

---

## Reflection & Lessons Learned

The Aggro→Threat rename happening *before implementation* is the arc's best structural decision. When the vocabulary is right, every subsequent commit reads clearly. "Threat table" immediately communicates what the data structure does; "aggro" would have required constant disambiguation between the mechanic and the data.

The consolidation of four StateTree tasks into one `EngageTask` is the second key lesson: **fewer states = fewer transitions = fewer bug surfaces.** The original plan had Agro→Chase→Attack→AttackPosition with transitions between each. The final design has one state with two re-evaluation intervals. The StateTree graph is simpler, the C++ code is simpler, and the bugs are simpler.

The codebase audit was the arc's quiet hero. Two commits (`adc399b`, `e225051`) cleaned up 48+ files of accumulated tech debt — deprecated APIs, const violations, unused includes, DRY violations. This is the kind of work that doesn't ship new features but makes every subsequent feature cheaper to implement. The `GetSelfPawn<T>` template alone eliminated two concrete helper functions and their duplication.

What I'd do differently: the `EnemyChaseTask.h` file still exists unused after the EngageTask consolidation — I'd have deleted it in the same commit rather than leaving dead code. And I'd have fully reconciled `Docs/AI/NPC_AI_System.md` and `Architecture Overview.md` with the final 6-state flow in the same pass, rather than leaving them stale.

---

## Forward-Looking Content

| Phase | Status |
|---|---|
| A3 — AI Systems (StateTree + Threat) | ✅ **100% (75/75)** |
| Sprint A5 — Multiplayer & Steam | ⏭️ Next |
| A5b — Persistence & Account System | Planned |
| A5c — Auth Extraction & Login Server | Planned |

**What's next:** A3 is fully closed. The threat table is archived to DONE. The next arc opens Sprint A5 — making the demo server-authoritative and multiplayer-safe. This means `HasAuthority()` guards on every server-only system, a full replication pass for NPCs and GAS, a dedicated-server build, and Steam auth tickets so players can actually log in together. The threat subsystem gets its first real stress test: two players, one `ThreatTable`, and NPCs that must decide who really hit them hardest.

> **Next time in Post 11:** Server's Word Is Law — HasAuthority() guards, replication, and the Steam-authenticated dedicated server.

---

*Arc commits: `8d38b8f` → `e854da8`. Related docs: `Docs/AI/Threat_System.md`, `Docs/AI/NPC_AI_System.md`, `Docs/Architecture/Architecture Overview.md`, `Planning/Sprint_A5_Multiplayer.md`, `Planning/Outlines/Episode_List.md`.*
