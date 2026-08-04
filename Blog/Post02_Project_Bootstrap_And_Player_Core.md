# 🎬 Post 2 — Blank Slate to Input-Agnostic Player Core: Stripping the Template and Building Onset's Movement

*6 commits · May 24–27, 2026 · covering project re-init, Enhanced Input, and the A1.1–A1.4 player systems*

---

## Introduction

Post 1 was a day of pure scaffolding — folders, templates, plans, and a rename. The docs told me exactly what Onset *should* be: a top-down ARPG with input-agnostic movement (touch, mouse, keyboard, gamepad), targeting, and GAS combat later. But on the morning of May 24 the repo still contained an untouched copy of Unreal's TopDown template wearing the name `MPTDARPG`.

The plan said "keep the core TopDown base, strip the demo variants." The reality, four commits later, was a blank slate rebuilt around a **multi-device input architecture** the original plan never described. This post covers that pivot — why I deleted a working template, why Enhanced Input became non-negotiable, and how the A1.1–A1.4 systems all landed in one giant commit that made the docs true.

---

## Technical Exposition

### The first false start: click-to-move, legacy input

The very first code commit of the arc (`219ff51`) was small, and honestly, it was the old me writing code. A `PlayerController` with a legacy `InputComponent` binding, a click handler that raycasts and checks `ActorHasTag("Enemy")`:

```cpp
void AOnsetPlayerController::OnClick()
{
	FHitResult HitResult;
	GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, false, HitResult);
	if (HitResult.bBlockingHit)
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor && HitActor->ActorHasTag("Enemy"))
		{
			TargetingComponent->SetCurrentTarget(HitActor);
		}
		else
		{
			TargetingComponent->ClearCurrentTarget();
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, HitResult.Location);
		}
	}
	else
	{
		TargetingComponent->ClearCurrentTarget();
	}
}
```

This works. It's how the engine template does it. It's also fundamentally **mouse-only**, and the docs I'd written said the whole point of Onset was a *multi-device* control scheme. The commit even carries a small leftover bug (movement firing on some misses) that I fixed in the same commit. But the real issue was architectural: I was solving "how do I move a character" when the actual problem was "how do **five different input devices** all drive the same actions."

### The docs pivot that changed everything

The next day (`2ee2e7b`, docs-only) I rewrote the plan. `Episode03_ClickToMove.md` became `Episode03_Movement_System.md`, describing a virtual joystick + tap-to-move + WASD + gamepad left stick, plus a software cursor for the gamepad right stick. This was **document-first development in action**: the docs identified the real requirement *before* any more code was written, and the code was forced to catch up.

The design boiled down to three insights:

1. **One shared 2D axis action** (`IA_Move`) for joystick / WASD / left stick. Device differences vanish — they all produce `FVector2D`.
2. **One shared confirm action** (`IA_Primary`) for tap / click / gamepad A-button. The *context* of the hit decides what it does.
3. **A cursor abstraction** (`UCursorManager`) so every raycast goes through one place that knows whether the "cursor" is a mouse, a touch, or an emulated gamepad position.

### Stripping the template for real

`1d2baa8` deleted essentially everything from Post 1's template import — mannequins, `Lvl_TopDown`, cursor FX, the old input actions — and re-initialized a blank `Onset` project. This reversed Post 1's "keep the template base" decision. Why? Because the template's value is a guaranteed-working starting point, but its cost is **inherited assumptions**: a click-to-move controller, mouse-cursor logic, and template asset cruft that fights the multi-device design. Once the requirements were clear, the template was more obstacle than shortcut. A blank project with a minimal `Onset.Build.cs` was the honest starting point.

---

## Implementation Details

### 1. Configure the modules the design requires

`84a256d` wires up the dependencies the new input architecture needs. Note the ordering — *modules first, code after*:

```csharp
PublicDependencyModuleNames.AddRange(new string[] {
	"Core", "CoreUObject", "Engine", "InputCore",
	"EnhancedInput", "AIModule", "NavigationSystem", "UMG"
});
```

`EnhancedInput` for the new input system, `UMG` for the joystick/cursor widgets, `AIModule` + `NavigationSystem` for movement and (later) AI. This is the "dependencies first" habit that shows up again in later arcs — declare the dependency surface before the code that needs it.

### 2. Build the cursor abstraction

`UCursorManager` is the quiet hero of the whole architecture. Every input raycast goes through `GetCursorPosition()` rather than raw `GetHitResultUnderCursor`, so the source of the "cursor" is a detail the rest of the code never sees:

