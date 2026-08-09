# 🎬 Post 16 — The Abilities Arrive: AoE, Cone, Shadowstep, and the Montage Attack

*2 commits · Aug 5, 2026 · covering Phase 1 Combat Abilities, Stagger Execution, Class Row Name Fix*

---

## Introduction

Post 15 called the series "complete." I knew better. The deferred work from A4.6 — the abilities I'd put off while the full combat design was still forming — was sitting in the planning docs with a giant ⏳ next to it. AoE. Cone. Shadowstep. A montage-driven basic attack. These aren't polish items; they're the difference between "you can swing a sword" and "you have a kit."

This post covers the two commits that finally shipped phase 1 of that ability set. The headline is the abilities themselves — a target-centered AoE burst, a directional frontal cone, a repositioning shadowstep, and a basic attack whose damage lands on the montage's impact frame. Underneath, there's a stagger execution that runs the game in slow motion for a tenth of a second, and a two-line fix that made character class names resolve against the data table.

The series isn't complete. It just learned to stop pretending.

---

## Technical Exposition

### Why overlap-based targeting instead of projectiles

All three new damage abilities resolve their targets with **overlap queries at activation time**, not with spawned projectiles. That's a deliberate choice for this combat style. The game is a top-down ARPG played with a mouse cursor or touch, and its targets are already established — the player locks a target through `UTargetingComponent`, and abilities apply damage on the server the instant they activate.

Projectiles would give the player dodge opportunities and feel great in a different game. But they add replication complexity (predictive spawning, impact-time checks), miss the point of this combat's instant-feedback pacing, and would have delayed the whole phase. Overlap queries give me the same "did I hit anything" answer with zero intermediate actors to manage. The trade-off I accepted: the damage is effectively hitscan, so the player must already be positioned correctly when they press the button. That's the correct feel for an ARPG with a targeting reticle.

### The two-layer targeting pattern

Each ability runs two checks in sequence:

1. **A broad spatial query** — a sphere overlap (AoE) or sphere sweep (Cone) that collects everything within range.
2. **A per-actor filter** — the cone narrows the sphere results with a dot-product gate; both abilities then apply PvP filtering per target.

This is the same pattern I used in `UTargetingComponent::IsActorTargetPVPValid` from A5 — validation is always per-target, never a blanket "this ability is allowed to hit players" flag. PvP toggled off mid-ability still protects every individual target.

---

## Implementation Details

### 1. The AoE — a target-centered sphere burst

