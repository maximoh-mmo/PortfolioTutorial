# 🎬 **Episode 5 — Enemy Spawner (C++)**

## **Episode Goal**
Create a slot‑based spawner that generates groups of NPCs at predefined points, using data‑driven AI profiles to configure each enemy on spawn.

---

## **Context & Dependencies**
- Requires [Episode 4](Episode04_ClickToTarget.md) (targeting system — gives us a reason to spawn enemies)
- Creates the enemy character class and places enemies in the world for the first time
- Introduces `UAIProfile` data assets for per‑enemy‑type configuration

---

## **High‑Level Summary**
We build the Enemy Spawner — an actor placed in the level that manages a set of **slots**, each representing a potential spawn location. On `BeginPlay`, slots are initialised from spawn points (or a fallback ring scatter). When `SpawnGroup()` is called, every empty slot receives a new NPC, configured via a `UAIProfile` data asset that determines class, state tree, perception, and combat parameters.

---

## **Key Concepts Introduced**
- [Spawner actor](../../Docs/AI/Spawner_System.md) (AOnsetSpawner)
- Slot‑based spawn management (`FSpawnerSlot`)
- `UAIProfile` data assets for data‑driven enemy configuration
- [`UGroupManagerComponent`](../../Docs/AI/Group_System.md) for group registration
- `UWorld::SpawnActor` with profile‑driven parameters

---

## **Technical Breakdown**

### **1. Create AI Profile Data Asset**
```cpp
UCLASS(BlueprintType, DefaultToInstanced, EditInlineNew)
class ONSET_API UAIProfile : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "StateTree")
    TSoftObjectPtr<UStateTree> StateTreeAsset;

    UPROPERTY(EditAnywhere, Category = "Perception")
    float SightRadius = 2000.0f;

    UPROPERTY(EditAnywhere, Category = "Perception")
    float SightAngle = 90.0f;

    UPROPERTY(EditAnywhere, Category = "Perception")
    float HearingRange = 1500.0f;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float Aggression = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Combat")
    float FleeHealthThreshold = 0.2f;

    UPROPERTY(EditAnywhere, Category = "Group")
    float AssistRadius = 500.0f;
};
```

### **2. Create Spawn Config Struct (Profile‑Based)**
```cpp
USTRUCT(BlueprintType)
struct FSpawnConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    TObjectPtr<UAIProfile> EnemyProfile;

    UPROPERTY(EditAnywhere)
    int32 GroupSize = 3;

    UPROPERTY(EditAnywhere)
    float SpawnRadius = 200.0f;

    UPROPERTY(EditAnywhere)
    float RespawnDelay = 30.0f;
};
```

### **3. Create Spawner Slot Struct**
```cpp
USTRUCT()
struct FSpawnerSlot
{
    GENERATED_BODY()

    FTransform SpawnTransform;
    TObjectPtr<AActor> Occupant;
};
```

### **4. Create Enemy Spawner Actor**
```cpp
UCLASS()
class AOnsetSpawner : public AActor
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable)
    void SpawnGroup();

    UFUNCTION(BlueprintCallable)
    void DestroyGroup();

    UFUNCTION(BlueprintCallable)
    void DebugKillLast();

protected:
    void InitSlots();
    void SpawnEnemyAtSlot(int32 SlotIndex);

    UPROPERTY(EditAnywhere)
    FSpawnConfig SpawnConfig;

    UPROPERTY(EditAnywhere)
    TArray<AActor*> SpawnPoints;

    UPROPERTY()
    TArray<FSpawnerSlot> Slots;
};
```

### **5. Slot Initialisation**
```cpp
void AOnsetSpawner::InitSlots()
{
    Slots.Empty();
    int32 Count = FMath::Max(1, SpawnConfig.GroupSize);

    for (int32 i = 0; i < Count; i++)
    {
        FSpawnerSlot& Slot = Slots.AddDefaulted_GetRef();
        Slot.Occupant = nullptr;

        if (SpawnPoints.IsValidIndex(i))
        {
            Slot.SpawnTransform = SpawnPoints[i]->GetActorTransform();
        }
        else
        {
            // Fallback: ring scatter
            float Angle = (360.0f / Count) * i;
            float Rad = FMath::DegreesToRadians(Angle);
            FVector Offset = FVector(FMath::Cos(Rad), FMath::Sin(Rad), 0.0f) * SpawnConfig.SpawnRadius;
            Slot.SpawnTransform = FTransform(FRotator::ZeroRotator, GetActorLocation() + Offset);
        }
    }
}
```

### **6. Spawn Logic (Slot‑Based)**
```cpp
void AOnsetSpawner::SpawnGroup()
{
    if (!HasAuthority()) return;

    for (int32 i = 0; i < Slots.Num(); i++)
    {
        if (Slots[i].Occupant == nullptr)
        {
            SpawnEnemyAtSlot(i);
        }
    }
}

void AOnsetSpawner::SpawnEnemyAtSlot(int32 SlotIndex)
{
    UAIProfile* Profile = SpawnConfig.EnemyProfile;
    if (!Profile) return;

    UClass* EnemyClass = Profile->GetEnemyPawnClass(); // provided by BP child of UAIProfile
    if (!EnemyClass) return;

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AOnsetEnemy* Enemy = GetWorld()->SpawnActor<AOnsetEnemy>(EnemyClass, Slots[SlotIndex].SpawnTransform, Params);
    if (Enemy)
    {
        Enemy->Profile = Profile;
        Slots[SlotIndex].Occupant = Enemy;
        GroupManager->RegisterMember(Enemy); // register with group system
    }
}
```

