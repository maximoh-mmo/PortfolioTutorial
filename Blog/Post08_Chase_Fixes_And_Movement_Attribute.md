# 🎬 Post 8 — Fixing the Chase: GAS Speed Sync, Pool Reset, and Anti-Bunching Offsets

*7 commits · Jun 10–11, 2026 · covering the E22 correctness pass and the pre-autoplay refactor*

---

## Introduction

Post 7 shipped the movement attribute set and declared victory. The next two days are what victory looks like in practice: **everything that shipped quietly broken gets fixed.** The GAS speed-sync callback never fired for infinite effects. Pooled enemies respawned with `Health = 0` from their death state. And — the one you can *see* — enemies chasing the player converged into a single clump that looked like they were all trying to occupy the same pixel.

This arc is short (seven commits, two days) and almost entirely remedial. It's the arc that teaches the difference between "the feature works in a test" and "the feature works in a system." It ends with a structural refactor that sets up the next big thing: Player AI autoplay.

---

## Technical Exposition

### The silent bug: `PostGameplayEffectExecute` never fires for infinite effects

The movement attribute set from Post 7 synced `MaxWalkSpeed` in `PostGameplayEffectExecute`. Except that callback **only fires for instant and periodic effects** — and the dynamic speed modifiers (Flee, Investigate, Search) were all **infinite-duration** GEs. So the speed sync silently never ran.

The fix is `PostAttributeChange`, which fires on any attribute change regardless of effect type:

```cpp
void UOnsetMovementAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue,
	float NewValue)
{
	if (Attribute == GetMovementSpeedAttribute())
	{
		float ClampedSpeed = FMath::Max(NewValue, 0.0f);
		if (AOnsetBaseCharacter* Character = Cast<AOnsetBaseCharacter>(GetOwningActor()))
		{
			Character->GetCharacterMovement()->MaxWalkSpeed = ClampedSpeed;
		}
	}
}
```

This is a classic "it compiles, it runs, it doesn't do anything" bug. The lesson isn't the GAS API detail — it's that **a callback that never fires is indistinguishable from one that works**, unless you verify the actual behavior end-to-end.

### The leaked death state

Pooled enemies keep their actor state between lives. Post 6's death flow set `Health = 0`; without an explicit reset, the next activation of that pooled pawn spawned an enemy with **zero health**. The fix puts the reset at the one choke point every enemy lifecycle passes through — pool return:

```cpp
void AOnsetBaseCharacter::ResetAttributes()
{
	if (!AbilitySystemComponent) return;
	AttributeSet->InitHealth(AttributeSet->GetMaxHealth());
	AttributeSet->InitMaxHealth(100.0f);
	MovementAttributes->InitMovementSpeed(600.0f);
}
```

Called from `UOnsetPoolSubsystem::ReturnToPool`, right after active effects are removed:

```cpp
if (Enemy->AbilitySystemComponent)
{
	Enemy->AbilitySystemComponent->RemoveActiveEffects(FGameplayEffectQuery(), -1);
}
Enemy->ResetAttributes();
```

Reset on return, not on spawn — because return is where the pooled pawn "ends its life," and making the pool's exit condition pristine means every entrance is safe.

### The visible bug: clumping

The crowd-separation weight got bumped from `2.0` to `8.0` — a band-aid. The root cause was destination convergence: `MoveToActor(TargetActor)` gave every enemy the **exact same goal point**. The real fix assigns each agent a distinct destination via a random lateral offset perpendicular to the approach direction:

```cpp
const FVector TargetLoc = TargetActor->GetActorLocation();
const FVector SelfLoc = AIController->GetPawn()->GetActorLocation();
const FVector ApproachDir = (SelfLoc - TargetLoc).GetSafeNormal2D();
const FVector Right = FVector::CrossProduct(ApproachDir, FVector::UpVector).GetSafeNormal();
const float Spread = FMath::RandRange(InstanceData.SpreadRadius * 0.5f, InstanceData.SpreadRadius);
const float LateralT = FMath::RandRange(-1.0f, 1.0f);
InstanceData.OffsetLocation = TargetLoc + Right * LateralT * Spread;
AIController->MoveToLocation(InstanceData.OffsetLocation, InstanceData.AcceptanceRadius);
```

