# 🎬 **Episode 5 — Enemy Spawner (C++)**

## **Episode Goal**
Create a spawner that generates groups of NPCs at defined points.

---

## **Context & Dependencies**
- Requires [Episode 4](Episode04_ClickToTarget.md) (targeting system — gives us a reason to spawn enemies)
- Creates the NPC character class and places enemies in the world for the first time

---

## **High‑Level Summary**
We build the Enemy Spawner — an actor placed in the level that spawns groups of NPCs. This is the first piece of the NPC lifecycle pipeline. The spawner manages spawn points, group configurations, and triggers.

---

## **Key Concepts Introduced**
- [Spawner actor](../../Docs/AI/Spawner_System.md) (AEnemySpawner)
- Spawn configuration struct
- UWorld::SpawnActor
- Group spawning (multiple NPCs at once)
- Spawn point specification
- Editor utility: placeable spawner actor

---

## **Technical Breakdown**

### **1. Create Spawn Config Struct**
```cpp
USTRUCT(BlueprintType)
struct FSpawnConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    TSubclassOf<ACharacter> EnemyClass;

    UPROPERTY(EditAnywhere)
    int32 GroupSize = 3;

    UPROPERTY(EditAnywhere)
    float SpawnRadius = 200.0f;

    UPROPERTY(EditAnywhere)
    float RespawnDelay = 30.0f;
};
```

### **2. Create Enemy Spawner Actor**
```cpp
UCLASS()
class AEnemySpawner : public AActor
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void SpawnGroup();

    UPROPERTY(EditAnywhere)
    FSpawnConfig SpawnConfig;

    UPROPERTY(EditAnywhere)
    TArray<AActor*> SpawnPoints;
};
```

### **3. Spawn Logic**
```cpp
void AEnemySpawner::SpawnGroup()
{
    for (int32 i = 0; i < SpawnConfig.GroupSize; i++)
    {
        FVector SpawnLocation = GetSpawnLocation(i);
        FRotator SpawnRotation = FRotator::ZeroRotator;

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        GetWorld()->SpawnActor<ACharacter>(SpawnConfig.EnemyClass, SpawnLocation, SpawnRotation, Params);
    }
}

FVector AEnemySpawner::GetSpawnLocation(int32 Index)
{
    if (SpawnPoints.IsValidIndex(Index))
    {
        return SpawnPoints[Index]->GetActorLocation();
    }

    // Fallback: scatter around spawner
    FVector Center = GetActorLocation();
    float Angle = (360.0f / SpawnConfig.GroupSize) * Index;
    float Rad = FMath::DegreesToRadians(Angle);
    return Center + FVector(FMath::Cos(Rad), FMath::Sin(Rad), 0.0f) * SpawnConfig.SpawnRadius;
}
```

### **4. Level Setup**
- Place the spawner actor in the level
- Define spawn points (child actors or manually placed)
- Set enemy class and group size

### **5. Testing**
- Press Play — spawner creates NPC group at start (or on trigger)
- Verify correct number of NPCs at correct locations
- Spawner can be re-triggered via blueprint or timer

---

## **Code Snippets**

```cpp
// AEnemySpawner.h
UCLASS()
class ONSET_API AEnemySpawner : public AActor
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = Spawning)
    void SpawnGroup();

    UFUNCTION(BlueprintCallable, Category = Spawning)
    void DestroyGroup();

protected:
    UPROPERTY(EditAnywhere, Category = Spawning)
    FSpawnConfig SpawnConfig;

    UPROPERTY(EditAnywhere, Category = Spawning)
    TArray<AActor*> SpawnPoints;

    UPROPERTY()
    TArray<AActor*> SpawnedGroup;

    FVector GetSpawnLocation(int32 Index) const;
};
```

---

## **Diagrams**
```
[Level Placement]
    │
AEnemySpawner (placed in world)
    │
    ├── SpawnConfig: EnemyClass, GroupSize, Radius
    ├── SpawnPoints: [Point1, Point2, ...]
    │
    ▼
SpawnGroup()
    │
    ├── Spawn NPC 1 at Point 1
    ├── Spawn NPC 2 at Point 2
    └── Spawn NPC 3 at Point 3
```

---

## **Common Pitfalls**
- Spawning NPCs inside each other (use collision adjust or spread)
- Not setting EnemyClass in the config → crash
- Forgetting to handle `bStartPlay` or world initialization timing
- Spawning too many NPCs in a single frame (performance)

---

## **Episode Checklist**
- [ ] Spawner actor compiles and is placeable
- [ ] SpawnGroup() creates NPCs at correct locations
- [ ] Config group size is respected
- [ ] Spawn points override fallback scatter
- [ ] Works in PIE

---

## **Public Repo Notes**
- Include a simple NPC stub in the map
- Spawner should auto-trigger on BeginPlay for testing

---

## **Recording Script**

**Intro:**
"Welcome back. We can move and target — now we need real enemies to fight. Today we build the enemy spawner."

**Body:**
- Create the SpawnConfig struct
- Create the AEnemySpawner actor
- Implement SpawnGroup() with point + fallback scattering
- Place in level and test
- Discuss respawn (stub for future episode)

**Outro:**
"Coming up in Phase 1, we add object pooling so spawning stays performant."

---

## **Next Episode Preview**
"Next phase, we implement object pooling for efficient NPC reuse."
