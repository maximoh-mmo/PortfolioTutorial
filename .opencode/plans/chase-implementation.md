# Chase + Base Structs — Implementation Plan

## Overview
- Create `FOnsetStateTreeTaskBase` (shared AIC/Target helpers for all tasks)
- Create `FOnsetStateTreeConditionBase` (same helpers for all conditions)
- Migrate existing 4 tasks to use the base
- Create `FOnsetStateTreeChaseTask` (single class, used by both Chase and Marooned)
- Create `FOnsetStateTreeDistanceCondition` using the condition base
- Update StateTree asset, docs, build

---

## 1. FOnsetStateTreeTaskBase — Task Shared Base

**New: `Source/Onset/Public/AI/Tasks/OnsetStateTreeTaskBase.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "OnsetStateTreeTaskBase.generated.h"

class AOnsetAIController;
class AOnsetEnemy;

USTRUCT()
struct FOnsetStateTreeTaskBase : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	static AOnsetAIController* GetAIC(FStateTreeExecutionContext& Context);
	static AOnsetEnemy* GetEnemyPawn(FStateTreeExecutionContext& Context);
	static AActor* GetTarget(FStateTreeExecutionContext& Context);
};
```

**New: `Source/Onset/Private/AI/Tasks/OnsetStateTreeTaskBase.cpp`**

```cpp
#include "AI/Tasks/OnsetStateTreeTaskBase.h"

#include "StateTreeExecutionContext.h"
#include "AI/OnsetAIController.h"
#include "Enemy/OnsetEnemy.h"
#include "Player/TargetingComponent.h"

AOnsetAIController* FOnsetStateTreeTaskBase::GetAIC(FStateTreeExecutionContext& Context)
{
	AOnsetAIController* AIC = Cast<AOnsetAIController>(Context.GetOwner());
	if (!AIC) UE_LOG(LogTemp, Warning, TEXT("GetAIC: Context owner is not AOnsetAIController"));
	return AIC;
}

AOnsetEnemy* FOnsetStateTreeTaskBase::GetEnemyPawn(FStateTreeExecutionContext& Context)
{
	AOnsetAIController* AIC = GetAIC(Context);
	if (!AIC) return nullptr;
	AOnsetEnemy* Enemy = Cast<AOnsetEnemy>(AIC->GetPawn());
	if (!Enemy) UE_LOG(LogTemp, Warning, TEXT("GetEnemyPawn: No AOnsetEnemy possessed by %s"), *AIC->GetName());
	return Enemy;
}

AActor* FOnsetStateTreeTaskBase::GetTarget(FStateTreeExecutionContext& Context)
{
	AOnsetAIController* AIC = GetAIC(Context);
	if (!AIC) return nullptr;
	if (!AIC->TargetingComponent) { UE_LOG(LogTemp, Warning, TEXT("GetTarget: TargetingComponent null on %s"), *AIC->GetName()); return nullptr; }
	return AIC->TargetingComponent->GetTarget();
}
```

## 2. Migrate Existing Tasks to FOnsetStateTreeTaskBase

### Each header: change base class
```
: public FStateTreeTaskCommonBase  →  : public FOnsetStateTreeTaskBase
```
Affected: `FOnsetStateTreeAgroTask`, `FOnsetStateTreeIdleTask`, `FOnsetStateTreeLostTargetTask`, `FOnsetStateTreeRoamTask`

Also add `#include "AI/Tasks/OnsetStateTreeTaskBase.h"` to each header.

### Each cpp: replace Cast with helper calls

