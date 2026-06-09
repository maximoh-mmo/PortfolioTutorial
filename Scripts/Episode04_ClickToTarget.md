# 🎬 **Episode 4 — Click‑to‑Target System**

## **Episode Goal**
Implement target selection, highlighting, and basic attack input routing.

---

## **Context & Dependencies**
- Requires [Episode 3](Episode03_Movement_System.md) (movement system + cursor abstraction)
- A temporary test NPC will be added to the level for targeting; the proper spawner comes in [Episode 5](Episode05_Enemy_Spawner.md)

---

## **High‑Level Summary**
We add click-to-target: when the player taps/clicks/confirms on an enemy, that enemy becomes the current target. A highlight indicates the selection, and a basic attack can be triggered. This works across all input methods — mouse click, touch tap, and gamepad cursor confirm. This is the foundation of the combat system.

---

## **Key Concepts Introduced**
- TargetingComponent — data holder for current target with validation
- Actor channel for enemy trace
- Target highlighting (outline or material effect)
- Distinguishing between ground confirm (move) and enemy confirm (target)
- Context resolution in PlayerController (IA_Primary handler branches on hit result)
- Basic attack routing via ability input (stub)

---

## **Technical Breakdown**

### **1. Create TargetingComponent (Data Holder + Validation)**
```cpp
UCLASS(meta=(BlueprintSpawnableComponent))
class UTargetingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTargetingComponent();

    UFUNCTION(BlueprintCallable)
    AActor* GetCurrentTarget() const { return CurrentTarget; }

    UFUNCTION(BlueprintCallable)
    void SetCurrentTarget(AActor* NewTarget);

    UFUNCTION(BlueprintCallable)
    void ClearTarget();

    UFUNCTION(BlueprintCallable)
    bool IsActorValidTarget(AActor* Target) const;

private:
    AActor* CurrentTarget = nullptr;
};
```
No Tick needed — targeting is purely event-driven via IA_Primary.

### **2. Modify Primary Handler (IA_Primary) — Context Resolution**
Determine whether the primary action (tap/click/A-button) hit an enemy or the ground, using the cursor abstraction from Episode 3. This context resolution lives in the PlayerController:
```cpp
void AOnsetPlayerController::OnPrimaryInteraction()
{
    FVector2D ScreenPos;
    if (!CursorManager->GetCursorPosition(ScreenPos)) return;

    FHitResult Hit;
    GetHitResultAtScreenPosition(ScreenPos, ECC_Visibility, false, Hit);

    if (!Hit.bBlockingHit) return;

    if (Hit.GetActor() && Hit.GetActor()->ActorHasTag("Enemy"))
    {
        TargetingComponent->SetCurrentTarget(Hit.GetActor());
    }
    else
    {
        MoveToLocation(Hit.Location);
    }
}
```

### **3. Target Highlighting**
Apply a post-process outline or simple material change on the target actor. Use a decal component or custom depth rendering.

### **4. Basic Attack Input (Stub)**
Bind an ability input key (e.g., `IA_Ability1`) → triggers `OnBasicAttack()`:
```cpp
void AOnsetPlayerController::OnBasicAttack()
{
    if (TargetingComponent->GetCurrentTarget())
    {
        // Route to [GAS](../../Docs/GAS/GAS_System.md) (stub — will be filled in Episode 14+)
        // For now, print a log message
        UE_LOG(LogTemp, Log, TEXT("Attack target: %s"), *TargetingComponent->GetCurrentTarget()->GetName());
    }
}
```

### **5. Testing**
- Click enemy → target is set, highlight appears
- Click ground → character moves, target is cleared
- Press attack key → logs target name
- Target persists through movement

---

## **Code Snippets**

```cpp
// UTargetingComponent.h — data holder with validation, no Tick
UCLASS(meta=(BlueprintSpawnableComponent))
class UTargetingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTargetingComponent();

    UFUNCTION(BlueprintCallable)
    AActor* GetCurrentTarget() const { return CurrentTarget; }

    UFUNCTION(BlueprintCallable)
    void SetCurrentTarget(AActor* NewTarget);

    UFUNCTION(BlueprintCallable)
    void ClearTarget();

    UFUNCTION(BlueprintCallable)
    bool IsActorValidTarget(AActor* Target) const;

private:
    AActor* CurrentTarget = nullptr;
};
```

```cpp
// Context resolution in PlayerController (uses IA_Primary + CursorManager)
void AOnsetPlayerController::OnPrimaryInteraction()
{
    FVector2D ScreenPos;
    if (!CursorManager->GetCursorPosition(ScreenPos)) return;

    FHitResult Hit;
    GetHitResultAtScreenPosition(ScreenPos, ECC_Visibility, false, Hit);

    if (!Hit.bBlockingHit) return;

    if (Hit.GetActor() && Hit.GetActor()->ActorHasTag("Enemy"))
    {
        GetTargetingComponent()->SetCurrentTarget(Hit.GetActor());
    }
    else
    {
        GetPawn()->GetController()->MoveToLocation(Hit.Location);
    }
}
```

---

## **Diagrams**
```
IA_Primary (any device)
 │
 ▼
CursorManager::GetCursorPosition()
 │
 ▼
Raycast at Screen Position
 ├── Hit Enemy → Set as CurrentTarget → Highlight
 └── Hit Ground → MoveToLocation → ClearTarget (optional)

Ability Input (any device) → HasTarget? → Yes → Send to GAS (stub)
                                         → No → Do nothing
```

---

## **Common Pitfalls**
- Not distinguishing enemies from other actors (use tags or interface)
- Highlight persisting on dead or out-of-range targets
- Tap/click-to-move overriding target selection
- Forgetting to clear target when enemy dies
- Touch tap registering as both move and target — handled by the Confirm handler branching on enemy vs ground
- Gamepad R-Stick cursor not hitting the enemy hitbox — ensure collision channels are set on NPCs

---

## **Episode Checklist**
- [ ] Tap/click/A-button on enemy sets the target (all input methods)
- [ ] Tap/click/A-button on ground moves the character
- [ ] Target highlight visible
- [ ] Ability key routes to current target (stub logs target name)
- [ ] Gamepad R-Stick cursor over enemy → IA_Primary → target set
- [ ] Target clears on death (if applicable)

---

## **Public Repo Notes**
- Include a test enemy in the level for demonstration
- Ensure enemy actor has the "Enemy" tag

---

## **Recording Script**

**Intro:**
"Welcome back. This episode we add click-to-target — the combat equivalent of click-to-move."

**Body:**
- Create TargetingComponent
- Modify click handling to branch on enemy vs ground
- Add target highlighting
- Wire up basic attack input as a stub
- Test the full flow

**Outro:**
"Next episode, we create the enemy spawner — now that we can target, we need real enemies to fight."

---

## **Next Episode Preview**
"Next time, we build the enemy spawner with groups of NPCs that we can target and attack."
