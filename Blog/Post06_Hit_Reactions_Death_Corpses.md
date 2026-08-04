# 🎬 Post 6 — Death Is a Fork, Not a State: Hit Reactions, Corpse Actors, and the Pool Migration

*9 commits · Jun 4–7, 2026 · covering A4.3 hit reactions, A4.4 NPC attack integration, A4.5a death, and A4.5b corpse system*

---

## Introduction

At the end of Post 5, the basic attack *dealt damage* — a number went down. But nothing in the world responded: no flinch, no feedback, no consequence when health hit zero. Combat is 90% feedback, and this arc builds all of it: event-driven hit reactions, NPC attacks that respect GAS cooldowns, and a complete death lifecycle where an enemy turns into a corpse, returns to the pool, and respawns on its own timer.

Two architectural themes run through these four days. First, **death is a fork, not a state** — a single detection point that fans out to an event, a direct call, and a cleanup chain, rather than a tag the whole system has to poll. Second, **lifecycles get separated**: hit-reaction timing leaves the StateTree and moves into GAS; corpse visual debris gets decoupled from the expensive pooled AI actors; the pool itself graduates from a level-placed actor to a `UWorldSubsystem`.

---

## Technical Exposition

### Who detects damage? The AttributeSet.

`PostGameplayEffectExecute` is the single point where every damage value lands. Making it the *sole* detector of damage — and later, of death — means the rest of the system never has to track health transitions itself. When a hit lands, the attribute set doesn't just update a number; it emits a **gameplay event** that GAS abilities can listen to:

```cpp
if (Data.EvaluatedData.Magnitude < 0.0f)
{
	FGameplayEventData Payload;
	Payload.EventTag = TAG_Event_HitReaction;
	Payload.Instigator = Data.EffectSpec.GetContext().GetInstigator();
	Payload.Target = GetOwningActor();
	Payload.EventMagnitude = FMath::Abs(Data.EvaluatedData.Magnitude);

	if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
	{
		ASC->HandleGameplayEvent(TAG_Event_HitReaction, &Payload);
	}
}
```

This is event-driven combat: **damage is data, the event is the announcement, and abilities are the subscribers.** No subsystem owns a polling loop over health bars.

### Why the StateTree stopped owning its own cooldown

Post 5's `AttackTask` ran its own cooldown timer — a float it counted down. This arc removes that: GAS now owns combat timing through the `Cooldown.BasicAttack` tag, and the task becomes a passive observer that *polls* the tag before activating. One source of truth for combat timing, at the cost of the StateTree being a dumb observer. For a system that must coordinate abilities, effects, and tags, giving the timing to GAS is the right trade.

---

## Implementation Details

### 1. The hit-reaction ability (A4.3)

