# 🎬 **Episode 2 — Top‑Down Camera Setup**

## **Episode Goal**
Implement a fixed top‑down camera with smoothing and collision handling.

---

## **Context & Dependencies**
- Requires [Episode 1](Episode01_Project_Setup.md) (project setup, base character class)

---

## **High‑Level Summary**
We add a top-down camera to the player character. The camera sits above the player, looks straight down at a slight angle, follows smoothly, and handles world collision to avoid clipping through geometry.

---

## **Key Concepts Introduced**
- SpringArmComponent
- CameraComponent
- Camera rotation and distance
- Camera collision (probe channel)
- Camera smoothing (lag settings)

---

## **Technical Breakdown**

### **1. Modify Character Header**
Add SpringArm and Camera components to `AOnsetCharacter`:

```cpp
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
class USpringArmComponent* CameraBoom;

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
class UCameraComponent* FollowCamera;
```

### **2. Create Components in Constructor**
```cpp
CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
CameraBoom->SetupAttachment(RootComponent);
CameraBoom->TargetArmLength = 1000.0f;
CameraBoom->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f));
CameraBoom->bDoCollisionTest = true;

FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
```

### **3. Camera Settings**
- Arm length: 1000–1200 units
- Pitch rotation: −55 to −65 degrees (slight angle for depth perception)
- Collision probe: WorldStatic + WorldDynamic
- Lag settings: Enable camera lag (speed ~5–10) for smooth movement feel

### **4. Testing**
- Press Play and observe camera following the character
- Move the character to verify smooth camera tracking
- Walk near walls to verify collision push-back

---

## **Code Snippets**

```cpp
// AOnsetCharacter.h additions
UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
TObjectPtr<USpringArmComponent> CameraBoom;

UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
TObjectPtr<UCameraComponent> FollowCamera;
```

```cpp
// AOnsetCharacter.cpp constructor
AOnsetCharacter::AOnsetCharacter()
{
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(GetRootComponent());
    CameraBoom->TargetArmLength = 1000.0f;
    CameraBoom->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f));
    CameraBoom->bDoCollisionTest = true;
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 8.0f;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
}
```

---

## **Diagrams**
```
[Character]
    │
    ▼
[SpringArmComponent] ← 1000 units, −60° pitch, collision test ON
    │
    ▼
[CameraComponent] ← top-down view
```

---

## **Common Pitfalls**
- Setting camera too close or too far for gameplay readability
- Forgetting to enable camera lag — movement feels jerky
- Wrong collision channel causes camera to clip through floors
- Not attaching CameraBoom to RootComponent

---

## **Episode Checklist**
- [ ] SpringArm + Camera added to character
- [ ] Camera angle and distance feel good in PIE
- [ ] Camera collision pushes back from walls
- [ ] Smooth lag feels natural
- [ ] Works with character movement

---

## **Public Repo Notes**
- Remove any default pawn camera (if the template added one)
- Remove StarterContent if present

---

## **Recording Script**

**Intro:**
"Welcome back. In this episode we're adding the top-down camera — the lens through which the player will experience the entire game."

**Body:**
- Add SpringArm and Camera components
- Walk through each parameter (arm length, rotation, collision)
- Toggle camera lag on/off to demonstrate the difference
- Test with movement

**Outro:**
"[Next episode](Episode03_ClickToMove.md) we add click-to-move so the player can actually navigate the world."

---

## **Next Episode Preview**
"[Next time](Episode03_ClickToMove.md), we implement point-and-click movement using raycasts and AI navigation."
