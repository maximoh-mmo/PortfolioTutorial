# 🎬 Post 4 — NPC Infrastructure in a Day: Spawner → Pool → Groups → Profiles

*16 commits · May 29, 2026 · covering sprint A2 — NPC character, object pooling, group system, and the fallback cube mesh saga*

---

## Introduction

Post 3 ended with exactly one enemy in the world, placed by hand in the editor. That's a fine state for testing targeting, but it's not a game. The next sprint — sixteen commits, all in a single marathon day — builds the entire NPC pipeline that the rest of the project will stand on: an enemy pawn, a spawner, an object pool, a group system, and the data-driven profiles that let those pooled enemies look and behave differently.

There's a real subplot buried in those sixteen commits. A "fallback cube mesh" — a stopgap for enemies with no artist assets — gets added, then **reverted three minutes later**, then rebuilt differently, then given collision, then reveals a targeting bug. It's the most honest sequence in the whole repository: a working developer making and reversing a decision in real time. This post walks the whole arc, including that saga, because the mistakes are the part worth reading.

---

## Technical Exposition

### Infrastructure before intelligence

The ordering decision here is deliberate and stated in the commits themselves: **the scaffolding must exist before AI work can be pure behavior.** You can't teach an enemy to roam or fight if there's no enemy, no way to spawn one, and no way to reuse it. So A2 builds, in dependency order:

1. **`AOnsetEnemy`** — the first actual enemy pawn (thin subclass of the player base character, tagged `"Enemy"`).
2. **`AOnsetSpawner`** — spawns groups of enemies.
3. **`AOnsetPoolManager`** — pre-allocates enemies and recycles them, so mid-game spawn/destroy churn doesn't tank performance.
4. **Group system** — tracks group membership and aggregate data.
5. **`UAIProfile` data assets** — so all the variation (mesh, stats, behavior) lives in data, not new classes.

The key architectural bet: **all enemy variation lives in `UAIProfile` data assets, not subclasses.** That single decision makes the pool trivially simple — it can hardcode one class (`AOnsetEnemy::StaticClass()`) and still deliver diverse enemies.

---

## Implementation Details

### 1. The spawner: slot-based, pool-backed

The first spawner scattered enemies in a ring around spawn points; by the end of the day it's slot-based and *requires* a pool. A slot holds a transform and an occupant; the pool supplies the actual instance:

```cpp
AOnsetEnemy* AOnsetSpawner::SpawnEnemyAtSlot(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex)) return nullptr;
	FSpawnerSlot& Slot = Slots[SlotIndex];
	if (Slot.Occupant && IsValid(Slot.Occupant)) return nullptr; // already occupied

	AOnsetEnemy* Spawned = nullptr;
	if (PoolManager)
	{
		Spawned = PoolManager->GetPooledEnemy();
		if (Spawned) Spawned->SetActorTransform(Slot.SpawnTransform);
	}
	else
	{
		UE_LOG(LogSpawner, Warning, TEXT("SpawnEnemyAtSlot: PoolManager is null — cannot spawn NPC."));
	}
	if (Spawned)
	{
		Spawned->ApplyProfile(Config.EnemyProfile);
		Slot.Occupant = Spawned;
		if (GroupManager) GroupManager->RegisterMember(Spawned);
	}
	return Spawned;
}
```

Note the failure behavior: if there's no pool, it logs a warning and *leaves the slot empty* rather than falling back to `SpawnActor`. Removing the fallback was a deliberate tradeoff — the pool is the only path, so its invariants are never bypassed.

### 2. The pool: hide, disable, recycle

The recycle contract is aggressive. A returned enemy is hidden, moved to the origin, ticks disabled, input disabled, collision disabled:

```cpp
void AOnsetPoolManager::ReturnToPool(AOnsetEnemy* Enemy)
{
	if (!Enemy) return;
	if (!ObjectPool.Contains(Enemy)) ObjectPool.Add(Enemy);
	Enemy->SetActorLocation(FVector::ZeroVector);
	Enemy->SetActorHiddenInGame(true);
	Enemy->SetActorTickEnabled(false);
	Enemy->DisableInput(nullptr);
	Enemy->SetActorEnableCollision(false);
}
```

And activation reverses it. The hidden/visible toggle is what later pool users (group data, corpse system) key off — which is exactly where the day's polarity bug bites.