```cpp
bool UCursorManager::GetCursorPosition(FVector2D& OutPosition) const
{
	if (bUsingGamepadCursor)
	{
		OutPosition = GamepadCursorPosition;
		return true;
	}
	if (!LastTouchPosition.IsZero())
	{
		OutPosition = LastTouchPosition;
		return true;
	}
	APlayerController* Controller = CachedPlayerController.Get();
	if (!Controller)
	{
		Controller = Cast<APlayerController>(GetOwner());
		CachedPlayerController = Controller;
	}
	if (Controller)
	{
		return Controller->GetMousePosition(OutPosition.X, OutPosition.Y);
	}
	return false;
}
```

The gamepad widget stays deliberately dumb — the header literally documents that "the PlayerController tells it where to be — no tick, no timers." All position authority lives in the cursor manager.

### 3. Register the mapping contexts and bind actions

With three mapping contexts (keyboard/mouse, touch, gamepad), the controller adds them all at startup:

```cpp
UEnhancedInputLocalPlayerSubsystem* Subsystem =
	ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
if (Subsystem)
{
	Subsystem->AddMappingContext(IMC_Touch, 0);
	Subsystem->AddMappingContext(IMC_KbMouse, 0);
	Subsystem->AddMappingContext(IMC_Gamepad, 0);
}
```

The action bindings are device-agnostic by construction:

```cpp
EnhancedInputComponent->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AOnsetPlayerController::OnMove);
EnhancedInputComponent->BindAction(IA_Cursor, ETriggerEvent::Triggered, this, &AOnsetPlayerController::OnCursorMove);
EnhancedInputComponent->BindAction(IA_Primary, ETriggerEvent::Started, this, &AOnsetPlayerController::OnPrimaryInteraction);
```

`OnMove` reads a `FVector2D` regardless of whether it came from WASD, a joystick, or a gamepad stick:

```cpp
void AOnsetPlayerController::OnMove(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	if (MovementVector.IsZero()) return;
	StopMovement(); // direct input is a deliberate override of pathfinding
	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->AddMovementInput(ControlledPawn->GetActorForwardVector(), MovementVector.Y);
		ControlledPawn->AddMovementInput(ControlledPawn->GetActorRightVector(), MovementVector.X);
	}
}
```

The `StopMovement()` call encodes a deliberate design decision (recorded in the A1.3 TODO): *direct input is the player's intentional override of pathfinding; auto-resuming on release would fight the player's intent.* Tap-to-move remains a separate action.

### 4. One confirm handler, branching on hit context

`OnPrimaryInteraction` is the unification of "tap enemy → target" and "tap ground → move." Instead of separate move/target actions fighting over the same tap, one handler reads the cursor position, raycasts, and branches on what it hit:

```cpp
void AOnsetPlayerController::OnPrimaryInteraction(const FInputActionValue& Value)
{
	FVector2D ScreenPos;
	if (!CursorManager->GetCursorPosition(ScreenPos)) return;

	FHitResult HitResult;
	if (!GetHitResultAtScreenPosition(ScreenPos, ECC_Visibility, false, HitResult)) return;

	AActor* HitActor = HitResult.GetActor();
	if (HitActor && HitActor->ActorHasTag("Enemy"))
	{
		TargetingComponent->SetTarget(HitActor);
	}
	else
	{
		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		FNavLocation NavLoc;
		if (NavSys && NavSys->ProjectPointToNavigation(HitResult.Location, NavLoc))
		{
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, NavLoc.Location);
		}
		TargetingComponent->ClearTarget();
	}
}
```

Two refinements over the first attempt: click positions are **projected onto the navmesh** before `SimpleMoveToLocation` (no more pathing to un-navigable points), and the cursor comes from the abstraction, not the OS mouse. The docs flagged this exact design as the fix for the classic "tap registers as both move and target" pitfall.

### 5. The touch bridge: widgets inject input

The `UJoystickWidget` doesn't call movement code directly — it *injects* a synthetic value into the same `IA_Move` action:

```cpp
void UJoystickWidget::InjectMovementInput()
{
	if (!IA_Move) return;
	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetOwningLocalPlayer());
	if (Subsystem)
	{
		Subsystem->InjectInputForAction(IA_Move, FInputActionValue(CurrentAxis), {}, {});
	}
}
```

This is elegant: the widget produces the same `FVector2D` the keyboard produces, and every downstream consumer behaves identically. The touch device becomes "a joystick that exists only on screen."

### 6. Camera on the pawn, input on the controller