### **7. Destroy & Debug**
```cpp
void AOnsetSpawner::DestroyGroup()
{
    for (FSpawnerSlot& Slot : Slots)
    {
        if (Slot.Occupant)
        {
            Slot.Occupant->Destroy();
            Slot.Occupant = nullptr;
        }
    }
}

void AOnsetSpawner::DebugKillLast()
{
    for (int32 i = Slots.Num() - 1; i >= 0; i--)
    {
        if (Slots[i].Occupant)
        {
            Slots[i].Occupant->Destroy();
            Slots[i].Occupant = nullptr;
            return;
        }
    }
}
```

### **8. Level Setup**
- Place the spawner actor in the level
- Define spawn points (child actors or manually placed)
- Create a `UAIProfile` data asset (or use a BP child that provides `GetEnemyPawnClass()`)
- Set profile and group size in `FSpawnConfig`

### **9. Testing**
- Press Play — spawner initialises slots and spawns NPC group on `BeginPlay`
- Verify correct number of NPCs at correct locations
- Use `DebugKillLast()` and observe slot‑by‑slot respawn on next `SpawnGroup()` call
- Set `NetMode` to simulate server‑only execution

---

## **Code Snippets**

```cpp
// AOnsetSpawner.h
UCLASS()
class ONSET_API AOnsetSpawner : public AActor
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = Spawning)
    void SpawnGroup();

    UFUNCTION(BlueprintCallable, Category = Spawning)
    void DestroyGroup();

    UFUNCTION(BlueprintCallable, Category = Spawning)
    void DebugKillLast();

protected:
    void InitSlots();
    void SpawnEnemyAtSlot(int32 SlotIndex);

    UPROPERTY(EditAnywhere, Category = Spawning)
    FSpawnConfig SpawnConfig;

    UPROPERTY(EditAnywhere, Category = Spawning)
    TArray<AActor*> SpawnPoints;

    UPROPERTY()
    TArray<FSpawnerSlot> Slots;
};
```

---

## **Diagrams**
```
[Level Placement]
    │
AOnsetSpawner (placed in world)
    │
    ├── SpawnConfig: EnemyProfile, GroupSize, Radius
    ├── SpawnPoints: [Point1, Point2, ...]
    │
    ▼
BeginPlay → InitSlots()
    │
    ├── Slot 0 → Point 1 (or ring scatter X°)
    ├── Slot 1 → Point 2 (or ring scatter Y°)
    └── Slot 2 → Point 3 (or ring scatter Z°)
    │
    ▼
SpawnGroup()
    │
    ├── Slot 0 empty? → SpawnEnemyAtSlot(0) from UAIProfile
    ├── Slot 1 occupied → skip
    └── Slot 2 empty? → SpawnEnemyAtSlot(2) from UAIProfile
```

---

## **Common Pitfalls**
- Spawning NPCs inside each other (use collision adjust or spread)
- Not setting `EnemyProfile` in the config → no NPCs spawned
- Forgetting `HasAuthority()` guard in multiplayer
- Calling `SpawnGroup()` before `InitSlots()` has run
- Spawning too many NPCs in a single frame (performance)

---

## **Episode Checklist**
- [ ] `UAIProfile` data asset compiles and is editable in editor
- [ ] Spawner actor compiles and is placeable
- [ ] `InitSlots()` creates correct number of slots
- [ ] Slot transforms match spawn points or fallback scatter
- [ ] `SpawnEnemyAtSlot()` creates NPC at correct slot location
- [ ] NPC's `Profile` property is set after spawn
- [ ] `DebugKillLast()` kills the correct NPC
- [ ] Works in PIE with `HasAuthority()` guard

---

## **Public Repo Notes**
- Include a simple NPC stub in the map
- Spawner should auto-trigger on BeginPlay for testing
- Create at least one `UAIProfile` BP data asset as a content example

---

## **Recording Script**

**Intro:**
"Welcome back. We can move and target — now we need real enemies to fight. Today we build the enemy spawner and the AI profile system that will make each enemy type configurable without C++ changes."

**Body:**
- Create the `UAIProfile` data asset
- Create the `FSpawnerSlot` struct
- Create the `AOnsetSpawner` actor with `InitSlots()`, `SpawnEnemyAtSlot()`, `SpawnGroup()`
- Create a spawn config that uses a profile instead of a raw class
- Place in level and test
- Demonstrate `DebugKillLast()` and slot‑based respawn

**Outro:**
"Coming up, we add the AI perception system and state trees to give these NPCs real behaviour."

---

## **Next Episode Preview**
"Next episode, we implement AI perception and state tree integration to bring our NPCs to life."
