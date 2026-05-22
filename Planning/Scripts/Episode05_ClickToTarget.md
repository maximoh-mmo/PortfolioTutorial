# 🎬 **Episode 5 — Click‑to‑Target System**

## **Episode Goal**
Implement target selection, highlighting, and basic attack input routing.

---

## **Context & Dependencies**
- Requires [Episode 4](Episode04_Enemy_Spawner.md) (enemy spawner provides NPCs in the world to target)

---

## **High‑Level Summary**
We add click-to-target: when the player taps/clicks on an enemy, that enemy becomes the current target. A highlight indicates the selection, and a basic attack can be triggered. This is the foundation of the combat system, supporting both mouse and touch input.

---

## **Key Concepts Introduced**
- TargetingComponent — stores and manages the current target
- Actor channel for enemy trace
- Target highlighting (outline or material effect)
- Distinguishing between ground tap/click (move) and enemy tap/click (target)
- Basic attack input routing

---

## **Technical Breakdown**

### **1. Create TargetingComponent**
```cpp
UCLASS()
class UTargetingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    AActor* GetCurrentTarget() const { return CurrentTarget; }
    void SetCurrentTarget(AActor* NewTarget);

private:
    AActor* CurrentTarget;
};
```

### **2. Modify Click Handler**
Determine whether the tap/click hit an enemy or the ground:
```cpp
void AOnsetPlayerController::OnClick()
{
    FHitResult Hit;
    GetHitResultUnderCursor(ECC_Visibility, false, Hit);

    if (!Hit.bBlockingHit) return;

    if (Hit.GetActor() && Hit.GetActor()->ActorHasTag("Enemy"))
    {
        // Set as target
        TargetingComponent->SetCurrentTarget(Hit.GetActor());
    }
    else
    {
        // Move to location
        MoveToLocation(Hit.Location);
    }
}
```

### **3. Target Highlighting**
Apply a post-process outline or simple material change on the target actor. Use a decal component or custom depth rendering.

### **4. Basic Attack Input**
Add `IA_BasicAttack` input action → triggers `OnBasicAttack()`:
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
// UTargetingComponent.h
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UTargetingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    AActor* GetCurrentTarget() const { return CurrentTarget; }

    UFUNCTION(BlueprintCallable)
    void SetCurrentTarget(AActor* NewTarget);

    UFUNCTION(BlueprintCallable)
    void ClearTarget();

private:
    AActor* CurrentTarget = nullptr;
};
```

```cpp
// Targeting logic in PlayerController
void AOnsetPlayerController::OnClick()
{
    FHitResult Hit;
    GetHitResultUnderCursor(ECC_Visibility, false, Hit);

    if (!Hit.bBlockingHit) return;

    if (Hit.GetActor() && Hit.GetActor()->ActorHasTag("Enemy"))
    {
        GetTargetingComponent()->SetCurrentTarget(Hit.GetActor());
    }
    else
    {
        // Move to hit location
        GetPawn()->GetController()->MoveToLocation(Hit.Location);
    }
}
```

---

## **Diagrams**
```
Click
 ├── Hit Enemy → Set as CurrentTarget → Highlight
 └── Hit Ground → MoveToLocation → ClearTarget (optional)

Attack Input → HasTarget? → Yes → Send to GAS (stub)
                         → No → Do nothing
```

---

## **Common Pitfalls**
- Not distinguishing enemies from other actors (use tags or interface)
- Highlight persisting on dead or out-of-range targets
- Tap/click-to-move overriding target selection
- Forgetting to clear target when enemy dies
- Touch tap registering as both move and target — use a brief delay or double-tap detection for mobile

---

## **Episode Checklist**
- [ ] Tap/click on enemy sets the target (mouse + touch)
- [ ] Tap/click on ground moves the character (mouse + touch)
- [ ] Target highlight visible
- [ ] Attack key routes to current target
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
"[Next episode](Episode06_Object_Pooling.md) we optimize spawner performance with object pooling."

---

## **Next Episode Preview**
"[Next time](Episode06_Object_Pooling.md), we implement object pooling for efficient NPC reuse."