`AOnsetPlayerCharacter` owns a spring arm + follow camera (fixed top-down, smoothing, collision push-back), while all input lives on the controller. This separation of concerns — documented in the class headers — is what lets an AI controller later possess the same pawn without inheriting player input handling.

### 7. Close the loop: make the docs true

The final commit (`1419c39`) is pure documentation sync: `Player_System.md`, `Architecture Overview.md`, `Targeting_System.md`, and `UI_System.md` all updated to name the classes that actually shipped (`UCursorManager`, `UJoystickWidget`, `UGamepadCursorWidget`), and the checklist corrected — A1 went from "planned across May 27–Jun 1" to **72% (26/36) complete in one day**.

---

## Results & Validation

- Blank, template-free `Onset` project on UE 5.8 with Enhanced Input configured.
- Input-agnostic movement: WASD, touch joystick, gamepad left stick all drive one `FVector2D` action.
- Unified confirm handler: tap/click/A-button resolves to target or move via hit context.
- `UCursorManager` abstraction covering mouse, touch, and emulated gamepad cursor.
- Camera pawn + input controller separation, ready for later AI possession.
- `IA_Ability1-4` and `IA_PvPToggle` action assets created — deliberately **unbound**, a placeholder for the next arc.

**Limitations acknowledged:** movement was character-relative at this point (the "W always moves toward screen top" behavior came much later, and the docs marked PvP/autoplay/replication as future work).

---

## Challenges & Solutions

### The template, again

**Problem:** Post 1 explicitly said "keep the core TopDown base." Within days I deleted it entirely.

**Solution:** The docs pivot revealed the template's core assumptions (mouse-first input, click-to-move) directly contradicted the multi-device design. Keeping it meant inheriting a mental model I'd have to fight forever. Deleting it cost a day but bought a clean foundation. **Lesson:** plans are hypotheses; when new requirements contradict an old decision, the plan should change — and this repo's whole workflow is built on that willingness to revise.

### Input double-scaling

**Problem:** Enhanced Input values were being scaled twice (once by the old legacy input scales, once by Enhanced Input).

**Solution:** Flushed legacy input scales via `DefaultInput.ini` (`bEnableLegacyInputScales=False`) — a one-line config fix that's easy to miss when you're mid-migration and mixing input systems.

### Component-null debugging

**Problem:** Right after the big commit, the controller was hitting null `CursorManager`/`TargetingComponent` symptoms.

**Solution:** Temporary constructor `UE_LOG` validity checks plus a `if (!TargetingComponent) return;` guard in the interaction handler. The checks were debug scaffolding (stripped in later arcs), but the guard stayed — defensive programming that outlived the debugging session.

---

## Reflection & Lessons Learned

The biggest lesson: **the docs pivot saved the project from my own first instinct.** The naive click-to-move controller worked, but "works" wasn't the requirement — *works on every device* was. Writing the movement-system doc forced the requirement into the open before I'd invested in the wrong abstraction.

I also learned that Enhanced Input's `InjectInputForAction` is the correct bridge between widgets and gameplay — it lets UI own input presentation while gameplay owns semantics. That pattern (widgets inject, controllers interpret) survived into the CommonUI era much later.

What I'd do differently: I'd have written the multi-device doc *before* the first code commit instead of one day after. The false start wasn't expensive here (one day, ~100 lines), but the habit of "code first, realize the design later" is exactly what this repo exists to avoid.

---

## Forward-Looking Content

| Phase | Status |
|---|---|
| A1.1–A1.4 — setup, camera, movement, targeting | ✅ Complete (72% of A1) |
| A1.4 — ability input pipeline (bind `IA_Ability1-4`) | ⏭️ Next |
| A1.5 — ability targeting library (`FAbilityTargetData`) | Next |
| A1.6 — PvP toggle (`UOnsetStatics`) | Next |

**What's next:** the arc shipped `IA_Ability1-4` assets but left them *unbound*. Post 3 wires them up — binding the four ability actions, gating target assignment behind validation, building a static `UAbilityTargetingLibrary` that turns a target into structured data for GAS, and adding the PvP toggle so players can choose whether other players are even targetable. That closes out sprint A1 at 100%.

> **Next time in Post 3:** You Can Fight Now — the ability input pipeline, the target-data library, and the PvP toggle that makes the demo honest about who you can hit.

---

*Arc commits: `1d2baa8`, `219ff51`, `2ee2e7b`, `84a256d`, `266461c`, `1419c39`. Related docs: `Docs/Player/Player_System.md`, `Docs/Gameplay/Targeting_System.md`, `Docs/Gameplay/UI_System.md`.*
