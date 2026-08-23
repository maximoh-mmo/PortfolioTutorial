# 🔍 CODE AUDIT & DOC SYNC — 2026-08-21

Senior-programmer deep dive across `Project/Source/Onset` against the repo documentation
(`README.md`, `Docs/`, `TODO/`). Focus: comment quality, doc-vs-code accuracy, cleanliness.

---

## 1. Verification Results — Documentation Claims vs Code

| Claim (source) | Verdict | Evidence |
|---|---|---|
| Pool return clears all GEs (A4.5b/E22) | ✅ Matches | `OnsetPoolSubsystem.cpp` → `RemoveActiveEffects(FGameplayEffectQuery(), -1)` before `ResetAttributes()` |
| AI LOD: 3 tiers (A3.6) | ✅ Matches | `OnsetAIController.cpp::UpdateLodTier` — full / 0.2s throttle / 0.5s + StateTree paused |
| Debug logging stripped from ThreatSubsystem + EngageTask (A3.6) | ✅ Matches | Zero `UE_LOG` calls remain in `OnsetThreatSubsystem.cpp` |
| Threat multipliers: per-class + per-ability (README) | ✅ Matches | `FOnsetAbilityDefinition::ThreatMultiplier`, `FOnsetCharacterClassInfo::ThreatMultiplier`, cached via `UOnsetGA_Generic::GetThreatMultiplier()` |
| Sight-based threat adds base 1.0 when not engaged (A3.6) | ✅ Matches | `OnsetAIController.cpp::OnPerceptionUpdated` guards with `IsEnemyEngagedWithPlayer` |
| Assist flows via perception hearing, not Group System (A2.3/A3.4) | ✅ Matches | Noise state lives on `AOnsetAIController`; consumed by HearingCondition/InvestigateTask |
| AI-state debug readout exists (Future_Ideas "Full Debug Display") | ✅ Matches | `WITH_EDITOR DrawDebugString` of active StateTree states in `Tick` |
| Security audit "2 Server_ RPCs" (A5.3) | ⚠️ Stale | Was point-in-time; PlayerController now carries ~15 auth/persistence/auto-combat RPCs. **Annotated in checklist** — re-run during A7 |

## 2. Issues Found & Fixed This Session

### Code comments added
- **`OnsetAuthSubsystem.h`** *(security-critical, was completely bare)* — class-level doc
  explaining Direct vs Token modes, config keys, token format/HMAC scheme, and per-method +
  per-field docs (`PendingTokenAuthMap` keying, dev-client map).
- **`OnsetAIController.h`** — documented the assist-hearing state block
  (`HeardNoiseLocation`, `HeardNoiseInstigator`, `bHasPendingNoise`, `LastNoiseHeardTime`),
  LOD tier behavior, `LodTickCounter`, `bInUse`.
- **`OnsetBaseCharacter.h`** — `InitAbilityActorInfo()`, `GrantDefaultAbilities()`,
  `bAbilitiesGranted` guard rationale.
- **`OnsetEnemy.h`** — `OwningSpawner`, `OnRep_VisualProfile`, `DeferredDeathCleanup`.
- **`OnsetThreatSubsystem.h`** — class doc (authority-only, weak-pointer decay),
  `GetBestTarget` distance weighting (1.0 / 0.5 / 0.1) documented at the declaration,
  private table semantics.
- **`OnsetCorpseSubsystem.h`** — `SweepDeadCorpses`, `ActiveCorpses` ordering guarantee.

### Cleanliness fixes
- **`OnsetPoolSubsystem.cpp`** — two log messages still named the retired
  `OnsetPoolManager`; renamed to `UOnsetPoolSubsystem`. Copy-paste comment above the
  *controller* fallback wrongly said `AOnsetEnemy`; corrected. Removed stray template
  comment "// Sets default values".

### Documentation fixes
- **`Private_Demo_Checklist.md` progress table** — A6 and A7 rows had Tasks/Done columns
  swapped (A7 read "Tasks=0, Done=23, 0%"). Corrected to Tasks=30/Done=19 and 23/0.
- **Stale security-audit claim** annotated as point-in-time (see table above).

## 3. Flagged — Needs Owner Decision (not changed)

1. **~72 `UE_LOG(LogTemp, ...)` calls across 19 files** (24 in `OnsetPlayerController.cpp`
   alone). Several are `Warning` level on normal paths (possess logs, autoplay toggles,
   HUD creation). Recommend migrating to named categories (`LogOnset`, `LogAutoCombat`)
   or demoting to `Verbose` before recording Ep 48+.
2. **175 files carry the template placeholder** "// Fill out your copyright notice…".
   Cosmetic only; a scripted one-line replace would normalize them if desired.
3. **LOD band assumption**: `UpdateLodTier` assumes `HearingRange ≥ SightRange`. If a
   perception profile authors hearing < sight, an enemy beyond sight but inside hearing
   gets its StateTree paused — assist-by-hearing would stall until sight range. Worth a
   `checkf` in `ApplyPerceptionProfile` or an explicit band ordering.
4. **`PlayerEngageTask.cpp`** carried the codebase's single `TODO` (AoE vs single-target
   pick) — **resolved 2026-08-21**: replaced by expected-damage selection
   (`GetComparisonDamage × hits`, refresh-gated DoTs, longest-cooldown tie-break;
   see [Player_AI_System.md](../Docs/AI/Player_AI_System.md)).
5. **Fallback pool enemies/controllers** spawned fresh then run through the full
   teardown pass (`ReleasePooled*`) just to land in the pool array — roundabout, and
   **investigation escalated this to a real bug**: the teardown left fallback actors
   hidden/tick-disabled/collision-off and nothing downstream re-enabled them
   (`SpawnEnemyAtSlot` never un-hides), so exhaustion-grown NPCs would spawn invisible
   and inert. **Resolved 2026-08-21**: both fallback branches now register directly in
   the pool array and normalize state explicitly (enemy: `ResetAttributes` +
   `OnRespawn`; controller: un-hide + re-enable StateTree/perception ticks) — matching
   exactly what pooled retrievals return. Verify with a small `PoolSize` + oversized
   spawner group in PIE (A7.1 stress test will exercise this path at scale).

## 4. Overall Assessment

The codebase is in **good shape for its stage**: consistent naming, disciplined
server-authority guards, meaningful UPROPERTY documentation on nearly all gameplay-facing
members, and docs that overwhelmingly match implementation. The gaps were concentrated in
the auth/persistence layer (newest code), pooled-controller bookkeeping, and a handful of
stale names/comments — all addressed or flagged above.