`UOnsetGA_HitReaction` is an instanced ability triggered by the `Event.HitReaction` gameplay event. On activation it commits, applies a cooldown GE (so it can't re-trigger instantly), and applies a stagger GE:

```cpp
if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
{
	EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
	return;
}

// Apply cooldown GE (blocks re-triggering)
if (CooldownGameplayEffectClass)
{
	(void)ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, CooldownGameplayEffectClass, GetAbilityLevel());
	if (StaggerEffectClass)
	{
		(void)ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, StaggerEffectClass, GetAbilityLevel());
	}
}
EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
```

The hit now has a visible consequence (stagger) and a cadence (cooldown) — and both are GAS-managed, stackable, and later replaceable by real animation/flinch logic.

### 2. The death fork (A4.5a)

When health hits exactly zero, the attribute set fans out to *both* a GAS event and a direct virtual call:

```cpp
if (GetHealth() == 0.0f)
{
	if (OldHealth != GetHealth())
	{
		FGameplayEventData Payload;
		Payload.EventTag = TAG_Event_Death;
		Payload.Instigator = Data.EffectSpec.GetContext().GetInstigator();
		Payload.Target = GetOwningActor();

		if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
		{
			ASC->HandleGameplayEvent(TAG_Event_Death, &Payload);
		}

		if (AOnsetBaseCharacter* Character = Cast<AOnsetBaseCharacter>(GetOwningActor()))
		{
			Character->OnDeath(Data.EffectSpec.GetContext().GetInstigator());
		}
	}
}
```

The event keeps the door open for GAS-reactive systems (the docs explicitly note "for future corpse system"); the direct `OnDeath()` call is the immediate action path. Notably, a `State.Dead` tag existed in the tags file but was never used for the death path — death here is **event + call**, not a state to poll.

The subtle choice: **defer cleanup to next tick.** You don't want to return the pawn to the pool *inside* an active GE callback. `SetTimerForNextTick` lets the corpse spawn synchronously (while the transform is still valid) and defers the pool return.

### 3. The corpse system (A4.5b)

Enemy death now does two parallel things: spawn a lightweight corpse and (next tick) run the pool-return cleanup:

```cpp
void AOnsetEnemy::OnDeath(AActor* KillingActor)
{
	if (UOnsetCorpseSubsystem* CorpseSub = GetWorld()->GetSubsystem<UOnsetCorpseSubsystem>())
	{
		UStaticMesh* CorpseMesh = Profile->CorpseMesh.IsNull() ? nullptr : Profile->CorpseMesh.LoadSynchronous();
		CorpseSub->SpawnCorpse(GetActorTransform(), CorpseMesh);
	}

	GetWorldTimerManager().SetTimerForNextTick(this, &AOnsetEnemy::DeferredDeathCleanup);
}
```

The corpse is a deliberate *non*-pooled actor: replicated, no tick, `SetLifeSpan` for cleanup, with a hard cap and oldest-eviction:

```cpp
AOnsetCorpse* UOnsetCorpseSubsystem::SpawnCorpse(const FTransform& Transform, UStaticMesh* CorpseMesh)
{
	SweepDeadCorpses();

	while (ActiveCorpses.Num() >= MaxActiveCorpses && ActiveCorpses.Num() > 0)
	{
		if (AOnsetCorpse* Oldest = ActiveCorpses[0].Get())
		{
			Oldest->Destroy();
		}
		ActiveCorpses.RemoveAt(0);
	}
	// ...spawn, SetLifeSpan(CorpseLifespan), add to ActiveCorpses
}
```

The rationale (from `Docs/AI/Corpse_System.md`, written the day before the code): *decouple world-debris from high-cost AI/ASC actors.* Corpses don't need an ability system, a perception component, or a StateTree — they're a static mesh with a lifespan. Pooling them would be over-engineering; a cap + eviction is the right cheap answer.

### 4. The spawner orchestrates respawn

The spawner owns respawn, not the enemy or the pool. On NPC death it clears the slot, starts a **per-slot** timer, and releases the NPC:

```cpp
void AOnsetSpawner::OnNPCDeath(AOnsetEnemy* Enemy)
{
	if (!Enemy || !PoolManager) return;

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		if (Slots[i].Occupant == Enemy)
		{
			Slots[i].Occupant = nullptr;
			GetWorldTimerManager().SetTimer(
				Slots[i].RespawnTimerHandle,
				FTimerDelegate::CreateUObject(this, &AOnsetSpawner::RespawnNPC, i),
				Config.RespawnDelay,
				false);
			break;
		}
	}

	PoolManager->ReleasePooledEnemy(Enemy);
}
```

Per-slot timers mean a 10-NPC kill cascade respawns *independently*, never in lockstep — the difference between a living spawner and a wave-synchronized one.

### 5. PoolManager → PoolSubsystem

The level-placed `AOnsetPoolManager` actor becomes a `UOnsetPoolSubsystem` (a `UWorldSubsystem`). No more placed-actor dependency, no manual ref wiring — anyone calls `GetWorld()->GetSubsystem<UOnsetPoolSubsystem>()`. The cost is lifecycle discipline: the subsystem must implement `OnWorldBeginPlay` instead of `BeginPlay`. Which is exactly where the arc's one-line bug bites:

```cpp
void UOnsetPoolSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	InitializePool();
}
```

---

## Results & Validation

- Hit reaction: damage event → instanced hit-reaction ability → cooldown + stagger GEs.
- NPC attacks poll GAS cooldown tags instead of owning timers.
- Death fork: `Event.Death` + direct `OnDeath()`, deferred cleanup, per-slot respawn.
- Player death: heal + teleport to home (a stub, refined in later arcs).
- Corpse system: replicated, cap + eviction, decoupled from AI actors.
- Pool migrated to `UWorldSubsystem` with correct lifecycle.
- `UOnsetCheatManager` with `God`/`Heal` — damage testing without a health bar.

---

## Challenges & Solutions

### The missing `Super::OnWorldBeginPlay` (the one-liner)

**Problem:** The freshly migrated `UOnsetPoolSubsystem` called `InitializePool()` without the super call — breaking the subsystem lifecycle. And it shipped while other code was already consuming the subsystem.

**Solution:** One line. The lesson isn't the fix, it's the pattern: **any class that overrides an engine lifecycle hook must call `Super` unless there's a documented reason not to.** A regex sweep for overridden hooks would catch a whole class of this.

### The inverted debug guard

**Problem:** The `AttackTask` had a leftover inverted guard that activated the basic attack *inside the failure branch* — firing the ability exactly when state was invalid.

**Solution:** Cleaned up when the task was migrated onto the shared task base. Evidence that "it compiles" ≠ "it's not a trap."

### The God-mode guard placement

**Problem:** The invulnerability check initially sat inside the death-check block, so god mode only reverted *lethal* blows.

**Solution:** Moved outside the death check so **all** damage is reverted. A subtle ordering bug that only shows when you actually try to use the cheat.

### The corpse cap edge case

**Problem:** The eviction loop lacked a `MaxActiveCorpses > 0` guard — a cap configured to 0 means an infinite loop.

**Solution:** Guard added later. Edge cases from configuration, not code.

### UE 5.8 API break

**Problem:** `CooldownGameplayEffectClass` changed shape; `FClassFinder` stopped matching.

**Solution:** Switched to `FObjectFinder` and `.Object`. Tracking engine API churn between UE versions is a fact of life here.

---

## Reflection & Lessons Learned

The arc's core insight is the title: **death is a flow, not a state.** A single authority (the attribute set) detects the transition and fans out to the systems that care, rather than every subsystem polling for "am I dead?" That inversion — *detection is centralized, response is distributed* — is the pattern I'd reach for first in any game with a shared damage pipeline.

The PoolSubsystem migration taught me to respect **lifecycle hooks as contracts**. `BeginPlay` vs `OnWorldBeginPlay`, `Super` calls, deferral out of callbacks — these are the seams where pooled/recycled systems break. The arc also reinforced that **cheats are a testing tool you should build early**: `God`/`Heal` made every subsequent combat iteration faster.

What I'd do differently: the `State.Dead` tag that was declared and never used is a small signal of indecision — I'd either use it or not declare it. And I'd have written the `MaxActiveCorpses > 0` guard the first time, since I clearly knew the cap was configurable.

---

## Forward-Looking Content

| Phase | Status |
|---|---|
| A4.3/A4.4 — hit reactions, NPC attack integration | ✅ Complete |
| A4.5a — death, pool return, respawn | ✅ Complete |
| A4.5b — corpse system | ✅ Complete |
| A3.4 — group assist (noise → hearing), investigate | ⏭️ Next |
| E21/E22 — profile split, movement attribute | Next |

**What's next:** death works, but the combat *decision-making* is still thin — the attack task is a one-line cooldown poll. Post 7 turns it into a real combat loop: damage now emits noise events that draw nearby enemies in to investigate, `ChaseRange`/`AttackRange` move into profiles, and the monolithic `UAIProfile` gets split into focused AI/Visual/Perception assets. Plus the `UOnsetMovementAttributeSet` that finally makes movement speed GAS-driven.

> **Next time in Post 7:** Enemies That Hear, Hunt, and Hustle — group assist, the StateTree combat loop, and GAS-driven movement speed.

---

*Arc commits: `b1c850f` → `f4bbf25`. Related docs: `Docs/AI/Corpse_System.md`, `Docs/AI/Pooling_System.md`, `Docs/AI/Spawner_System.md`, `Docs/GAS/GAS_System.md`.*