Because the offset is randomized on every `EnterState`, the StateTree retry loop (Chase-Failed → Selector) produces a *fresh* spread each attempt — cheap emergent formation without a formation/assignment system. Separation weight and offset are complementary layers: one pushes during flow, one spreads at the destination.

### The chase timeout

Crowd-simulated path following can deadlock — an enemy permanently blocked by another NPC or geometry, chasing forever. A hard time budget converts a forever-`Running` state into `Failed`:

```cpp
InstanceData.ChaseStartTime = AIController->GetWorld()->GetTimeSeconds();
AIController->MoveToActor(TargetActor, InstanceData.AcceptanceRadius);
// ...
float Elapsed = AIController->GetWorld()->GetTimeSeconds() - InstanceData.ChaseStartTime;
if (Elapsed > InstanceData.MaxChaseDuration)
	return EStateTreeRunStatus::Failed;
```

The StateTree routes `Failed → LostTarget → Selector`, giving the enemy a reset-and-retry loop instead of infinite stuck pathing.

### The profile moves to the controller

The previous arc put behavior data on the pawn; but the StateTree execution context's owner *is* the controller, and conditions read thresholds at evaluation time. The `UAIProfile` moves to the `AOnsetAIController` where the context can reach it — visual data stays on the pawn, behavior data lives where behavior executes:

```cpp
UFUNCTION(BlueprintCallable, Category = "AI")
const UAIProfile* GetAIProfile() const { return AIProfile; }
```

---

## Implementation Details

The arc's fixes in sequence:

1. **`be9b07a`** — move `AIProfile` to controller; `PostAttributeChange` speed sync.
2. **`c172625`** — `ResetAttributes()` on pool return (three hours after the last one, after the duplicate `AddSpawnedAttribute` calls got cleaned up).
3. **`fa17a74`** — chase timeout for stuck pathing.
4. **`4af0c64`** — crowd separation `2.0 → 8.0` (the band-aid).
5. **`6d8b663`** — whitespace lint (a discipline pass mid-bugfix).
6. **`14d99fe`** — per-agent offset chase (the real fix) + camera arm `1000 → 2000`.
7. **`6f1db6c`** — reorganize AI sources into `Enemy/` + `StateTree/`, rename the task base, and stub the Player AI autoplay system.

The camera change in `14d99fe` deserves a note: arm `1000 → 2000` for a wider FOV. With the player about to become AI-controlled and crowds spreading out, the test camera needs to see the whole field — a telemetry decision made in service of the next feature.

### The Player AI stubs (the quiet big deal)

The last commit leaves three stubs and a possession-swap surface:

- `AOnsetPlayerAIController` — owns a `UStateTreeAIComponent`, will drive the player pawn.
- `FPlayerAcquireTargetTask` / `FPlayerAttackTask` — header-only sketches.
- `ActiveEnemies` tracking in the pool + `EnableAutoCombat`/`DisableAutoCombat` on the PlayerController:

```cpp
void AOnsetPlayerController::EnableAutoCombat()
{
	if (!AutoCombatController || bAutoCombatEnabled) return;
	if (APawn* MyPawn = GetPawn())
	{
		AutoCombatController->Possess(MyPawn);
		UnPossess();
		bAutoCombatEnabled = true;
	}
}
```

The reorganization wasn't cosmetic — it was **prerequisite**. Two consumers of the same StateTree infrastructure (NPC AI + Player AI) were about to coexist, and clear separation had to come first. `FOnsetStateTreeTaskBase` becomes `FOnsetStateTreeTask`, and the shared primitives move into `StateTree/` where both NPC and Player tasks will live.

