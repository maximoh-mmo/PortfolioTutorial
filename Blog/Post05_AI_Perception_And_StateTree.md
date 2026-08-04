# 🎬 Post 5 — From Stubs to StateTrees: Onset's NPC Brain and the Pooled AIController Leak

*17 commits · May 30 – Jun 3, 2026 · covering A3.1–A3.3 AI systems and the A4.1 GAS foundation*

---

## Introduction

Post 4 gave the NPCs bodies, spawners, pools, and group identity. What it didn't give them was a brain. An enemy could stand, get hit, and get reused — but it couldn't notice the player, decide to chase, or attack. This arc makes that brain real.

Over five days, the A3 sprint turns the pooled enemies into perception-driven creatures with a full StateTree behavior loop: Idle → Roam → Agro → Chase → Attack. It opens with a genuinely good simplification — deleting a hand-rolled group-assist event in favor of the engine's own AI Perception hearing pipeline — and it closes by laying the GAS foundation that the combat arcs will build on. In the middle sits the arc's hardest bug: a controller leak that only pooling could have created.

---

## Technical Exposition

### Why StateTree instead of Behavior Tree

The docs and commit history are consistent: StateTree was chosen for its data-driven, lightweight design. Tasks and conditions are plain C++ structs — no per-node actor/component overhead — sharing a static-helpers base. They're deterministic, cheap, and server-safe. The *topology* (which states connect to which, with what transition conditions) lives in a `.uasset`, so designers rewire behavior without recompiling, while the logic stays in code.

### The simplification that started it all

The arc's first commit deletes a half-built feature. Post 4's group system shipped a `NotifyMemberAttacked()` stub plus an `AssistRadius` — the plan being "when a group member gets hit, the group reacts." The commit message is blunt: **remove it, adopt AI Perception hearing for assist.**

Why? UE already provides cross-entity communication (`UAISense_Hearing::ReportNoiseEvent`), per-entity range (hearing radius), and cross-group filtering. Building a bespoke event system on top of the Group System meant reimplementing all three, worse. Instead:

- Damage emits a noise event.
- Every nearby controller's hearing radius decides who "hears" it.
- The hearing pipeline runs into the *same* `OnPerceptionUpdated` target-acquisition path as sight.

Group membership stops being a decision engine and becomes what it should be: an identity provider. The Group System was demoted, not deleted — and the codebase got simpler.

---

## Implementation Details

### 1. The StateTree schema and context task (A3.1)

A custom schema + a Global Task that feeds live state into the tree every tick. The context task is the bridge between the AI controller's `TargetingComponent` and the StateTree's instance data:

```cpp
EStateTreeRunStatus FOnsetStateTreeContextTask::EnterState(FStateTreeExecutionContext& Context,
                                                           const FStateTreeTransitionResult& TransitionResult) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	const AOnsetAIController* AIController = Cast<AOnsetAIController>(Context.GetOwner());
	if (!AIController) return EStateTreeRunStatus::Failed;

	if (AIController->TargetingComponent)
	{
		InstanceData.Target = AIController->TargetingComponent->GetTarget();
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FOnsetStateTreeContextTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	return EnterState(Context, FStateTreeTransitionResult());
}
```

Global tasks re-enter on every tick, so `Target` in the instance data stays fresh without any state-specific bookkeeping. This is the pattern every later task builds on.

### 2. Perception → target acquisition (A3.2)

`OnPerceptionUpdated` merges sight and hearing, then picks the nearest actor that isn't friendly:

```cpp
TArray<AActor*> PerceivedActors;
PerceptionComp->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
PerceptionComp->GetCurrentlyPerceivedActors(UAISense_Hearing::StaticClass(), PerceivedActors);

AActor* BestTarget = nullptr;
float BestDist = FLT_MAX;
FVector MyLocation = GetPawn()->GetActorLocation();

for (AActor* Actor : PerceivedActors)
{
	if (Actor == nullptr) continue;

	if (Actor->ActorHasTag(FName("Player")) == GetPawn()->ActorHasTag(FName("Player")))
	{
		continue; // same team
	}
	float Dist = FVector::DistSquared(MyLocation, Actor->GetActorLocation());
	if (Dist < BestDist)
	{
		BestDist = Dist;
		BestTarget = Actor;
	}
}
if (BestTarget == nullptr)
{
	TargetingComponent->ClearTarget();
	return;
}
TargetingComponent->SetTarget(BestTarget);
```

The friend/foe check is tag-based (same `Player` tag → skip). It's the same `TargetingComponent` the player uses, with the same validation — one targeting abstraction serving input and perception alike.

