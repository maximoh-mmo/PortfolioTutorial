# 🎬 Post 7 — Enemies That Hear, Hunt, and Hustle: Group Assist, the Combat Loop, and GAS Movement

*12 commits · Jun 7–9, 2026 · covering A3.4 group assist, the StateTree combat loop, the profile split, and the movement attribute set*

---

## Introduction

Post 6 made hits feel real — stagger, corpses, respawn — but the enemies' *decision-making* was still a stub. The attack task was a one-line cooldown poll. A pack of enemies would happily ignore its dying neighbor. And the `UAIProfile` data asset had quietly become a god object holding mesh, animation, behavior, and perception all at once.

This arc is about making enemies *social* and *decisive*. It opens with the A3.4 work that finally wires the group-assist flow Post 4 planned and Post 5 simplified: damage emits a noise event, and nearby enemies — especially the victim's pack — come running to investigate. Then the combat loop gets real thresholds (`ChaseRange`/`AttackRange` on the profile, read by the transition conditions), followed by a proper search task and the long-planned Episode 21 refactor that splits `UAIProfile` into three focused assets. It closes with the GAS movement attribute set that fixes an actual speed-leak bug.

The through-line is **decomposition**: every commit carves a god-object or a blurry responsibility into something focused, and every split is motivated by a real bug, not aesthetics.

---

## Technical Exposition

### Assist through the engine's ears

The A3.4 design decision (made back on May 30, recorded in the TODOs) pays off now: group assist flows through **AI Perception hearing**, not the Group System. The Group System supplies only identity — "is this NPC in the victim's group?" — while the *range* of assist emerges naturally from each controller's hearing radius.

Damage emits a noise event with loudness proportional to the damage:

```cpp
if (AActor* OwningActor = GetOwningActor())
{
	UAISense_Hearing::ReportNoiseEvent(
		OwningActor->GetWorld(),
		OwningActor->GetActorLocation(),
		FMath::Abs(Data.EvaluatedData.Magnitude),  // Loudness = damage amount
		OwningActor,                                // Instigator
		0.0f                                       // Max range 0 = unlimited
		);
}
```

Every controller within hearing range stores the event; a `HearingCondition` gates the transition to investigate:

```cpp
bool FOnsetStateTreeHearingCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FOnsetStateTreeHearingConditionInstanceData& InstanceData = Context.GetInstanceData(*this);

	AOnsetAIController* Controller = FOnsetStateTreeTaskBase::GetController(Context);
	if (!Controller || !Controller->bHasPendingNoise) return false;

	// Noise expired? (No new noise within MaxTimeSinceLastNoise seconds)
	float TimeSinceLastHeardNoise = Controller->GetWorld()->GetTimeSeconds() - Controller->LastNoiseHeardTime;
	if (TimeSinceLastHeardNoise > InstanceData.MaxTimeSinceLastNoise) return false;
	return true;
}
```

The `InvestigateTask` then moves the NPC to the noise — with a social subtlety: **group members run, outsiders walk** (a speed multiplier). The victim's pack responds fast; casual bystanders drift over. Emergent, data-driven, and it reuses the exact same perception path as target acquisition.

### Thresholds belong to the profile, not the node

Before this arc, `DistanceCondition` read a hardcoded `DistanceThreshold` per node. Now it reads from the profile — one source of truth per enemy variant:

```cpp
if (InstanceData.DistanceSource == EOnsetStateTreeDistanceSource::AttackRange ||
    InstanceData.DistanceSource == EOnsetStateTreeDistanceSource::ChaseRange)
{
	AOnsetEnemy* Self = FOnsetStateTreeTaskBase::GetSelfEnemyCharacter(Context);
	if (!Self || !Self->Profile) return false;
	float Threshold = InstanceData.DistanceSource == EOnsetStateTreeDistanceSource::AttackRange
						  ? Self->Profile->AttackRange
						  : Self->Profile->ChaseRange;
	DistanceThresholdSquared = Threshold * Threshold;
}
```

Change an enemy variant's chase range in one place, and every transition in every tree using it updates. This is the same "single source of truth" discipline as the GAS cooldown tag from Post 6.

---

## Implementation Details

### 1. The search task: cone-restricted, nav-projected

After investigating, an enemy that lost the trail searches. `SearchTask` picks points within a cone in front of the NPC (so it actually *looks*, not teleports around), projects them onto the navmesh, and has a dual exit — minimum cycles *or* minimum duration:

```cpp
FVector2D RandomOffset2D = FMath::RandPointInCircle(InstanceData.SearchRadius);
FVector RandomPoint = InstanceData.SearchCenter + FVector(RandomOffset2D.X, RandomOffset2D.Y, 0.0f);

FVector DirToPoint = (RandomPoint - InstanceData.SearchCenter).GetSafeNormal();
float Dot = FVector::DotProduct(InstanceData.InitialForward, DirToPoint);
float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(InstanceData.ConeHalfAngle));

if (Dot >= CosHalfAngle)
{
	FNavLocation Projected;
	if (NavSystem->ProjectPointToNavigation(RandomPoint, Projected, FVector(InstanceData.SearchRadius * 0.5f)))
	{
		return Projected.Location;
	}
}
```

A yaw-sweep oscillation keeps the NPC looking around between moves. This task is pure C++ — the pattern of "small, deterministic decision primitives" continues.

### 2. The profile split (Episode 21)

The monolithic `UAIProfile` (behavior + mesh + anim + material + perception) becomes three focused data assets:

| Asset | Responsibility |
|---|---|
| `UAIProfile` | Behavior: StateTree, attack/chase ranges, flee threshold, assist radius |
| `UVisualProfile` | Appearance: skeletal mesh, animation blueprint, material |
| `UPerceptionProfile` | Senses: sight radius/angle, hearing range |

This is single-responsibility decomposition with a concrete payoff already visible in the codebase: `ApplyProfile(UVisualProfile*)` handles visuals, `ApplyPerceptionProfile` configures senses, and `UAIProfile` drives StateTree + combat. A god-object data asset is the same smell as a god class — data-oriented design says split them too.

### 3. The interaction component

`UInteractionComponent` is carved out of the PlayerController — the click-resolution logic (raycast → enemy? target : move) that Post 2 built inline becomes a proper component. Because it casts its owner to `AOnsetPlayerController`, it's naturally a no-op under AI control. Single responsibility again: the controller coordinates, the component does.

### 4. GAS-driven movement (Episode 22)

The arc's most defensible feature. Direct `MaxWalkSpeed` writes were a bug factory: they don't stack, they leak across pooled pawns (UE has no "re-run constructor" for recycled actors), and they bypass GAS entirely. The fix is a dedicated attribute set + dynamic gameplay effects:

```cpp
void UOnsetMovementAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	if (Data.EvaluatedData.Attribute == GetMovementSpeedAttribute())
	{
		float ClampedSpeed = FMath::Max(GetMovementSpeed(), 0.0f);
		SetMovementSpeed(ClampedSpeed);
		if (AOnsetBaseCharacter* Character = Cast<AOnsetBaseCharacter>(GetOwningActor()))
		{
			Character->GetCharacterMovement()->MaxWalkSpeed = ClampedSpeed;
		}
	}
}
```

Speed modifiers become `MultiplyCompound` GEs that **stack multiplicatively** (flee × search × stagger), and the pool's `ReturnToPool` removes active effects as the cleanup point:

```cpp
struct FActiveGameplayEffectHandle FOnsetStateTreeTaskBase::ApplyMovementSpeedModifier(
	const AOnsetBaseCharacter* Self, const float Magnitude)
{
	if (!Self || !Self->AbilitySystemComponent) return FActiveGameplayEffectHandle();

	UGameplayEffect* SpeedGE = NewObject<UGameplayEffect>(GetTransientPackageAsObject(), FName("DynamicSpeedModifier"));
	SpeedGE->DurationPolicy = EGameplayEffectDurationType::Infinite;
	FGameplayModifierInfo ModifierInfo;
	ModifierInfo.Attribute = UOnsetMovementAttributeSet::GetMovementSpeedAttribute();
	ModifierInfo.ModifierOp = EGameplayModOp::MultiplyCompound;
	ModifierInfo.ModifierMagnitude = FScalableFloat(Magnitude);
	SpeedGE->Modifiers.Add(ModifierInfo);

	return Self->AbilitySystemComponent->ApplyGameplayEffectToSelf(
		SpeedGE, 1.0f, Self->AbilitySystemComponent->MakeEffectContext());
}
```