### 3. The group system: components, not a manager actor

Group tracking deliberately avoids a placed `AGroupManager` actor — the commit documents the choice: *"avoids a placed-actor dependency in every level."* Instead, `UGroupComponent` (per NPC) and `UGroupManagerComponent` (on the spawner) track membership, and `GetGroupData()` is computed on demand rather than cached every tick.

The on-demand computation has a trap, found at the very end of the day. Group data was computed over NPCs that were **not hidden** — which, in a pool, is the *wrong* set:

```cpp
for (const AOnsetEnemy* Enemy : Members)
{
	if (Enemy && !Enemy->IsPendingKillPending() && !Enemy->IsHidden())
	{
		AccumulatedLocation += Enemy->GetActorLocation();
		ValidCount++;
	}
}
```

The bug is the polarity: `!IsHidden()` counts *active* NPCs, but the original code filtered on `IsHidden()` — counting **pooled, hidden** NPCs instead. Group center and alive-count were computed from exactly the wrong set. One-commit fix, but it's the kind of bug that's invisible in a small demo and catastrophic with a full spawner. (The sibling function `GetNearbyAllies` kept the same bug for another day.)

### 4. `ApplyProfile`: the enemy becomes data-driven

The `UAIProfile` data asset carries mesh, animation blueprint, material, and behavior config. `AOnsetEnemy::ApplyProfile()` turns those into a living enemy:

```cpp
void AOnsetEnemy::ApplyProfile(UAIProfile* InProfile)
{
	Profile = InProfile;
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp) return;

	if (Profile)
	{
		if (!Profile->SkeletalMesh.IsNull())
		{
			USkeletalMesh* Mesh = Profile->SkeletalMesh.LoadSynchronous();
			if (Mesh) MeshComp->SetSkeletalMesh(Mesh);
		}
		if (Profile->AnimBlueprintClass)
		{
			MeshComp->SetAnimInstanceClass(Profile->AnimBlueprintClass);
		}
		if (Profile->OverrideMaterial)
		{
			MeshComp->SetMaterial(0, Profile->OverrideMaterial);
		}
		else
		{
			MeshComp->SetMaterial(0, nullptr);
		}
	}
	else
	{
		MeshComp->SetSkeletalMesh(nullptr);
		MeshComp->SetAnimInstanceClass(nullptr);
		MeshComp->SetMaterial(0, nullptr);
	}
}
```

The crucial contract: **`ApplyProfile(nullptr)` is the reset.** The pool calls it on return; the spawner calls it with the real profile on retrieval. One function serves as both configure and wipe — elegant, and the defensive comment added in `fc2d352` makes the intent explicit.

### 5. The fallback cube saga

This is the arc's heart. When a profile has no skeletal mesh, the enemy is **invisible** — fine for a finished game, confusing during development. Three attempts:

1. **`59231da`: a permanent `FallbackMeshComp` subobject** on every enemy. Reverted **3 minutes later** (`a199ad9`). Why? Every pooled enemy would permanently carry an unused `UStaticMeshComponent` — memory and render cost on a pool that recycles one class for many variants. A permanent fallback is the wrong shape for pooling.

2. **`4bc06c2`: create/destroy on the fly.** A `NewObject<UStaticMeshComponent>("CubeVis")` only exists while needed:

```cpp
UStaticMeshComponent* CubeVis = NewObject<UStaticMeshComponent>(this, TEXT("CubeVis"));
CubeVis->SetupAttachment(RootComponent);
CubeVis->RegisterComponent();
CubeVis->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
CubeVis->SetCollisionObjectType(ECC_WorldDynamic);
CubeVis->SetCollisionResponseToAllChannels(ECR_Block);
if (UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
	CubeVis->SetStaticMesh(CubeMesh);
CubeVis->SetHiddenInGame(false);
```

And it's destroyed at the *top* of every `ApplyProfile` call — which means pool-return reset doubles as cube cleanup. The component only exists while genuinely needed.

3. **`4b059ce`: the collision reveal.** The cube's default `NoCollision` meant the **`ECC_Visibility` targeting trace passed straight through** — cube-NPCs were untargetable. Fix: real collision responses, plus auto-sizing the capsule to the mesh bounds (mesh-based NPCs often have no physics asset, so the capsule is the only traceable surface):