### 3. The pooled AIController leak (the arc's hard bug)

The worst bug of the arc came from the interaction of pooling and the engine's automatic AI possession. With `AutoPossessAI=Spawned` + `AIControllerClass` on the enemy:

- The engine auto-spawned a fresh controller on **every** `SpawnActor` — including the pool's pre-allocation.
- `ReturnToPool`'s `UnPossess()` left those controllers **orphaned** — one leaked actor per pool cycle.
- Double-possession suppressed `OnPossess`, so `StartLogic()` never ran.

The fix was structural: **pool controllers alongside pawns**, with the spawner as the sole possessor:

```cpp
AOnsetAIController* AOnsetPoolManager::GetPooledController()
{
	if (!bPoolInitialized) InitializePool();
	for (AOnsetAIController* Controller : ControllerPool)
	{
		if (Controller && Controller->IsHidden())
		{
			Controller->SetActorHiddenInGame(false);
			Controller->SetActorTickEnabled(true);
			Controller->StateTreeComponent->SetComponentTickEnabled(true);
			Controller->PerceptionComponent->SetComponentTickEnabled(true);
			return Controller;
		}
	}
	// Pool exhausted — fallback SpawnActor
	UE_LOG(LogPooling, Warning, TEXT("OnsetPoolManager: Controller Pool exhausted — spawning new Controllers as fallback."));
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AOnsetAIController* Controller = GetWorld()->SpawnActor<AOnsetAIController>(
		AOnsetAIController::StaticClass(), FTransform::Identity, Params))
	{
		ReleasePooledController(Controller);
		return Controller;
	}
	return nullptr;
}
```

Two parallel pools (`ObjectPool` for pawns, `ControllerPool` for controllers), one possessor, one reset path (`ResetForPool()`). More bookkeeping, but the invariant — *exactly one controller per NPC, zero orphaned actors* — is guaranteed. This design survives essentially unchanged into the `UOnsetPoolSubsystem` of the current codebase.

### 4. Behavior states as small C++ tasks (A3.3)

Each state is a `FStateTreeTaskCommonBase`-derived struct, later consolidated under a shared `FOnsetStateTreeTaskBase`. The Roam task shows the shape — nav-reachable point anchored at the enemy's home:

```cpp
EStateTreeRunStatus FOnsetStateTreeRoamTask::EnterState(FStateTreeExecutionContext& Context,
                                                        const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	InstanceData.bHasArrived = false;
	InstanceData.PauseTimer = 0.0f;

	AOnsetAIController* AIController = Cast<AOnsetAIController>(Context.GetOwner());
	if (!AIController) return EStateTreeRunStatus::Failed;
	AOnsetEnemy* Enemy = Cast<AOnsetEnemy>(AIController->GetPawn());
	if (!Enemy) return EStateTreeRunStatus::Failed;
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(AIController->GetWorld());
	if (!NavSys) return EStateTreeRunStatus::Failed;

	FVector Home = Enemy->HomeLocation;
	FNavLocation RandomPoint;
	if (NavSys->GetRandomReachablePointInRadius(Home, InstanceData.RoamRadius, RandomPoint))
	{
		InstanceData.Destination = RandomPoint.Location;
		AIController->MoveToLocation(InstanceData.Destination, InstanceData.AcceptanceRadius);
		return EStateTreeRunStatus::Running;
	}
	return EStateTreeRunStatus::Failed;
}
```

Transitions are gated by reusable conditions. The `DistanceCondition` is the workhorse — squared-distance compare with configurable operators:

```cpp
float Distance = FVector::DistSquared(SourceLocation, TargetLocation);
float DistanceThresholdSquared = InstanceData.DistanceThreshold * InstanceData.DistanceThreshold;

switch (InstanceData.Comparison)
{
case UE::StateTree::EComparisonOperator::Less:
	return Distance < DistanceThresholdSquared;
case UE::StateTree::EComparisonOperator::LessOrEqual:
	return Distance <= DistanceThresholdSquared;
case UE::StateTree::EComparisonOperator::Greater:
	return Distance > DistanceThresholdSquared;
case UE::StateTree::EComparisonOperator::GreaterOrEqual:
	return Distance >= DistanceThresholdSquared;
default:
	return false;
}
```

### 5. GAS foundation (A4.1/A4.2)

The arc closes with the first GAS code. `UOnsetAttributeSet` (Health/MaxHealth), native gameplay tags, an ASC on the base character, and — the moment combat becomes real — `UOnsetGA_BasicAttack`. The attribute set shows the GAS idioms that survive the whole project:

