# 🎬 **Episode 3 — Point‑and‑Click Movement**

## **Episode Goal**
Use screen raycasts (mouse + touch) and MoveToLocation to implement tap/click‑to‑move.

---

## **Context & Dependencies**
- Requires [Episode 2](Episode02_TopDown_Camera.md) (top-down camera)

---

## **High‑Level Summary**
We add click-to-move by raycasting from the screen position (mouse cursor or touch tap) into the world, finding the target location, and sending the character there using Unreal's AI navigation system. This is the primary control scheme for the entire demo, supporting both mouse and touch input.

---

## **Key Concepts Introduced**
- Screen-space input (mouse cursor + touch tap)
- Raycasting from screen to world (DeprojectScreenToWorld)
- Hit result and collision channels
- AI Navigation / MoveToLocation
- Input action mappings
- Simple tap/click feedback (optional)
- Unified input handling for mouse and touch

---

## **Technical Breakdown**

### **1. Input Setup**
Enable mouse cursor on PlayerController (mouse ignored on touch devices):
```cpp
bShowMouseCursor = true;
DefaultMouseCursor = EMouseCursor::Default;
```
Touch input is handled by the same input system — UE routes touch taps through the same `GetHitResultUnderCursor` pipeline, so no separate touch handling is needed for click-to-move.

### **2. Input Mapping**
Add an `IA_ClickMove` action mapping (Left Mouse Button).

### **3. Raycast Logic**
```cpp
void AOnsetPlayerController::OnClickMove()
{
    FHitResult Hit;
    GetHitResultUnderCursor(ECC_Visibility, false, Hit);

    if (Hit.bBlockingHit)
    {
        // Move to hit location
    }
}
```

### **4. MoveToLocation**
Use the AIController's `MoveToLocation` to navigate:
```cpp
if (APawn* MyPawn = GetPawn())
{
    if (AController* AIC = Cast<AController>(MyPawn->GetController()))
    {
        AIC->MoveToLocation(Hit.Location);
    }
}
```

### **5. Testing**
- Click anywhere in the world — character moves
- Click multiple times — movement updates to new location
- Click on elevated surfaces — character navigates around obstacles

---

## **Code Snippets**

```cpp
// AOnsetPlayerController.h
UCLASS()
class AOnsetPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnClickMove();

protected:
    virtual void SetupInputComponent() override;
};
```

```cpp
// AOnsetPlayerController.cpp
void AOnsetPlayerController::BeginPlay()
{
    Super::BeginPlay();
    bShowMouseCursor = true;
}

void AOnsetPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction("ClickMove", IE_Pressed, this, &AOnsetPlayerController::OnClickMove);
}

void AOnsetPlayerController::OnClickMove()
{
    FHitResult Hit;
    GetHitResultUnderCursor(ECC_Visibility, false, Hit);
    if (Hit.bBlockingHit)
    {
        GetPawn()->GetController()->MoveToLocation(Hit.Location);
    }
}
```

---

## **Diagrams**
```
Mouse Click
    │
    ▼
Deproject Screen → World Ray
    │
    ▼
Line Trace (ECC_Visibility)
    │
    ▼
Hit Location
    │
    ▼
MoveToLocation → AI Navigation
    │
    ▼
Character Pathfinding
```

---

## **Common Pitfalls**
- `GetHitResultUnderCursor` returns false if no collision channel is set
- Not assigning the input action in Project Settings
- `bShowMouseCursor` not set to true
- Character has no AIController or MovementComponent
- Touch input not working on mobile — ensure touch interface is enabled in Project Settings

---

## **Episode Checklist**
- [ ] Click moves the character
- [ ] Touch tap moves the character (mobile viewport or touch-enabled device)
- [ ] Cursor visible in game (desktop)
- [ ] Movement stops at clicked/tapped location
- [ ] Works on different surfaces and heights
- [ ] Input action properly mapped

---

## **Public Repo Notes**
- Ensure input mappings are included in the public branch
- Verify navigation mesh works with the default level

---

## **Recording Script**

**Intro:**
"Welcome back. Today we implement the core control scheme — point-and-click movement."

**Body:**
- Enable mouse cursor
- Create the input action mapping
- Wire up the raycast
- Test movement to different locations
- Show how navigation handles obstacles

**Outro:**
"[Next episode](Episode04_Enemy_Spawner.md) we build the enemy spawner — our first real NPC lifecycle system."

---

## **Next Episode Preview**
"[Next time](Episode04_Enemy_Spawner.md), we create the enemy spawner to generate groups of NPCs in the world."
