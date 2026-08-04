# 🎬 Post 9 — Teaching the Player to Play Itself: Autoplay, the Idle-Timer Handoff, and a bIsAlive Lifecycle

*4 commits · June 18, 2026 · covering A3.5 — Player AI autoplay in a single 8-minute session*

---

## Introduction

For weeks the NPCs have had a brain. The player character — the whole reason the demo exists — has had nothing but input handlers. This arc fixes that asymmetry in the most literal way possible: **the player pawn gets possessed by an AI controller and plays itself.**

The four commits land in a single eight-minute window, and together they close out A3.5 (Player AI Autoplay). The idea is a testing harness as much as a feature: the docs frame it as *"autoplay/testing mode… useful for stress tests, demos, and debugging."* If the player can fight automatically, you can demo AI-vs-AI, stress-test the threat systems, and let the game stand on its own while you record footage. But the interesting engineering isn't the "watch me fight" part — it's the possession handoff, the lifecycle flag that makes the AI refuse dead enemies, and the input override that hands control back the instant a human touches anything.

---

## Technical Exposition

### Autoplay is a possession swap, not a simulation

The design decision documented in the sprint plan is that autoplay is implemented by **actually transferring possession**. `AutoCombatController->Possess(MyPawn)` then `UnPossess()`. The pawn is oblivious — `GrantDefaultAbilities` already has a `bAbilitiesGranted` guard, so re-possession doesn't re-grant. This is the "controller swap viability" decision, and it means the player AI uses the *exact same StateTree tasks* the NPCs use, with zero special-casing in the pawn.

### The lifecycle flag: `bIsAlive`

Before autoplay could exist, targeting had a correctness hole: pooled enemies return to the pool *hidden but alive*, and the pool recycles them with stale state. If the player AI scanned for targets by `IsHidden()` or `IsValid()`, it could "target" a corpse or a hidden-but-respawning NPC. The fix is an explicit lifecycle flag that stays in sync through every pool transition:

```cpp
void AOnsetBaseCharacter::ResetAttributes()
{
	if (!AbilitySystemComponent) return;
	AttributeSet->InitHealth(AttributeSet->GetMaxHealth());
	AttributeSet->InitMaxHealth(100.0f);
	MovementAttributes->InitMovementSpeed(600.0f);
	bIsAlive = false; // Pool return — OnRespawn() re-enables on retrieval
}

void AOnsetBaseCharacter::OnDeath(AActor* KillingActor)
{
	bIsAlive = false;
}

bool AOnsetBaseCharacter::IsAlive() const
{
	return bIsAlive && AttributeSet && AttributeSet->GetHealth() > 0.0f;
}

void AOnsetBaseCharacter::OnRespawn()
{
	bIsAlive = true;
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);
	SetActorEnableCollision(true);
}
```

The pool becomes the single hook point: retrieval calls `OnRespawn()`, return leaves the flag false.

```cpp
for (AOnsetEnemy* Enemy : ObjectPool)
{
	if (Enemy && Enemy->IsHidden())
	{
		ActiveEnemies.Add(Enemy);
		Enemy->OnRespawn();
		return Enemy;
	}
}
```

---

## Implementation Details

### 1. The ability tag family

Autoplay needs to know *which* abilities it may fire — the hit-reaction ability must never be AI-driven. A new `Ability.Type.*` tag family (`Attack`/`Buff`/`Debuff`/`Heal`) lands, and `GA_BasicAttack` gets tagged. The AI's engage logic later filters on `TAG_Ability_Attack`.

### 2. The idle timer and the handoff

`AOnsetPlayerController` runs a five-second idle timer. Any input resets it; if it fires, the AI takes over:

```cpp
void AOnsetPlayerController::EnableAutoCombat()
{
	if (!AutoCombatController || bAutoCombatEnabled) return;
	if (APawn* MyPawn = GetPawn())
	{
		AutoCombatController->Possess(MyPawn);
		UnPossess();
		GetWorldTimerManager().SetTimerForNextTick(this, &AOnsetPlayerController::DelayedSetViewTarget);
		bAutoCombatEnabled = true;
	}
}

void AOnsetPlayerController::ResetIdleTimer()
{
	GetWorldTimerManager().ClearTimer(IdleAutoCombatTimerHandle);
	if (IdleAutoCombatDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(IdleAutoCombatTimerHandle, this,
			&AOnsetPlayerController::EnableAutoCombat, IdleAutoCombatDelay, false);
	}
}
```

Every input handler starts with the same two lines — the input override that makes autoplay zero-friction:

```cpp
void AOnsetPlayerController::OnMove(const FInputActionValue& Value)
{
	if (bAutoCombatEnabled) DisableAutoCombat();
	ResetIdleTimer();
	FVector2D MovementVector = Value.Get<FVector2D>();
	if (MovementVector.IsZero()) return;
	StopMovement();
	// ...
}
```

### 3. AcquireTarget: the best living enemy

The task picks the nearest *alive* enemy within a leash/acquire range that lerps from the controller's tuning sliders, then confirms the target is navmesh-reachable:

```cpp
for (const TWeakObjectPtr<AOnsetEnemy>& Weak : Pool->GetActiveEnemies())
{
	AOnsetEnemy* Enemy = Weak.Get();
	if (!Enemy || !Enemy->IsAlive()) continue;
	float DistH = FVector::Dist(Enemy->GetActorLocation(), Home);
	if (DistH > Leash) continue;
	float DistS = FVector::Dist(Enemy->GetActorLocation(), SelfLoc);
	if (DistS > Range) continue;
	FNavLocation Projection;
	if (!NavSys->ProjectPointToNavigation(Enemy->GetActorLocation(), Projection, FVector(200.0f))) continue;
	if (DistS < BestDist) { BestDist = DistS; Best = Enemy; }
}
if (Best)
{
	FOnsetStateTreeTask::SetTarget(Context, Best);
	return true;
}
```