```cpp
void UOnsetAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// Clamp Health to [0, MaxHealth]
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
}

void UOnsetAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(UOnsetAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UOnsetAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
}
```

Two naming notes worth preserving: `FAbilityTargetData` from Post 3 was renamed `FOnsetTargetData` because it **collided with a UE 5.8 engine type**, and `TAG_Cooldown_Melee` became `TAG_Cooldown_BasicAttack` as the ability surface grew.

---

## Results & Validation

- Group-assist event deleted; AI Perception hearing adopted (the right engine tool).
- C++ StateTree schema + context task feeding live target state each tick.
- Sight + hearing perception with nearest-non-friendly acquisition.
- **Controllers pooled alongside pawns** — orphaned-controller leak fixed.
- Idle → Roam → Agro → LostTarget → Chase → Marooned states as reusable C++ tasks/conditions.
- First GAS: attribute set, tags, ASC, and a working `GA_BasicAttack` with a player auto-attack timer.

---

## Challenges & Solutions

### The controller leak (detailed above)

The root cause was trusting the engine's `AutoPossessAI` under pooling. The lesson: **pooling changes the lifecycle assumptions of everything it touches.** Any system that assumes "an actor is born, lives, dies" breaks subtly under "an actor is recycled." The fix — explicit controller pooling and a single possessor — is the durable pattern.

### The silent GE damage bug

**Problem:** `GE_BasicAttackDamage` never actually dealt damage.

**Solution (recorded in the daily TODO):** The effect had **Required Tags set**, silently blocking execution. Removing them made damage flow end-to-end. GAS's tag-filtering features are powerful and *silent* — worth remembering when damage mysteriously stops.

### The `FAbilityTargetData` name collision

**Problem:** Compile broke on a name that existed in the engine.

**Solution:** Renamed to `FOnsetTargetData`. A reminder to prefix custom types aggressively in UE.

### Hygiene failures

Two dedicated commits (`f9ef619`, `5dd86b7`) added missing UE copyright headers to new files — the classic failure mode when creating many files at speed. Also: ~20 debug `UE_LOG` calls accumulated during StateTree bring-up, all stripped in the final commit. Debug logging has a half-life; it must be swept before it becomes noise.

---

## Reflection & Lessons Learned

The best decision in this arc was **reusing the engine instead of building bespoke.** `NotifyMemberAttacked` was deleted, not fixed — a hard but correct call. When the engine's built-in pipeline (noise → hearing → perception) does exactly what you need, your custom code is liability, not asset. It's tempting to keep your own system because you already wrote half of it; the arc shows the value of abandoning it anyway.

The controller pooling was the hardest-won lesson. It reframed how I think about pooled actors: **a pooled actor is a resource, not an entity.** Its lifecycle is owned by the pool, and every subsystem touching it (possess, unpossess, reset, re-activate) must be centralized in one reset path.

I also learned the shape of a good GAS foundation: minimal (two attributes, one ability, native tags) but with the correct idioms in place — clamping in `PostGameplayEffectExecute`, `DOREPLIFETIME_CONDITION_NOTIFY`, tags as a first-class surface. Everything in later combat arcs hangs off those hooks.

---

## Forward-Looking Content

| Phase | Status |
|---|---|
| A3.1–A3.3 — StateTree schema, perception, behavior states | ✅ Complete |
| A4.1/A4.2 — GAS foundation, basic attack | ✅ Complete |
| A4.3 — hit reactions (event-driven) | ⏭️ Next |
| A4.4 — NPC attack integration into GAS | Next |
| A4.5 — death, pool return, respawn, corpses | Next |

**What's next:** the enemies now *notice* the player, but when the basic attack connects, nothing feels like it happened. Post 6 makes the hit land: the GAS `Event.HitReaction` event triggers a proper hit-reaction ability, NPC attacks stop owning their own cooldowns and poll GAS tags instead, and death becomes a fork — `Event.Death` + a direct `OnDeath()` call — that spawns a corpse, returns the NPC to the pool, and starts a respawn timer. Plus: the `PoolManager` actor becomes a `UWorldSubsystem`.

> **Next time in Post 6:** Death Is a Fork, Not a State — hit reactions, corpse actors, and the pool-manager → pool-subsystem migration.

---

*Arc commits: `ab36977` → `47076ad`. Related docs: `Docs/AI/NPC_AI_System.md`, `Docs/AI/Group_System.md`, `Docs/AI/Pooling_System.md`, `Docs/GAS/GAS_System.md`.*
