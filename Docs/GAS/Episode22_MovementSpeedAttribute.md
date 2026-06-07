# 🏗 Episode 22 — GAS Movement Speed Attribute

**Phase:** 3.5 — Architecture Cleanup
**Prerequisites:** Episode 15 (Basic Attack), Episode 21 (Architecture Cleanup), Episodes 25-26 (Flee + Group Assist)
**Core teach:** Replace scattered direct `MaxWalkSpeed` writes with a central GAS attribute — demonstrating stacking, SRP, and data-oriented speed control.

---

## The Problem

Currently, any system that changes movement speed writes directly to `UCharacterMovementComponent::MaxWalkSpeed`:

- `FOnsetStateTreeFleeTask` — caches original, applies multiplier, restores on exit
- `FOnsetStateTreeInvestigateTask` — same pattern duplicated

This has three issues:
1. **No stacking** — stagger slow + flee slow clobber each other, last write wins
2. **Scattered responsibility** — every task has a `CachedOriginalWalkSpeed` + restore logic
3. **Bypasses GAS** — GAS effects can't interact with NPC movement speed

---

## Solution: `MovementSpeed` GAS Attribute

Add a new `MovementSpeed` attribute to `UOnsetAttributeSet`. Wire its `PostGameplayEffectExecute` to sync with `MaxWalkSpeed`. All speed changes flow through GAS gameplay effects — they stack naturally by modifier operation (Add, Multiply, Override).

### Stacking Diagram

```
   Base MovementSpeed (300)
        │
        ├── GE_InvestigationSpeed (×0.5) ─────────┐
        │                                          │
        ├── GE_StaggerSlow (×0.5) ─────────────────┤
        │                                          │
        └── GE_HasteBuff (+100) ───────────────────┤
                                                   │
                              MovementSpeed = (300 × 0.5 × 0.5) + 100 = 175
                                                   │
                                                   ▼
                                         MaxWalkSpeed = 175
```

---

## Files Created

| File | Purpose |
|------|---------|
| `Content/Game/Combat/GE_SpeedModifier.uasset` | Reusable infinite GE with tagged application |

## Files Modified

| File | Change |
|------|--------|
| `OnsetAttributeSet.h` | Add `MovementSpeed` attribute property + getter/setter/initter + `OnRep` |
| `OnsetAttributeSet.cpp` | Wire `PostGameplayEffectExecute` → `MaxWalkSpeed`; add rep notify |
| `OnsetBaseCharacter.h/.cpp` | Init `MovementSpeed` from `MaxWalkSpeed` in `BeginPlay`/`PossessedBy` |
| `OnsetStateTreeFleeTask.h/.cpp` | Remove `CachedOriginalWalkSpeed`, apply/remove `GE_SpeedModifier` by tag |
| `OnsetStateTreeInvestigateTask.h/.cpp` | Same refactor as FleeTask |
| `OnsetGameplayTags.h/.cpp` | Add `TAG_State_InvestigationSpeed`, `TAG_State_FleeSpeed`, `TAG_State_StaggerSlow` |

---

## Step-by-Step

### Step 1 — Add `MovementSpeed` Attribute

**`OnsetAttributeSet.h`** — add to the attribute block:
```cpp
UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MovementSpeed, Category = "Movement")
FGameplayAttributeData MovementSpeed;
GAMEPLAYATTRIBUTE_VALUE_GETTER(MovementSpeed);
GAMEPLAYATTRIBUTE_VALUE_SETTER(MovementSpeed);
GAMEPLAYATTRIBUTE_VALUE_INITTER(MovementSpeed);
```

Add `OnRep_MovementSpeed` and `GetMovementSpeedAttribute()` static.

**`OnsetAttributeSet.cpp`** — add replication + rep notify:
```cpp
DOREPLIFETIME_CONDITION_NOTIFY(UOnsetAttributeSet, MovementSpeed, COND_None, REPNOTIFY_Always);

void UOnsetAttributeSet::OnRep_MovementSpeed(const FGameplayAttributeData& OldMovementSpeed)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UOnsetAttributeSet, MovementSpeed, OldMovementSpeed);
}
```

