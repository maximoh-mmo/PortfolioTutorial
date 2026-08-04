# 🎬 Post 13 — Menus That Actually Stack: CommonUI, Screen Stacks, and the UMG-to-CommonUI Migration

*4 commits · Jul 8, 2026 · covering CommonUI UI Migration*

---

## Introduction

Sprint A5b built the persistence layer. The game can save, load, and travel between worlds. But the menus — the lobby, the character select, the pause screen — are canvas-drawn HUDs that don't render in packaged `-game` builds. They work in the editor, they work in PIE, and they vanish silently in shipping.

CommonUI solves this. It's Epic's official UI framework for UE 5, built on a screen-stack architecture that handles layered navigation, modal dialogs, and platform-specific input. The migration is 4 commits, but the implications are structural: every menu in the game moves from `AHUD` subclasses to `UCommonActivatableWidget`, from canvas drawing to UMG, from fragile input handling to a centralized `UCommonInputSubsystem`.

This post is short — the migration is small — but the architectural decision behind it is big. CommonUI is the foundation that makes the game feel like a game, not a tech demo.

---

## Technical Exposition

### Why CommonUI

The canvas HUD fallback was always a stopgap. UMG widgets compile and work in PIE but silently fail in packaged `-game` builds — a known engine quirk that has no workaround except a different rendering path. CommonUI's `UCommonActivatableWidget` is designed to work in packaged builds. It's the official UE 5 UI framework, and it handles the exact problems the canvas HUDs were kludging around.

### The screen stack architecture

CommonUI introduces a layered navigation model with three stack types:

| Stack | Purpose | Examples |
|---|---|---|
| Game | In-world UI | HUD, minimap, damage indicators |
| Menu | Full-screen overlays | Pause, settings, inventory |
| Modal | Blocking dialogs | Confirm quit, error messages |

The `UCommonInputSubsystem` owns navigation between stacks. Input mode (GameOnly, UIOnly, GameAndUI) is managed centrally — no more `SetInputMode` calls scattered across widgets. The `FInputModeGameAndUI` transition that previously caused cursor flicker is gone.

### The widget lifecycle

`UCommonActivatableWidget` replaces `UUserWidget` for all screens. The lifecycle is explicit: `OnActivated()` (called when widget becomes active), `OnDeactivated()` (called when pushed off the stack), and `NativeOnActivated()`/`NativeOnDeactivated()` for Blueprint overrides. This replaces the fragile `Construct`/`Destruct` pattern that caused initialization bugs when widgets were pre-loaded and hidden.

---

## Implementation Details

### 1. The GameInstanceSubsystem navigation hub

`UOnsetUISubsystem` (GameInstanceSubsystem) owns the screen stack and manages transitions:

```cpp
UOnsetUISubsystem::OpenScreen(TSubclassOf<UCommonActivatableWidget> ScreenClass, FName LayerName)
{
    UCommonActivatableWidget* Widget = CreateWidget<UCommonActivatableWidget>(this, ScreenClass);
    Widget->SetOwningPlayer(LocalPlayer);
    Widget->AddToScreenStack(LayerName);
}
```

The subsystem is the single point of navigation. No widget opens another widget directly — all navigation goes through the subsystem. This prevents the "who owns the stack" problem that plagues ad-hoc UMG navigation.

### 2. The screen classes

Each screen becomes a `UCommonActivatableWidget` subclass with a corresponding UMG widget blueprint:

| Screen | Class | Layer |
|---|---|---|
| Main Menu | `UOnsetMainMenuWidget` | `Menu` |
| Character Select | `UOnsetCharacterSelectWidget` | `Menu` |
| Pause | `UOnsetPauseMenuWidget` | `Menu` |
| Settings | `UOnsetSettingsWidget` | `Modal` |
| HUD | `UOnsetHUDWidget` | `Game` |

The `UOnsetHUDWidget` replaces the canvas `AHUD` fallback for the in-world HUD. It's the only Game-layer widget; all others are Menu or Modal.

### 3. Input subsystem integration

`UCommonInputSubsystem` manages input modes. The `FInputModeGameAndUI` transition that caused cursor flicker in the canvas HUDs is replaced by CommonUI's built-in input mode management:

```cpp
void UOnsetUISubsystem::SetInputMode(ECommonInputType InputType)
{
    UCommonInputSubsystem* InputSub = UCommonInputSubsystem::Get(this);
    InputSub->SetInputType(InputType);
    InputSub->SetDefaultInputAction(nullptr); // clear any pending input
}
```

Input type is `UI` for menus, `Game` for gameplay, `GameAndUI` for hybrid states. The subsystem handles the transition cleanly — no flicker, no stuck cursors.

### 4. The widget binding pattern

Each `UCommonActivatableWidget` binds to its data in `NativeOnActivated()` and unbinds in `NativeOnDeactivated()`:

```cpp
void UOnsetCharacterSelectWidget::NativeOnActivated()
{
    Super::NativeOnActivated();
    UISubsystem = GetGameInstanceSubsystem<UOnsetUISubsystem>(this);
    UISubsystem->OnCharacterDataReceived.AddUObject(this, &ThisClass::HandleCharacterData);
    UISubsystem->RequestAccountData();
}

void UOnsetCharacterSelectWidget::NativeOnDeactivated()
{
    UISubsystem->OnCharacterDataReceived.RemoveAll(this);
    Super::NativeOnDeactivated();
}
```

This replaces the manual `Construct`/`Destruct` binding that caused memory leaks when widgets were pre-loaded and hidden.

### 5. The UMG widget blueprints

Each screen has a corresponding UMG widget blueprint. The blueprints are authored in the editor, not in C++, and they bind to the C++ widget class via the `Widget Class` property. The C++ class handles logic; the Blueprint handles layout. This separation of concerns is CommonUI's intended pattern.

---

## Results & Validation

- All canvas HUD fallbacks replaced with CommonUI `UCommonActivatableWidget` subclasses
- `UOnsetUISubsystem` (GameInstanceSubsystem) as centralized navigation hub
- Three screen layers: Game, Menu, Modal
- `UCommonInputSubsystem` for input mode management — no more flicker
- Widget lifecycle via `NativeOnActivated`/`NativeOnDeactivated` — no more Construct/Destruct leaks
- UMG widget blueprints for all screens — layout in Blueprint, logic in C++
- Packaged `-game` builds render menus correctly for the first time

---

## Challenges & Solutions

### UMG doesn't render in packaged `-game` builds

**Problem:** `UUserWidget` subclasses silently don't render in packaged `-game` builds. The canvas HUD fallback was the workaround.

**Solution:** CommonUI's `UCommonActivatableWidget` renders correctly in packaged builds. The migration replaces the canvas fallback entirely.

### Who owns the screen stack?

**Problem:** Ad-hoc navigation (widget A opens widget B directly) creates tangled ownership and makes back-button behavior inconsistent.

**Solution:** `UOnsetUISubsystem` owns all navigation. No widget opens another widget directly — all navigation goes through the subsystem's `OpenScreen()` method.

### Input mode transitions causing cursor flicker

**Problem:** Manual `SetInputMode` calls in different widgets caused cursor flicker during transitions.

**Solution:** `UCommonInputSubsystem` manages input modes centrally. The subsystem handles transitions cleanly — no flicker, no stuck cursors.

### Blueprint vs C++ widget logic split

**Problem:** Early widgets put logic in Blueprint that belonged in C++ (data fetching, RPC calls).

**Solution:** C++ handles all logic (data fetching, RPC calls, navigation). Blueprint handles layout only. The `NativeOnActivated`/`NativeOnDeactivated` pattern makes this clean: Blueprint overrides `NativeOnActivated` for layout setup, C++ handles data binding.

---

## Reflection & Lessons Learned

The CommonUI migration is the smallest post in the series by commit count (4 commits) but arguably the most impactful architectural decision. The canvas HUD fallback was a working solution for PIE and editor testing, but it was always a stopgap. CommonUI is the real UI framework, and it's what makes the game feel like a game rather than a tech demo.

The centralized navigation hub (`UOnsetUISubsystem`) is the pattern that makes CommonUI work. Without it, the screen stack becomes a tangled mess of widget-to-widget references. The subsystem is the single point of navigation, and every widget obeys it. This is the same pattern as `IPlayerDataStore` — an interface/abstraction that decouples consumers from implementations.

The input subsystem integration is the most invisible but most important change. Cursor flicker during input mode transitions is one of those bugs that players never notice consciously but always feel. CommonUI's `UCommonInputSubsystem` eliminates it entirely.

---

## Forward-Looking Content

| Phase | Status |
|---|---|
| A5 — Multiplayer + Steam | ✅ **100% (42/42)** |
| A5b — Persistence & Account System | ✅ **100% (30/30)** |
| CommonUI UI Migration | ✅ **100% (4/4)** |
| A5c — Auth Extraction & Login Server | ⏭️ Next |
| Character Classes + Final Polish | Planned |

**What's next:** the UI is solid, but the game still has no cloud account system. Post 14 extracts the auth logic from the Steam SDK into a standalone login server, adds PostgreSQL-backed account storage, and implements the two-server token auth flow. Then Post 15 delivers the final polish: character classes, loading screens, and API access control hardening.

> **Next time in Post 14:** The Cloud Spine — extracting auth into a login server with PostgreSQL, Lambda, and DynamoDB.

---

*Arc commits: `f58c84b → 555e9ed`. Related docs: `Docs/UI/CommonUI_Migration.md`, `Planning/Scripts/Episode40–43_*.md`.*
