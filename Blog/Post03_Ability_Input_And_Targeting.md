# 🎬 Post 3 — You Can Fight Now: Ability Input, Target Data, and the PvP Toggle

*3 commits · May 28, 2026 · covering A1.4 ability input pipeline, A1.5 targeting library, A1.6 PvP toggle — sprint A1 reaches 100%*

---

## Introduction

At the end of Post 2, Onset had everything a player needs to *exist* in the world: camera, movement, targeting — even a hand-placed test enemy. But the ability buttons were decorative. The `IA_Ability1-4` actions existed as assets with no bindings, no handlers, and no way to turn "I clicked an enemy" into "my character hit that enemy."

This arc, a single eight-hour day, closes the loop. Three commits take the game from "click-to-target" to "target gated by validation → structured target data → choose whether players can even be targeted." It also closes sprint A1 at **38/38 (100%)**, the first fully complete milestone of the project. Along the way it introduces two patterns that define the rest of the codebase: a **validation gate** on targeting and a **static library** for data production.

---

## Technical Exposition

### The problem: abilities need targets, targets need validation

Post 2's `UTargetingComponent::SetTarget()` happily accepted anything. Abilities didn't exist yet, so nobody cared. But the moment you wire up four ability buttons, "target anything" becomes a real hazard: what if there's no target? What if you somehow target yourself? What if — once multiplayer lands — you target another player who hasn't opted into PvP?

The design answer was to make **validation a property of setting the target**, not a scattered check at each ability. If bad targets can't enter `CurrentTarget`, then every consumer of targeting is automatically safe.

### Why a static library for target data

The first design (from the daily TODOs) called for a full `UAbilityTargetingComponent` with cursor raycasts, ground-location queries, and direction math. The commit that shipped it deliberately stripped that down: *"No component, no PlayerController changes — abilities call the library directly."*

The reasoning was scope discipline. A component is stateful machinery; what the ability stubs actually needed was a pure function: *"given my current target and my position, give me actor, location, and direction."* A static library function returning a plain struct is the minimal thing that satisfies that. The fancier targeting modes (single / AoE / directional) got deferred to the GAS system in sprint A4, represented here as three fields in one struct rather than three methods.

---

## Implementation Details

### 1. Bind the ability actions and gate the target (A1.4)

The four actions get bound in `SetupInputComponent`, each to a stub handler. The important change is upstream, in `UTargetingComponent::SetTarget`:

```cpp
void UTargetingComponent::SetTarget(AActor* NewTarget)
{
	if (!IsActorTargetValid(NewTarget)) return;
	CurrentTarget = NewTarget;
	UE_LOG(LogTemp, Warning, TEXT("Setting target to %s"), *NewTarget->GetName());
}

bool UTargetingComponent::IsActorTargetValid(AActor* Actor)
{
	return Actor != nullptr && Actor != GetOwner();
}
```

A null target or self-target simply never becomes the current target. Simple, and it means every downstream reader of `GetTarget()` can trust what it finds.

The touch devices get wired too. `InjectAbilityInput()` lets blueprint widgets push ability presses through the *same* Enhanced Input path the keyboard uses — the pattern established in Post 2's joystick, now applied to combat:

```cpp
void AOnsetPlayerController::InjectAbilityInput(int32 AbilityIndex, bool bPressed)
{
	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!Subsystem) return;

	UInputAction* Action = nullptr;
	switch (AbilityIndex)
	{
	case 1: Action = IA_Ability1; break;
	case 2: Action = IA_Ability2; break;
	case 3: Action = IA_Ability3; break;
	case 4: Action = IA_Ability4; break;
	default: return;
	}
	Subsystem->InjectInputForAction(Action, FInputActionValue(bPressed), {}, {});
}
```

### 2. Produce structured target data (A1.5)

The targeting library defines the data contract between targeting and abilities:

```cpp
USTRUCT(BlueprintType)
struct FAbilityTargetData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Ability Targeting")
	AActor* TargetActor = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Ability Targeting")
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Ability Targeting")
	FVector TargetDirection = FVector::ZeroVector;
};
```

And the static function that fills it:

```cpp
FAbilityTargetData UAbilityTargetingLibrary::GetTargetData(
	UTargetingComponent* TargetingComponent, AActor* SourceActor)
{
	FAbilityTargetData Data;

	if (!TargetingComponent) return Data;

	Data.TargetActor = TargetingComponent->GetTarget();
	if (!Data.TargetActor) return Data;

	Data.TargetLocation = Data.TargetActor->GetActorLocation();

	if (SourceActor)
	{
		Data.TargetDirection = (Data.TargetLocation - SourceActor->GetActorLocation()).GetSafeNormal();
	}

	return Data;
}
```

The ability stubs were rewritten from inline logging to calling this library — the log message literally became `Ability %d — no target` when the returned struct is empty. This struct later becomes `FOnsetTargetData` (it collides with a UE 5.8 engine type, a fun bug discovered in Post 5) and feeds GAS directly.

### 3. The PvP toggle (A1.6)

This is the biggest commit of the three. The PvP flag lives on the **replicated `AOnsetPlayerState`**, toggled through a server RPC — the server is authoritative, clients only react:

```cpp
// AOnsetPlayerState.h
UPROPERTY(ReplicatedUsing = OnRep_PvPEnabled)
bool bIsPvPEnabled;

// Server RPC on the controller
UFUNCTION(Server, Reliable)
void Server_SetPvPEnabled(bool bEnabled);
```

A second validation predicate makes the rule explicit: **players are only valid targets when the source has PvP enabled.**