The `IsAlive()` filter is the whole point of the lifecycle flag from `b50004c`.

### 4. Engage: approach, then attack on a throttle

The engage task deliberately combines movement and attack in one state — a documented decision: *"movement and attack live in one task for simpler state management"* — rather than separate Chase/Attack states with transition flicker. Within range it stops, focuses, and fires ready offensive abilities on a 0.25s tick throttle:

```cpp
if (DistanceSquared > Self->AttackRange * Self->AttackRange)
{
	Controller->MoveToActor(Target, InstanceData.AcceptanceRadius);
	return EStateTreeRunStatus::Running;
}
Controller->StopMovement();
Controller->SetFocus(Target);

if (Now - InstanceData.LastAttackTick < InstanceData.AttackTickInterval)
	return EStateTreeRunStatus::Running;
InstanceData.LastAttackTick = Now;

UAbilitySystemComponent* ASC = Self->AbilitySystemComponent;
TArray<FGameplayAbilitySpec> Abilities = ASC->GetActivatableAbilities();

for (auto& AbilitySpec : Abilities)
{
	if (!AbilitySpec.Ability
		|| !AbilitySpec.Ability->CheckCooldown(AbilitySpec.Handle, ASC->AbilityActorInfo.Get())
		|| !AbilitySpec.Ability->GetAssetTags().HasTag(TAG_Ability_Attack))
		continue;
	ReadyAbilities.Add(AbilitySpec.Handle);
}
```

Note the perf guard: an AoE overlap scan only runs when *two or more* abilities are ready. The AI doesn't fire everything every frame; it behaves like a careful player.

---

## Results & Validation

- Player pawn possessed by an AI controller running the same StateTree tasks as the NPCs.
- Five-second idle trigger; any input cancels autoplay and resets the timer.
- `bIsAlive` lifecycle flag filtering dead/hidden enemies from target acquisition.
- Ability tag family (`Ability.Type.*`) gating which abilities the AI fires.
- Navmesh-projected target selection with leash/acquire tuning sliders.
- Camera handover via deferred `SetViewTarget` (later removed once the engine followed naturally).
- Docs rewritten to match — the recurring "make the docs true" discipline.

---

## Challenges & Solutions

### The camera race

**Problem:** Possession swap risks a black frame — the `PlayerCameraManager` cleanup doesn't wait.

**Solution:** `DelayedSetViewTarget` deferred one tick via `SetTimerForNextTick`. Later, as the architecture simplified, the whole workaround was deleted (`adc399b`, the Post 10 audit) because the camera followed the possessed pawn natively. A short-lived fix that did its job and then became dead code.

### The StateTree asset wouldn't load

**Problem:** Constructor `FObjectFinder` was flaky for the `PlayerAutoCombat` asset.

**Solution:** Moved to `LoadObject` with a fallback re-load inside `OnPossess`, plus a warning log. The current code adds a `WITH_EDITOR` `CompileIfChanged()` guard — evidence that the tree needed editor recompilation to run. Lesson: StateTree assets are editor-dependent; treat their load as fallible at runtime.

### The `PlayerAttackTask` → `PlayerEngageTask` rename

**Problem:** The plan said `AttackTask`; the design converged on a combined engage task.

**Solution:** Renamed before implementation. The docs-vs-code drift was corrected in the doc-sync commit, not left to accumulate.

---

## Reflection & Lessons Learned

The possession-swap approach was the right call, and it's the arc's biggest lesson: **reuse the engine's ownership model instead of building a parallel one.** The player AI controller is just another controller. The pawn never knows who's driving it. That's the kind of design that survives because it *removes* code paths rather than adding them.

The `bIsAlive` flag taught me the second lesson: **when a system (pooling) creates state ambiguity, give the state an explicit name.** `IsHidden()` is a rendering concept; `IsAlive()` is a gameplay concept. The autoplay AI was the first consumer to *require* the distinction, and the flag made the whole thing unambiguous.

What I'd do differently: I'd have kept the camera workaround only as long as it was needed and deleted it in the same sprint. It survived into the audit because it looked load-bearing when it wasn't. Dead code that *looks* important is more dangerous than dead code that looks dead.

---

## Forward-Looking Content

| Phase | Status |
|---|---|
| A3.5 — Player AI autoplay | ✅ Complete |
| A3 — AI Systems | ✅ Complete (91%) |
| Threat System (Aggro → Threat) | ⏭️ Next |
| Sprint A5 — Multiplayer & Steam | Next |

**What's next:** autoplay was built to *observe* fights — and observing fights immediately exposed the combat loop's weakness: a crowd of enemies converges into a clump, and "who should I hit?" has no answer when multiple players exist. The next arc builds the Threat System: a server-side threat table fed by the damage pipeline, threat × distance target scoring, deterministic angular combat spread, AI LOD — plus the codebase audit that deletes the camera workaround and renames the entire task family.

> **Next time in Post 10:** From Aggro to Threat — building a server-side threat table, simplifying the StateTree, and taming tick spam.

---

*Arc commits: `b50004c`, `b4c820e`, `2a37283`, `f9f6edf`. Related docs: `Docs/AI/Player_AI_System.md`, `Docs/AI/Pooling_System.md`, `Docs/GAS/GAS_System.md`.*