`UOnsetGA_AoE` inherits from `UOnsetGameplayAbility` (the base I added for shared UI metadata, which Post 18's ability bar consumes). It's `InstancedPerActor`, resolves its target location from the targeting component, and runs a sphere overlap:

```cpp
AActor* TargetActor = Self->TargetingComponent->GetTarget();
FVector TargetLocation = Self->GetActorLocation();
if (IsValid(TargetActor))
{
    TargetLocation = TargetActor->GetActorLocation();
}

TArray<FOverlapResult> OverlapResults;
FCollisionShape Sphere = FCollisionShape::MakeSphere(AoERadius);
FCollisionQueryParams QueryParams;
QueryParams.AddIgnoredActor(Self);

World->OverlapMultiByChannel(OverlapResults, TargetLocation, FQuat::Identity,
                            OverlapChannel, Sphere, QueryParams);
```

If the player has no target, the burst defaults to centered on themselves — the AoE still goes off, it just centers on the caster. That decision came from playtesting: fizzling a queued ability because the target died between the press and the activation feels terrible. Centering on self keeps the feedback consistent.

Each hit actor gets its own `FGameplayAbilityTargetData_ActorArray` and a damage effect. The PvP filter is the familiar one:

```cpp
if (HitChar->IsA(AOnsetPlayerCharacter::StaticClass()))
{
    AOnsetPlayerState* SelfPS = Self->GetPlayerState<AOnsetPlayerState>();
    AOnsetPlayerState* TargetPS = HitChar->GetPlayerState<AOnsetPlayerState>();
    if (SelfPS && TargetPS && (!SelfPS->bIsPvPEnabled || !TargetPS->bIsPvPEnabled))
    {
        continue; // PvP disabled for either party
    }
}
```

### 2. The Cone — sphere overlap, then a dot-product gate

The cone is the same idea, but the broad query can't do a cone natively. I run a sphere sweep with `ConeRange` as the radius, then discard anything outside the half-angle using a dot product against the character's flattened forward vector:

```cpp
FVector Start = Self->GetActorLocation();
FVector FlatForward = Self->GetActorForwardVector();
FlatForward.Z = 0.f;
FlatForward.Normalize();

const float ConeDotThreshold = FMath::Cos(FMath::DegreesToRadians(ConeHalfAngle));
// ...
FVector ToTarget = HitChar->GetActorLocation() - Start;
ToTarget.Z = 0.f;
ToTarget.Normalize();

if (FVector::DotProduct(FlatForward, ToTarget) < ConeDotThreshold)
{
    continue; // Outside the cone
}
```

Flattening both vectors onto the ground plane matters in a top-down game — a slope under the target shouldn't push them out of an ability that, visually, is clearly pointing at them. The dot product is checked against `cos(halfAngle)`, so `ConeHalfAngle = 90°` yields a full 180° frontal arc.

### 3. The Shadowstep — reposition with an invulnerability window

Shadowstep is the ability I'm proudest of, because it was born from a design failure. The original plan called for a "dash" ability — a quick forward movement that feels good in an action game. But on a top-down camera with a targeting reticle, a straight-line dash mostly just moves you out of range of the thing you're trying to hit. It reads as a misclick.

The pivot: instead of dashing *toward* nothing, blink *behind* the nearest enemy within a distance gate:

```cpp
AActor* TargetEnemy = FindNearestEnemy(Self, DistanceGate);
if (!TargetEnemy)
{
    // No valid target - fizzle (no cooldown, no teleport)
    EndAbility(Handle, ActorInfo, ActivationInfo, false, true);
    return;
}

FVector BlinkLocation = TargetEnemy->GetActorLocation() + TargetEnemy->GetActorForwardVector() * -BehindOffset;

ACharacter* Char = Cast<ACharacter>(Self);
if (Char && Char->GetCharacterMovement())
{
    Char->SetActorLocation(BlinkLocation, false, nullptr, ETeleportType::TeleportPhysics);
}
```

The ability then applies an invulnerability effect whose duration is set with a SetByCaller magnitude:

```cpp
SpecHandle.Data->SetSetByCallerMagnitude(
    FGameplayTag::RequestGameplayTag(FName("Duration")), InvulnerabilityDuration);
Self->AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
```

The `ETeleportType::TeleportPhysics` flag is the subtle part — it tells the movement component not to interpolate, so the character doesn't visibly slide through the enemy's body. And the invulnerability window (0.5s by default) gives the reposition teeth: you arrive behind the enemy already protected while you turn to face it. The `TAG_State_Invulnerable` tag this grants is respected both by damage application in `PostGameplayEffectExecute` and by the stagger execution (below).

### 4. The montage-timed basic attack

The basic attack got a real upgrade: instead of dealing damage the frame the ability activates, it now plays a montage and schedules the damage for the impact frame. The old instant version was fine for testing the pipeline; this version has *weight*:

```cpp
if (AttackMontage && Self->GetMesh() && Self->GetMesh()->GetAnimInstance())
{
    float MontageDuration = Self->PlayAnimMontage(AttackMontage);
    if (MontageDuration > 0.0f)
    {
        FTimerDelegate TimerDelegate;
        TimerDelegate.BindUObject(this, &UOnsetGA_BasicAttack::ApplyDamageAfterDelay, Handle, ActorInfo, ActivationInfo);
        World->GetTimerManager().SetTimer(MontageTimerHandle, TimerDelegate, DamageTime, false);
        return; // Don't end ability yet - wait for montage
    }
}

// No montage or failed to play - apply damage immediately
ApplyDamageAfterDelay(Handle, ActorInfo, ActivationInfo);
```

The ability stays active until the delayed damage fires, then ends. If the montage can't play (no anim asset, mesh missing), it falls back to instant damage so the ability never dead-ends. The range check runs at activation and again at impact, so a target that kites out of range mid-swing isn't damaged.

### 5. The stagger execution — hitstop via global time dilation

`UExecCalc_Stagger` is a custom `UGameplayEffectExecutionCalculation`. It's what makes a hit feel like a hit: it launches the target back with knockback and drops global time dilation to 10% for 0.1 seconds:

```cpp
TargetCharacter->LaunchCharacter(KnockbackDir * KnockbackMagnitude, true, true);

UGameplayStatics::SetGlobalTimeDilation(World, 0.1f);

FTimerHandle TimerHandle;
World->GetTimerManager().SetTimer(TimerHandle, [World]()
{
    UGameplayStatics::SetGlobalTimeDilation(World, 1.0f);
}, 0.1f, false);
```

The lambda-based timer is worth calling out — no need for a member function or a dedicated tick to restore the dilation; the timer owns its cleanup. And the invulnerability check at the top means a staggered target can't be re-staggered out of a shadowstep window.

### 6. Wiring it to input

The abilities are granted with explicit GAS input IDs in `GrantDefaultAbilities` — AoE on input 1, Cone on input 2, shadowstep as a passive (no input slot):

```cpp
AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AoEAbility, 1, 1, this)); // Input ID 1
AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(ConeAbility, 1, 2, this)); // Input ID 2
AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(ShadowstepAbility, 1, INDEX_NONE, this)); // Passive
```

The controller routes `IA_Ability1`/`IA_Ability2` through `AbilityLocalInputPressed`, which activates whatever ability is bound to that input ID:

```cpp
void AOnsetPlayerController::OnAbility1(const FInputActionValue& Value)
{
    Server_DisableAutoCombat();
    ResetIdleTimer();
    if (AOnsetBaseCharacter* Self = GetPawn<AOnsetBaseCharacter>())
    {
        Self->AbilitySystemComponent->AbilityLocalInputPressed(1);
        Self->AbilitySystemComponent->AbilityLocalInputReleased(1);
    }
}
```

Each new ability also registered its own cooldown tag (`Cooldown.AoE`, `Cooldown.Cone`, `Cooldown.Shadowstep`) and ability-type tag (`Ability.Type.AoE`, etc.) — the tags Post 17's ability bar subscribes to.

### 7. The two-line class fix

`Server_CreateCharacter` looked up the class's data table row using `GetValueAsName`, which returns the enum's *internal* name (`EOnsetCharacterClass::Tank`). The data table rows were keyed by their *display* values — the pretty names. Non-ASCII names broke the lookup, silently falling back to default stats:

```cpp
FName RowName = *UEnum::GetDisplayValueAsText(CharacterClass).ToString();
FOnsetCharacterClassInfo* Row = ClassTable->FindRow<FOnsetCharacterClassInfo>(RowName, nullptr);
```

One line changed, and the character creation stats started matching the class the player actually picked.

---

## Results & Validation

- **AoE** — target-centered sphere burst (300u radius), PvP-filtered per target, self-centered fallback when no target
- **Cone** — frontal arc via sphere sweep + dot-product gate (90° half-angle, 500u range)
- **Shadowstep** — blink behind nearest enemy within 1500u, 200u behind offset, 0.5s invulnerability
- **Basic attack** — montage-timed damage with instant-damage fallback
- **Stagger** — knockback + 0.1s global hitstop, blocked by invulnerability tag
- **Input** — ability input IDs 1/2 wired, shadowstep granted passive
- **Fix** — class stats resolve via display value instead of internal enum name

The abilities activate from input, respect the targeting system, filter players correctly with PvP off, and the shadowstep's repositioning is genuinely fun — which is the real test.

---

## Challenges & Solutions

### The dash became a shadowstep

**Problem:** The planned "dash" ability felt like a mistake on a top-down camera. Moving in the direction you're already facing does nothing useful when your target is where you're looking.

**Solution:** Reconsidered what the ability is *for* — repositioning — and reworked it as a blink behind the nearest enemy within a distance gate. The invulnerability window turned it from a reposition into a defensive combo tool. This is the best argument I have for designing abilities against the camera, not against the genre.

### The cone needed two passes

**Problem:** Unreal has no native cone-overlap query, and a single sphere sweep would hit enemies standing *beside* but not *in front of* the caster.

**Solution:** Sweep a sphere, then validate each result with a dot product against the flattened forward vector. Flattening to the ground plane prevents terrain slope from incorrectly excluding targets. Cost is negligible for the handful of NPCs on screen.

### Global time dilation is global

**Problem:** `SetGlobalTimeDilation` affects *everything* — including the timer that's supposed to restore it, and the player's own inputs. A naive restore can get starved.

**Solution:** Arm the restore with a `World`-capturing timer lambda before the dilation takes effect, so the restore fires regardless of the dilation. Accept the global scope for now — it's a single-player-visible effect and 0.1s is imperceptible on other actors.

---

## Reflection & Lessons Learned

The two-layer targeting pattern — broad spatial query, then per-actor validation — ended up being the same shape across all three abilities. That's not an accident; it's the correct decomposition for "find candidates" vs "is this candidate valid," and it's the reason the PvP logic lives in one obvious place instead of being copy-pasted with subtle drift.

The shadowstep pivot taught me the most. A feature spec ("dash ability") is a hypothesis about fun, and it was wrong in this camera context. The moment I re-anchored the design to "what does the player actually want to accomplish," the right ability design appeared. I'm going to apply that test to every future ability: name the *intent*, not the *verb*.

I also under-anticipated how much of this phase was about tags. Every ability needed a cooldown tag, a type tag, an asset tag — and the ability bar (Post 17) and input wiring both depend on those tags existing consistently. The tag registry is quietly the most important file in this commit.

---

## Forward-Looking Content

| Phase | Status |
|---|---|
| A5 — Multiplayer + Steam | ✅ 100% (42/42) |
| A5b — Persistence & Account System | ✅ 100% (30/30) |
| A5c — Auth Extraction & Login Server | ✅ 100% (38/38) |
| A6 — Character Classes + Final Polish | ✅ 100% (12/12) |
| **A4.6 — Phase 1 Abilities (AoE, Cone, Shadowstep, Montage, Stagger)** | ✅ **Committed** |

With abilities on the input bar, the next obvious gap is showing the player what they're doing. Post 17 builds the in-game HUD — player health, the ability bar with real cooldown fills, a target frame, and pooled damage numbers that float up off the characters.

---

*Arc commits: `1768497`, `ef77c2d`. Related docs: `Docs/Gameplay/Ability_Targeting_System.md`, `Docs/GAS/GAS_System.md`.*