| File | Old | New |
|------|-----|-----|
| **AgroTask.cpp** EnterState | `Cast<...>(Context.GetOwner())` | `GetAIC(Context)` |
| | `AIController->TargetingComponent->GetTarget()` | `GetTarget(Context)` |
| | include `OnsetAIController.h`, `TargetingComponent.h` | (via base cpp) |
| **AgroTask.cpp** Tick | Same replacements | |
| **LostTargetTask.cpp** | `Cast<...>(Context.GetOwner())` | `GetAIC(Context)` |
| | include `OnsetAIController.h` | (via base cpp) |
| **RoamTask.cpp** EnterState | `Cast<...>(Context.GetOwner())` | `GetAIC(Context)` |
| | `Cast<AOnsetEnemy>(AIC->GetPawn())` | `GetEnemyPawn(Context)` |
| **RoamTask.cpp** Tick | `Cast<...>(Context.GetOwner())` | `GetAIC(Context)` |
| **IdleTask.cpp** | No changes (doesn't use AIC) | |

## 3. FOnsetStateTreeConditionBase — Condition Shared Base

**New: `Source/Onset/Public/AI/Conditions/OnsetStateTreeConditionBase.h`**

```cpp
#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "OnsetStateTreeConditionBase.generated.h"

class AOnsetAIController;
class AOnsetEnemy;

USTRUCT()
struct FOnsetStateTreeConditionBase : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	static AOnsetAIController* GetAIC(FStateTreeExecutionContext& Context);
	static AOnsetEnemy* GetEnemyPawn(FStateTreeExecutionContext& Context);
	static AActor* GetTarget(FStateTreeExecutionContext& Context);
};
```

**New: `Source/Onset/Private/AI/Conditions/OnsetStateTreeConditionBase.cpp`**

Same implementation as the task base (duplicated code, zero coupling). All three helpers take `FStateTreeExecutionContext&` — `GetAIC` does the Cast, `GetEnemyPawn` and `GetTarget` call `GetAIC` internally.

## 4. FOnsetStateTreeDistanceCondition (now using ConditionBase)

**Update from plan v1:** Change base from `FStateTreeConditionCommonBase` to `FOnsetStateTreeConditionBase`.

**`OnsetStateTreeDistanceCondition.h`:** same as v1 but `: public FOnsetStateTreeConditionBase`

**`OnsetStateTreeDistanceCondition.cpp`:**
```cpp
bool FOnsetStateTreeDistanceCondition::TestCondition(...)
{
	AOnsetAIController* AIC = GetAIC(Context);  // was Cast<...>
	if (!AIC || !AIC->GetPawn()) return false;

	FVector SourceLocation = AIC->GetPawn()->GetActorLocation();
	// ... rest same as v1, replace:
	//   AActor* Target = AIC->TargetingComponent->GetTarget();  →  GetTarget(AIC)
	//   Cast<AOnsetEnemy>(AIC->GetPawn())  →  GetEnemyPawn(AIC)
}
```

## 5. FOnsetStateTreeChaseTask (using TaskBase)

**Same code as v1 but by way of `FOnsetStateTreeTaskBase`:**

```cpp
// OnsetStateTreeChaseTask.h
USTRUCT()
struct FOnsetStateTreeChaseTask : public FOnsetStateTreeTaskBase  // was FStateTreeTaskCommonBase
{
	// ...
};

// OnsetStateTreeChaseTask.cpp
EStateTreeRunStatus FOnsetStateTreeChaseTask::EnterState(...)
{
	AOnsetAIController* AIC = GetAIC(Context);
	if (!AIC) return Failed;

	AActor* Target = GetTarget(Context);
	if (!Target) return Succeeded;

	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AIC->MoveToActor(Target, InstanceData.AcceptanceRadius);
	return Running;
}
```

## 6. File Delta Summary

| Action | Files |
|--------|-------|
| **New** | `Tasks/OnsetStateTreeTaskBase.h` + `.cpp` |
| **New** | `Conditions/OnsetStateTreeConditionBase.h` + `.cpp` |
| **New** | `Tasks/OnsetStateTreeChaseTask.h` + `.cpp` |
| **New** | `Conditions/OnsetStateTreeDistanceCondition.h` + `.cpp` |
| **Modify** | `Tasks/OnsetStateTreeAgroTask.h/.cpp` — change base, use helpers, strip redundant includes |
| **Modify** | `Tasks/OnsetStateTreeLostTargetTask.h/.cpp` — same |
| **Modify** | `Tasks/OnsetStateTreeRoamTask.h/.cpp` — same |
| **Modify** | `Tasks/OnsetStateTreeIdleTask.h` — change base only |
| **Modify** | `Docs/AI/NPC_AI_System.md` — add Chase task + base sections |
| **Modify** | `TODO/Private_Demo_Checklist.md` — mark Chase, add Marooned |
| **Modify** | `TODO/06-12-26.md` |
| **Edit** | `ST_NPC_Base.uasset` — add Chase, Marooned states, condition transitions |

## 7. Build & Asset Wire

1. Compile with `Build.bat OnsetEditor Win64 Development`
2. If compile succeeds, open `ST_NPC_Base.uasset`
3. Add Chase state → `FOnsetStateTreeChaseTask`
4. Add Marooned state → same task
5. Add 3× `FOnsetStateTreeDistanceCondition` (AttackRange, LostRange, LeashRadius)
6. Wire transitions as per table below
7. Save asset

**Transitions from Chase (evaluated in order):**

| To | Condition | Source | Comparison | Threshold | AllowNoTarget |
|----|-----------|--------|------------|-----------|---------------|
| Attack | Distance ≤ 150 | CurrentTarget | LessOrEqual | 150 | false |
| Lost | Distance > 2000 \|\| no target | CurrentTarget | Greater | 2000 | true |
| Marooned | HomeDistance > 1500 | HomeLocation | Greater | 1500 | false |

**Transitions from Marooned:**

| To | Condition | Source | Comparison | Threshold | AllowNoTarget |
|----|-----------|--------|------------|-----------|---------------|
| Attack | Distance ≤ 150 | CurrentTarget | LessOrEqual | 150 | false |
| Lost | Distance > 2000 \|\| no target | CurrentTarget | Greater | 2000 | true |

## 8. Notes
- `FStateTreeConditionCommonBase` — if missing in UE5.8, try `FStateTreeConditionBase` directly and add `GetConditionInstanceDataType()` returning nullptr.
- `EStateTreeComparison` lives in `StateTreeTypes.h`, pulled by `StateTreeExecutionContext.h`.
- `EOnsetStateTreeDistanceSource` enum in the condition header — new type for asset binding.
- Task/condition .cpp files no longer need individual includes for `OnsetAIController.h`, `TargetingComponent.h`, `OnsetEnemy.h` — the base .cpp handles them.