```cpp
if (USkeletalMesh* SkeletalMesh = Profile->SkeletalMesh.LoadSynchronous())
{
	SkeletalComp->SetSkeletalMesh(SkeletalMesh);
	FBoxSphereBounds Bounds = SkeletalMesh->GetImportedBounds();
	float Radius = FMath::Max(Bounds.BoxExtent.X, Bounds.BoxExtent.Y);
	float HalfHeight = FMath::Max(Radius, Bounds.BoxExtent.Z);
	GetCapsuleComponent()->SetCapsuleSize(Radius, HalfHeight);
}
```

The sequence is a textbook lesson: **the first fix addressed the symptom (invisibility), the second addressed the shape (pool compatibility), and only the third uncovered the real cost (collision/targeting).** Each revision was correct at the moment it was made.

---

## Results & Validation

- Enemy pawn, spawner, pool, and group system all shipping in one day.
- Single-class pool (`AOnsetEnemy`) with data-driven variation — the bet that enables everything after.
- `ApplyProfile` configure/reset contract, later reused by respawn and corpse systems.
- Group data computed on demand, with the `IsHidden()` polarity bug caught and fixed same-day.
- Docs and TODOs swept to match reality (with one honest progress correction: 100% walked back to 92% when unverified items were caught).

---

## Challenges & Solutions

### Invisible enemies, three ways

Covered above — the fallback cube saga. The lesson generalized: **in a pooled, data-driven system, "no data" must still produce a visible, targetable result.** The `ApplyProfile(nullptr)` contract and on-the-fly cube are the durable answers.

### The progress-reporting trap

**Problem:** The midday commit declared "A2 100% (31/31), total 69/69" — before several items (group-center verification, `NotifyMemberAttacked` wiring) were actually done.

**Solution:** A later commit honestly walked it back to 92% with the unchecked items listed. This repo treats progress tracking as a discipline, not a scoreboard — and catching an overclaim in the same day it was made is the reason the checklists stay trustworthy.

### Compile error in UE 5.8

**Problem:** `ApplyProfile(const UAIProfile*)` failed to compile — `TObjectPtr<UAIProfile>` has no implicit conversion from `const UAIProfile*`.

**Solution:** Dropped the `const`. A reminder that UE's pointer types don't always compose with const-correctness the way C++ literati expect.

---

## Reflection & Lessons Learned

The single-class pool is the arc's best decision. By committing to "variation lives in data, not classes," the pool, spawner, and group system all stay trivial, and every later system inherits that simplicity. The reverse — a class per enemy type — would have poisoned the whole architecture.

The `ApplyProfile(nullptr)` reset pattern is the second durable idea: one function that's both a setter and a wipe, keyed on null. It reappears everywhere later (pool return, respawn, corpse cleanup) because it gives recycling a single choke point for "make this instance pristine."

The day's mistakes all came from the same root: **moving fast against invisible failure modes.** The cube looked invisible (fine) but was untargetable (not fine); the group data used the wrong polarity (nobody could see the group center); the progress report claimed completion prematurely. All three were caught by verification, not by faith in the code.

What I'd do differently: I'd have written the "collision prerequisites" into `Targeting_System.md` *before* building the fallback cube, because the real lesson — "if it's invisible, check collision" — was discovered by accident in the editor.

---

## Forward-Looking Content

| Phase | Status |
|---|---|
| A2 — NPC character, spawner, pooling, groups | ✅ Complete |
| A3 — AI: StateTrees, perception, behavior | ⏭️ Next |
| A4 — Combat: GAS, death, corpses | Planned |

**What's next:** the infrastructure is done, so now the NPCs get brains. Post 5 begins the A3 sprint: AI Perception (sight + hearing) feeding a C++-defined StateTree schema, pool possession lifecycle fixes (the orphaned-controller leak), the Idle → Roam → Agro → Chase state machine — and the first GAS foundation, because combat is coming.

> **Next time in Post 5:** From Stubs to StateTrees — building Onset's NPC brain with UE 5.8 StateTree + AI Perception (and why we pooled the AIControllers).

---

*Arc commits: `2e5c772` → `b74e899`. Related docs: `Docs/AI/Spawner_System.md`, `Docs/AI/Pooling_System.md`, `Docs/AI/Group_System.md`, `Docs/Gameplay/Targeting_System.md`.*