### Step 2 — Wire to MaxWalkSpeed

In `PostGameplayEffectExecute`, add a new block:
```cpp
if (Data.EvaluatedData.Attribute == GetMovementSpeedAttribute())
{
    float Clamped = FMath::Max(GetMovementSpeed(), 0.0f);
    SetMovementSpeed(Clamped);
    if (AOnsetBaseCharacter* Character = Cast<AOnsetBaseCharacter>(GetOwningActor()))
    {
        Character->GetCharacterMovement()->MaxWalkSpeed = Clamped;
    }
}
```

### Step 3 — Init Base Value

In `AOnsetBaseCharacter::PossessedBy` (or `BeginPlay`), after `InitAbilityActorInfo`:
```cpp
if (AbilitySystemComponent && AttributeSet)
{
    float DefaultSpeed = GetCharacterMovement()->MaxWalkSpeed;
    AbilitySystemComponent->SetNumericAttributeBase(
        UOnsetAttributeSet::GetMovementSpeedAttribute(), DefaultSpeed);
}
```

### Step 4 — Create GE_SpeedModifier

BP asset at `/Game/Game/Combat/GE_SpeedModifier`:
- **Duration Policy:** Infinite
- **Modifier:** `MovementSpeed` × `Scalar` (or Override, per use case)
- **Tags:** Add `TAG_State_SpeedModifier` (or per-system tag)

### Step 5 — Add System Tags

```cpp
UE_DEFINE_GAMEPLAY_TAG(TAG_State_InvestigationSpeed, "State.InvestigationSpeed");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_FleeSpeed, "State.FleeSpeed");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_StaggerSlow, "State.StaggerSlow");
```

### Step 6 — Refactor FleeTask

**Before (direct):**
```cpp
CachedOriginalWalkSpeed = Self->GetCharacterMovement()->MaxWalkSpeed;
Self->GetCharacterMovement()->MaxWalkSpeed = CachedOriginalWalkSpeed * SpeedMod;
// ExitState:
Self->GetCharacterMovement()->MaxWalkSpeed = CachedOriginalWalkSpeed;
```

**After (GAS):**
```cpp
UGameplayEffect* SpeedGE = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("DynamicSpeedGE"));
SpeedGE->DurationPolicy = EGameplayEffectDurationType::Infinite;
// Add modifier: MovementSpeed × SpeedMod
// Tag: TAG_State_FleeSpeed
FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectToSelf(SpeedGE, 1.0f, ASC->MakeEffectContext());
// In instance data, store the handle
// ExitState:
ASC->RemoveActiveGameplayEffect(InstanceData.SpeedEffectHandle);
```

Or cleaner: pre-create reusable `GE_SpeedModifier` BPs, apply by class with `SetByCaller` magnitude.

### Step 7 — Refactor InvestiageTask

Identical pattern to FleeTask but applies `GE_SpeedModifier` with `TAG_State_InvestigationSpeed`.

### Step 8 — Verify Stacking

In PIE:
1. Stagger an NPC (applies ×0.5)
2. While staggered, NPC flees (applies ×0.35)
3. Result: speed = base × 0.5 × 0.35 (stacked multiplicatively)
4. Stagger expires → speed = base × 0.35 (only flee remains)
5. NPC stops fleeing → speed returns to base

---

## Key Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| GE duration | Infinite with tag removal | Clean activate/deactivate cycle, no duration tracking |
| Modifier op | Multiply for speed scaling | Natural for percentages; add for flat buffs (haste) |
| Per-system tagged GEs | One GE class + per-system source tag | Single BP asset, clean removal by tag |
| Handle tracking | `FActiveGameplayEffectHandle` per task | Precise removal, no tag collisions |
| Replication | COND_None + OnRep | Server-authoritative, client visually syncs |

