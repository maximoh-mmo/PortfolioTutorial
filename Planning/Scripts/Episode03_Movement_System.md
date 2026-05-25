# 🎬 **Episode 3 — Movement System (Virtual Joystick + Tap-to-Move + WASD + Gamepad)**

## **Episode Goal**
Implement multi-device movement: touch virtual joystick + tap-to-move, mouse click-to-move, WASD keys, and gamepad controls. Build the cursor abstraction layer so all future raycast systems work with any input device.

---

## **Context & Dependencies**
- Requires [Episode 1](Episode01_Project_Setup.md) (project setup, Enhanced Input architecture, cursor manager component)
- Requires [Episode 2](Episode02_TopDown_Camera.md) (top-down camera)

---

## **High‑Level Summary**
The movement system supports every target input method. Touch gets both a virtual joystick (continuous movement) and tap-to-move (pathfinding). Mouse uses click-to-move. Keyboard gets WASD, and gamepad gets left-stick movement with right-stick cursor emulation. A shared cursor abstraction means all later systems (targeting, abilities) work identically regardless of device.

---

## **Key Concepts Introduced**
- Virtual joystick widget (touch) — drag-based 2D axis input
- Tap-to-move (touch) — screen raycast to `MoveToLocation`
- Click-to-move (mouse) — same raycast pipeline
- WASD movement (keyboard) — direct character velocity
- Gamepad L-Stick movement + R-Stick cursor
- Cursor abstraction layer — unified screen position from any device
- Direct-input / pathfinding hand-off — joystick/WASD interrupts `MoveToLocation`
- Enhanced Input bindings via `UEnhancedInputComponent`

---

## **Technical Breakdown**

### **1. Cursor Abstraction (UCursorManager)**
All raycast systems read cursor position from a single source:
```
Mouse → GetMousePosition()
Touch → last touch event screen position
Gamepad R-Stick → accumulated 2D delta, clamped to viewport
```
```cpp
// UCursorManager::GetCursorPosition(FVector2D& OutScreenPos) -> bool
bool UCursorManager::GetCursorPosition(FVector2D& OutScreenPos)
{
    if (bUsingGamepadCursor) // R-Stick active
    {
        OutScreenPos = GamepadCursorPosition;
        return true;
    }
    if (APlayerController* PC = GetPlayerController())
    {
        return PC->GetMousePosition(OutScreenPos.X, OutScreenPos.Y);
    }
    return false;
}
```

### **2. Virtual Joystick (Touch)**
A `UJoystickWidget` placed in the viewport as a touch zone:
- Touch Begin → record origin
- Touch Move → compute normalized 2D delta → inject into `IA_Move` via `UEnhancedInputLocalPlayerSubsystem::InjectInputForAction()`
- Touch End → return to zero

### **3. IA_Confirm → MoveToLocation (Tap + Click)**
`IA_Confirm` fires on left-click or touch tap:
```cpp
void AOnsetPlayerController::OnConfirm()
{
    FVector2D ScreenPos;
    if (!CursorManager->GetCursorPosition(ScreenPos)) return;

    FHitResult Hit;
    GetHitResultAtScreenPosition(ScreenPos, ECC_Visibility, false, Hit);

    if (Hit.bBlockingHit && !Hit.GetActor()->ActorHasTag("Enemy"))
    {
        MoveToLocation(Hit.Location);
    }
}
```
(Enemy check is a stub — fully branched in Episode 4.)

### **4. IA_Move → Direct Movement (Joystick + WASD + L-Stick)**
```cpp
void AOnsetCharacter::OnMove(const FInputActionValue& Value)
{
    FVector2D MoveVector = Value.Get<FVector2D>();
    AddMovementInput(MoveVector.X, MoveVector.Y);

    // Interrupt any active pathfinding
    if (AController* C = GetController())
    {
        C->StopMovement();
    }
}
```
When `IA_Move` returns to zero, the character stops. If the player previously used tap-to-move, the pathfinding is already cancelled by the interrupt.

### **5. Gamepad R-Stick Cursor**
```cpp
void AOnsetPlayerController::OnCursorMove(const FInputActionValue& Value)
{
    FVector2D Delta = Value.Get<FVector2D>();
    // Scale by sensitivity and frame delta
    CursorManager->AddGamepadCursorDelta(Delta * Sensitivity * GetWorld()->DeltaRealTimeSeconds());
    CursorManager->ClampToViewport();
}
```
A `UGamepadCursorWidget` (crosshair) follows the software cursor position. The widget auto-hides after 2 seconds of R-Stick idle.