---

## Results & Validation

- Speed modifiers now actually sync `MaxWalkSpeed` (via `PostAttributeChange`).
- Recycled enemies no longer spawn with zero health.
- Chasing packs spread out into organic arcs instead of clumping.
- Stuck chases time out and retry with a fresh formation.
- AIProfile behavior data owned by the controller; visual data on the pawn.
- AI sources split into `Enemy/` + `StateTree/`; Player AI autoplay stubbed with possession-swap hooks.

---

## Challenges & Solutions

### Trial-and-error attribute registration

**Problem:** `be9b07a` added `AddSpawnedAttribute` calls in both the constructor and `PossessedBy`. Three hours later `c172625` removed both — the constructor call is ineffective (spawned attributes are transient), and `PossessedBy` duplicated existing logic.

**Solution:** A visible back-and-forth. The lesson: **when you're unsure where a GAS initialization belongs, expect to put it wrong the first time** — and clean up the wrong version in the same session, not six commits later.

### The clumping band-aid vs. the real fix

**Problem:** Separation `2.0 → 8.0` helped at flow-time but the clump returned at the destination.

**Solution:** The per-agent offset. The pair shows the value of layered diagnosis: tune the parameter, but keep hunting for the *root* cause. A good bug-fix session ends with the parameter change *and* the structural fix.

### Dead callbacks and the value of verification

The `PostGameplayEffectExecute` non-firing bug and the leaked health both share one trait: **they were invisible in a smoke test.** They needed a systemic view — "what actually happens across a full lifecycle?" — to surface. Both were caught within a day of shipping because the next integration step (respawning from a pool) exercised the full cycle.

---

## Reflection & Lessons Learned

This arc is the clearest statement of the project's rhythm: **build, then harden.** Every "completed" system needed a corrective pass once it met the wider system — pooling exposed lifecycle leaks, GAS exposed callback semantics, crowds exposed destination convergence. Treating these as normal (not as personal failure) is what let them get fixed in a day each.

The pool-return choke point deserves emphasis: **if every actor lifecycle passes through one function, put the reset there.** `ResetAttributes` + `RemoveActiveEffects` in `ReturnToPool` is the single line of defense against an entire class of recycled-state bugs, and it stays correct because nothing can bypass the pool anymore (Post 4 removed the `SpawnActor` fallback precisely so this invariant holds).

The reorganization-before-feature instinct is worth copying: before the Player AI feature added a second consumer of the StateTree infrastructure, the sources were split and named for coexistence. That's the "make the next feature cheap" move, and it's visible in hindsight as the commit that looks like housekeeping but is actually load-bearing.

---

## Forward-Looking Content

| Phase | Status |
|---|---|
| E22 correctness — speed sync, pool reset | ✅ Complete |
| Chase fixes — timeout, separation, offsets | ✅ Complete |
| Source reorg — `Enemy/` + `StateTree/` | ✅ Complete |
| A3.5 — Player AI autoplay | ⏭️ Next |

**What's next:** three stubs are waiting — `AOnsetPlayerAIController`, `PlayerAcquireTargetTask`, `PlayerAttackTask` — plus `ActiveEnemies` tracking and a possession-swap hook that hands the player pawn to a StateTree when the player goes idle. Post 9 turns those stubs into a full autoplay mode: the player character becomes AI-driven, acquiring targets from the pool, engaging with the same StateTree primitives the NPCs use, and handing control back the instant the human touches input.

> **Next time in Post 9:** Letting Go of the Wheel — Player AI autoplay, auto-acquire, auto-attack, and the idle-timer handover.

---

*Arc commits: `be9b07a` → `6f1db6c`. Related docs: `Docs/GAS/GAS_System.md`, `Docs/GAS/Episode22_MovementSpeedAttribute.md`, `Docs/AI/Pooling_System.md`, `Docs/AI/NPC_AI_System.md`, `Docs/AI/Player_AI_System.md`.*