The dynamic GE is built with `NewObject` in code — no BP asset dependency, one helper serving three tasks (Flee, Investigate, Search). The commit message is candid about the trigger: "fixes SearchTask speed leak."

---

## Results & Validation

- Damage → noise → hearing → investigate, with group members reacting faster than outsiders.
- `ChaseRange`/`AttackRange` on the profile, read by transition conditions (single source of truth).
- `HasTarget`/`HasNoTarget` conditions extracted and reused.
- `SearchTask`: cone-restricted, nav-projected, dual-exit.
- `UAIProfile` split into AI/Visual/Perception; `UInteractionComponent` extracted.
- `UOnsetMovementAttributeSet`: stacking, GE-driven speed modifiers; pool clears GEs on return.
- `Combat/` → `GAS/` directory migration; sprint plan (`Sprint_A3_A4.md`) established.

---

## Challenges & Solutions

### The inverted null check

**Problem:** `DistanceCondition` had `if (!AIController || AIController->GetPawn()) return false;` — it returned early whenever a pawn *existed*.

**Solution:** Flipped to `!AIController->GetPawn()`. Inverted boolean logic is invisible to the eye until a condition just... never fires.

### The fleeing NPC that stared at its chaser

**Problem:** FleeTask never cleared focus, so enemies literally ran *backward* while staring at the threat.

**Solution:** `ClearFocus(EAIFocusPriority::Gameplay)` in `EnterState`. Face where you're going.

### The SearchTask speed leak

**Problem:** `ExitState` called the base method that did nothing, leaving the pawn permanently slowed.

**Solution:** Deferred deliberately (acknowledged in the notes), then fixed in the same arc by the movement attribute set + GE removal on pool return. A real bug that directly *motivated* the GAS movement refactor.

### The spawner never applied perception

**Problem:** "Targeting was completely broken" — `ApplyPerceptionProfile` was never called by the spawner.

**Solution:** Fixed as part of the E22 pass. A class that *configures* systems must have every configuration path exercised.

### UE 5.8 UHT error

**Problem:** New search instance data failed UHT with `USTRUCT(BlueprintType)`/`BlueprintReadOnly`.

**Solution:** Dropped `BlueprintReadOnly` to match the existing pattern. Engine-specific, cheap fix.

---

## Reflection & Lessons Learned

The arc's deepest lesson is that **emergent behavior beats bespoke behavior.** Group assist wasn't implemented as a system — it fell out of damage → noise → hearing → investigate, all existing pieces composed. The assist range "just works" because it's hearing range; group urgency "just works" because of a multiplier on group membership. The less code you write for a feature, the fewer bugs it can have.

I also learned that **refactors should be motivated by bugs, and the refactor should fix them.** The movement attribute set wasn't a "let's be more GAS-idiomatic" whim — it was the fix for the SearchTask speed leak and the pooled-pawn speed leak. Same with the profile split and the InteractionComponent: each was justified by pain in the existing code. Refactor-as-bugfix is how you justify cleanup to yourself six commits deep into a sprint.

The `Combat/` → `GAS/` move, done as a pure rename before adding the new attribute set, is the right ordering: organize first, then grow into the organized space.

---

## Forward-Looking Content

| Phase | Status |
|---|---|
| A3.4 — group assist, investigate, search | ✅ Complete |
| E21 — profile split, interaction component | ✅ Complete |
| E22 — GAS movement attribute set | ✅ Complete |
| E22 correctness pass (PostAttributeChange, pool reset) | ⏭️ Next |
| A3.5 — Player AI autoplay | Next |

**What's next:** the new movement attribute set ships with two latent bugs — the speed-sync callback never fires for infinite GEs, and recycled enemies keep their death-state health. Post 8 fixes both, then attacks a visual problem: enemies chasing in a perfect clump. Expect a crowd-separation weight bump, a chase timeout, and the real fix — per-agent offset destinations — plus the source reorganization that clears space for the Player AI system coming in Post 9.

> **Next time in Post 8:** Fixing the Chase — GAS speed sync, pool reset, anti-bunching offsets, and the pre-autoplay refactor.

---

*Arc commits: `c5ae6db` → `f467770`. Related docs: `Docs/AI/NPC_AI_System.md`, `Docs/AI/Group_System.md`, `Docs/GAS/GAS_System.md`, `Docs/GAS/Episode22_MovementSpeedAttribute.md`, `Docs/Architecture/Architecture Overview.md`.*