```cpp
bool UTargetingComponent::IsActorTargetPVPValid(AActor* TargetActor, AActor* SourceActor)
{
	if (const AOnsetPlayerCharacter* TargetCharacter = Cast<AOnsetPlayerCharacter>(TargetActor); !TargetCharacter) return false;
	if (const AOnsetPlayerController* SourceController = Cast<AOnsetPlayerController>(SourceActor->GetInstigatorController()))
	{
		AOnsetPlayerState* SourcePlayerState = SourceController->GetPlayerState<AOnsetPlayerState>();
		if (SourcePlayerState && !SourcePlayerState->bIsPvPEnabled) return false;
	}
	return true;
}
```

When a player turns PvP off, the client cleans up its own targeting immediately via the replication callback:

```cpp
void AOnsetPlayerState::OnRep_PvPEnabled()
{
	UE_LOG(LogNet, Log, TEXT("AOnsetPlayerState::OnRep_PvPEnabled()"));
	if (bIsPvPEnabled) return;
	AOnsetPlayerController* Controller = Cast<AOnsetPlayerController>(GetPlayerController());
	if (Controller)
	{
		UTargetingComponent* TargetingComponent = Cast<UTargetingComponent>(
			Controller->GetComponentByClass(UTargetingComponent::StaticClass()));
		if (TargetingComponent)
		{
			AActor* Target = TargetingComponent->GetTarget();
			if (Target && Target->ActorHasTag("Player"))
			{
				TargetingComponent->ClearTarget();
			}
		}
	}
}
```

Finally, a new `UOnsetStatics` blueprint function library centralizes the awkward `Cast` plumbing that Blueprint widgets otherwise struggle with — a small but recurring pattern for keeping blueprint graphs clean.

---

## Results & Validation

- Four ability actions bound and firing through a validation-gated targeting pipeline.
- `UAbilityTargetingLibrary` producing actor/location/direction target data for GAS.
- Replicated PvP toggle: server-authoritative, client-cleaned, player-vs-player targeting gated.
- Touch bridge (`InjectAbilityInput`) matches the keyboard path exactly.
- **Sprint A1: 38/38 tasks complete — the first 100% milestone.**
- Demo level gains its first hand-placed enemy (`BP_OnsetBaseEnemy`) — and immediately exposes that the world has no way to populate enemies at scale.

---

## Challenges & Solutions

### Component-null chasing

**Problem:** Post-2 symptoms of null components in the interaction handler.

**Solution:** Temporary `UE_LOG` validity checks in the constructor plus a permanent null guard in `OnPrimaryInteraction`. The guard is defensive code that earned its place.

### Double-scaling input

**Problem:** Ability values scaled twice (legacy + Enhanced Input).

**Solution:** `DefaultInput.ini` — `bEnableLegacyInputScales=False`. A config-level fix worth remembering whenever mixing input systems.

### The latent bug that would surface later

**Problem:** `OnRep_PvPEnabled()` fetches `UTargetingComponent` from the controller via `GetComponentByClass`.

**Solution (later):** Targeting actually lives on the **pawn**, not the controller — the controller resolves it in `OnPossess`. This shipped "correct-enough" (the lookup fails silently, PvP just doesn't clear the target on the owning client) and got corrected in a later arc. Good example of code that *looks* right, works in PIE, and hides a subtle ownership mistake that only multiplayer testing reveals.

### Doc drift

**Problem:** `PVP_System.md` names `IsActorValidTarget()` while code shipped `IsActorTargetPVPValid()`.

**Solution:** Docs describe intent; the code drifted one commit ahead. Noted, not fixed — a recurring rhythm in this project where code moves faster than docs, and later "make the docs true" passes catch up.

---

## Reflection & Lessons Learned

This arc taught me the value of **making invalid states unrepresentable**. By gating `SetTarget`, the entire downstream surface (abilities, HUD, later GAS) never has to check "is this a legal target?" because the answer is structurally guaranteed. The PvP rule extended the same idea: a player who hasn't opted into PvP can't even *be* selected, so there's no damage-filtering edge case to write later.

The static-library decision also aged well. A pure function + plain struct is trivially testable, trivially reusable (NPCs will use the same library in later arcs), and carries none of the lifecycle baggage a component would. Deferring targeting *modes* to GAS kept this arc small while still building the data shape GAS would need.

What I'd do differently: I'd have written `OnRep_PvPEnabled` to resolve targeting from the pawn on day one. The subtle "works in PIE, wrong in multiplayer" class of bug is the most expensive kind, because nothing in editor testing surfaces it.

---

## Forward-Looking Content

| Phase | Status |
|---|---|
| A1 — Core player systems (setup→PvP toggle) | ✅ **100% (38/38)** |
| A2 — NPC lifecycle: character, spawner, pooling, groups | ⏭️ Next |
| A3 — AI: StateTrees, perception, behavior | Planned |
| A4 — Combat: GAS, death, corpses | Planned |

**What's next:** with one hand-placed enemy and no way to populate the world, the next arc builds the NPC pipeline: the `AOnsetEnemy` pawn, a slot-based spawner, object pooling for performance, and a group system — all data-driven through `UAIProfile` assets so enemies can vary without new classes. Expect a memorable subplot: the fallback cube mesh that gets added, reverted three minutes later, and rebuilt differently.

> **Next time in Post 4:** NPC Infrastructure in a Day — spawner → pool → groups → AI profiles, and the fallback cube mesh saga.

---

*Arc commits: `eff5f48`, `5dff7ed`, `f433986`. Related docs: `Docs/Gameplay/Ability_Targeting_System.md`, `Docs/Gameplay/Targeting_System.md`, `Docs/Gameplay/PVP_System.md`.*