### **6. Input Bindings (SetupInputComponent)**
```cpp
void AOnsetPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent);

    Input->BindAction(IA_Confirm, ETriggerEvent::Triggered, this, &AOnsetPlayerController::OnConfirm);
    Input->BindAction(IA_Cursor,  ETriggerEvent::Triggered, this, &AOnsetPlayerController::OnCursorMove);
}
```
```cpp
void AOnsetCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent);

    Input->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AOnsetCharacter::OnMove);
}
```

---

## **Code Snippets**

```cpp
// UCursorManager.h
UCLASS()
class UCursorManager : public UActorComponent
{
    GENERATED_BODY()

public:
    bool GetCursorPosition(FVector2D& OutScreenPos);
    void SetGamepadCursorActive(bool bActive);
    void AddGamepadCursorDelta(FVector2D Delta);
    void ClampToViewport();

private:
    FVector2D GamepadCursorPosition;
    bool bUsingGamepadCursor = false;
};
```

```cpp
// JoystickWidget OnTouchMoved
FVector2D Delta = TouchPosition - TouchOrigin;
Delta /= JoystickRadius;
Delta = Delta.ClampAxes(-1.0, 1.0);

UEnhancedInputLocalPlayerSubsystem* Subsystem = ...;
Subsystem->InjectInputForAction(IA_Move, FInputActionValue(Delta));
```

---

## **Diagrams**
```
Input Device        Cursor Source          Enhanced Input          Character
───────────         ─────────────          ──────────────          ─────────
Touch Joystick  ──→ InjectInputForAction ─→ IA_Move          ──→ AddMovementInput
Touch Tap       ──→ UCursorManager      ─→ IA_Confirm        ─→ MoveToLocation
Mouse Click     ──→ GetMousePosition    ─→ IA_Confirm        ─→ MoveToLocation  
WASD            ────────────────────────→ IA_Move            ─→ AddMovementInput
Gamepad L-Stick ────────────────────────→ IA_Move            ─→ AddMovementInput
Gamepad R-Stick ────────────────────────→ IA_Cursor          ─→ Software Cursor Pos
```

---

## **Common Pitfalls**
- Forgetting to call `StopMovement()` on direct input — character keeps pathfinding
- Virtual joystick not injecting into the right subsystem — must use `APlayerController`'s local subsystem
- Gamepad cursor going outside viewport — must clamp each frame
- `IA_Confirm` firing on both tap and touch-joystick interactions — ensure touch events are routed correctly (tap = short press, joystick = hold+drag)
- Not calling `bShowMouseCursor = true` in PlayerController BeginPlay for desktop

---

## **Episode Checklist**
- [ ] Tap-to-move works (touch)
- [ ] Virtual joystick moves character (touch)
- [ ] Click-to-move works (mouse)
- [ ] WASD movement with pathfinding interrupt (keyboard)
- [ ] Gamepad L-Stick movement (gamepad)
- [ ] Gamepad R-Stick cursor moves on screen, clamped
- [ ] Cursor manager provides correct position from all sources
- [ ] Switching input methods mid-movement is clean
- [ ] Navigation around obstacles works

---

## **Public Repo Notes**
- Include Enhanced Input assets (IA_Move, IA_Confirm, IA_Cursor, IMC_Touch, IMC_Desktop, IMC_Gamepad)
- Ensure Nav Mesh Bounds Volume is in the map
- Include a default pawn that can be possessed for testing

---

## **Recording Script**

**Intro:**
"Welcome back. Last episode we set up the camera. Today we implement movement — and not just one kind. We're building a movement system that works with touch, mouse, keyboard, and gamepad, all through a shared input architecture."

**Body:**
- Recap the Enhanced Input setup from Episode 1
- Build the cursor manager
- Implement the virtual joystick widget for touch
- Wire up IA_Confirm for tap-to-move and click-to-move
- Wire up IA_Move for WASD and gamepad L-Stick
- Implement the gamepad R-Stick software cursor
- Show hand-off between direct input and pathfinding
- Test with all input methods

**Outro:**
"In the next episode, we add click-to-target — we can move, now let's pick a fight."

---

## **Next Episode Preview**
"Next time, we implement the targeting system so we can select enemies and route attacks."
