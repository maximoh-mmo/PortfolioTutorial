# 🎬 **Episode 3 — Point‑and‑Click Movement**

## **Episode Goal**
Use mouse raycasts and MoveToLocation to implement click‑to‑move.

---

## **Context & Dependencies**
- Requires [Episode 2](Episode02_TopDown_Camera.md) (top-down camera)

---

## **High‑Level Summary**
We add click-to-move by raycasting from the mouse cursor into the world, finding the clicked location, and sending the character there using Unreal's AI navigation system. This is the primary control scheme for the entire demo.

---

## **Key Concepts Introduced**
- Mouse cursor input (Show Mouse Cursor)
- Raycasting from screen to world (DeprojectScreenToWorld)
- Hit result and collision channels
- AI Navigation / MoveToLocation
- Input action mappings
- Simple click feedback (optional)

---

## **Technical Breakdown**

### **1. Input Setup**
Enable mouse cursor on PlayerController:
```cpp
bShowMouseCursor = true;
DefaultMouseCursor = EMouseCursor::Default;
```

### **2. Input Mapping**
Add an `IA_ClickMove` action mapping (Left Mouse Button).

### **3. Raycast Logic**
```cpp
void AMPTDARPGPlayerController::OnClickMove()
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
// AMPTDARPGPlayerController.h
UCLASS()
class AMPTDARPGPlayerController : public APlayerController
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
// AMPTDARPGPlayerController.cpp
void AMPTDARPGPlayerController::BeginPlay()
{
    Super::BeginPlay();
    bShowMouseCursor = true;
}

void AMPTDARPGPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction("ClickMove", IE_Pressed, this, &AMPTDARPGPlayerController::OnClickMove);
}

void AMPTDARPGPlayerController::OnClickMove()
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

---

## **Episode Checklist**
- [ ] Click moves the character
- [ ] Cursor visible in game
- [ ] Movement stops at clicked location
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
"[Next episode](Episode04_ClickToTarget.md) we add click-to-target so we can select enemies to attack."

---

## **Next Episode Preview**
"[Next time](Episode04_ClickToTarget.md), we implement click-to-target selection and basic attack routing."
