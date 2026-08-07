# Onset HUD — Asset Checklist & Implementation Steps

This checklist covers the editor assets needed to make the assignable, data-driven
ability bar and the designable HUD (health bar, target HUD, damage numbers) render
with full functionality. All widget *logic* lives in C++ (`Project/Source/Onset/Public/UI/`
+ `Private/UI/`); the Widget Blueprints below only provide the visual tree and styling.

The bind names below are **case-sensitive** and come straight from the C++ headers.
- `BindWidget` = required; the Blueprint fails to compile if the widget is missing.
- `BindWidgetOptional` = binding is optional, but if the named widget is absent that
  HUD element silently does nothing.

Since the HUD rework, C++ never builds widgets: all visual trees come from the WBPs.
All text widgets are **Common Text** (`UCommonTextBlock`, set via the *Content* palette)
so they can use a Common UI style. No `ProgressBar`/`TextBlock` is bound anywhere anymore.

---

## Proposed asset locations

- New UI: `Content/UI/HUD/`
- Ability icons (your own art): `Content/UI/Textures/` or `Content/Game/Combat/Icons/`
- Combat assets: `Content/Game/Combat/` (paths are hardcoded in C++, do not rename)

---

## A. Gameplay assets (required — icons + cooldowns depend on them)

These paths are hardcoded via `ConstructorHelpers::FObjectFinder` / `LoadObject` in:
- `Source/Onset/Private/Combat/OnsetGA_AoE.cpp:19-26`
- `Source/Onset/Private/Combat/OnsetGA_Cone.cpp`
- `Source/Onset/Private/Combat/OnsetGA_Shadowstep.cpp`
- `Source/Onset/Private/Core/OnsetBaseCharacter.cpp:48-66` (default grants)

If a GA Blueprint is missing, the C++ class fallback still runs — but the slot icon is
only set on the Blueprint (`UOnsetGameplayAbility::AbilityIcon`).

| # | Asset | Location | Parent | Required settings |
|---|-------|----------|--------|-------------------|
| A1 | `GA_AoE` | `Content/Game/Combat/` | `UOnsetGA_AoE` | `Ability UI → AbilityIcon` = AoE texture |
| A2 | `GA_Cone` | `Content/Game/Combat/` | `UOnsetGA_Cone` | `Ability UI → AbilityIcon` = Cone texture |
| A3 | `GA_Shadowstep` | `Content/Game/Combat/` | `UOnsetGA_Shadowstep` | optional (not bound to a slot yet) |
| A4 | `GE_AoE_Damage` | `Content/Game/Combat/` | `UGameplayEffect` | Instant (`Duration Policy = Instant`), Health modifier `< 0` (copy `GE_BasicAttackDamage`) |
| A5 | `GE_AoE_Cooldown` | `Content/Game/Combat/` | `UGameplayEffect` | `Duration Policy = Has Duration`; **Granted Tags: `Cooldown.AoE` (and only that tag)** |
| A6 | `GE_Cone_Damage` | `Content/Game/Combat/` | `UGameplayEffect` | same as A4 |
| A7 | `GE_Cone_Cooldown` | `Content/Game/Combat/` | `UGameplayEffect` | `Has Duration`; **Granted Tags: `Cooldown.Cone` only** |
| A8 | `GE_Shadowstep_Cooldown` | `Content/Game/Combat/` | `UGameplayEffect` | `Has Duration`; **Granted Tags: `Cooldown.Shadowstep` only** |
| A9 | `GE_Shadowstep_Invuln` | `Content/Game/Combat/` | `UGameplayEffect` | `Has Duration`; Granted Tags: `State.Invulnerable` |

> **Critical**: the cooldown GE must grant **exactly one** owned tag.
> `UOnsetGameplayAbility::GetPrimaryCooldownTag()` returns the *first* tag in the
> cooldown GE's granted tags (`OnsetGameplayAbility.cpp:7-14`), and the ability bar
> tracks that single tag to start/stop each slot's cooldown fill. Extra tags break
> the display.

---

## B. UI Widget Blueprints (all 6 required)

### B1. `WBP_AbilitySlot` — parent `UAbilitySlotWidget`
All four binds are **`BindWidget` (required)** — compile error if missing; exact
names/casing:

| Widget name | Widget type | Purpose |
|-------------|-------------|---------|
| `SlotButton` | OnsetButtonBase (Common Button) | whole-slot click target; routes to the slot's ability |
| `AbilityIcon` | Image | ability icon (shown when an ability is assigned) |
| `EmptyIcon` | Image | placeholder icon (shown when no ability is assigned); set your empty-slot art on this widget |
| `KeyLabel` | Common Text | key label, e.g. "1" (C++ sets the text) |

Suggested layout: Button 56×56, with `AbilityIcon` and `EmptyIcon` overlaid centered
(~32×32), KeyLabel in a corner. Slots are **always visible**: C++ swaps between
`AbilityIcon` (assigned) and `EmptyIcon` (no ability) — the designer positions/styles both.

**Cooldown fill (designer-owned, event-driven).** There is no progress bar and no
countdown text. Implement the fill yourself in the WBP (e.g. an `Image` overlay with a
masked material, or an opacity/scale animation) and drive it from two blueprint events:

- `OnCooldownStarted(Duration)` — play a 1-second fill animation with
  **Play Rate = `1 / Duration`** (so it empties in exactly the cooldown's duration).
- `OnCooldownEnded()` — stop/reset the fill.

C++ calls these from the cooldown tag events; it never polls per-frame.

### B2. `WBP_AbilityBar` — parent `UAbilityBarWidget`
- Must contain a **Horizontal Box** named `SlotContainer` (root or near-root).
  It is `BindWidgetOptional`; if absent no slots are created (no fallback is built).
- **Class Defaults → Ability Bar → `AbilitySlotWidgetClass` = `WBP_AbilitySlot`**
  (required; no slots are created without it).
- C++ adds `FMargin(4)` padding per slot child, so keep your own spacing thin.

### B3. `WBP_PlayerHealthBar` — parent `UPlayerHealthBarWidget`
| Widget name | Widget type | Purpose |
|-------------|-------------|---------|
| `HealthText` | Common Text | `"X / Y"` (C++ formats it) |

The bar itself is a **material-driven fill**, not a progress bar. C++ exposes
`HealthPercent` (BlueprintReadOnly) and calls `OnHealthPercentChanged(Percent)` only when
the ratio actually changes. In the WBP, bind that event to drive your fill material
(e.g. set a scalar parameter on a `MaterialInstanceDynamic` or play a short fill anim).

### B4. `WBP_TargetHUD` — parent `UTargetHUDWidget`
| Widget name | Widget type | Purpose |
|-------------|-------------|---------|
| `ReticleBorder` | Border | reticle / bracket around the target |
| `NameText` | Common Text | target name |

Target health fill uses the same material approach: C++ exposes `TargetHealthPercent`
(BlueprintReadOnly) and calls `OnTargetHealthPercentChanged(Percent)` on change — bind it
to your fill material.

- **Must be a direct child of a Canvas Panel** (see B6): the widget repositions itself
  every tick via `Cast<UCanvasPanelSlot>(Slot)->SetPosition()`
  (`TargetHUDWidget.cpp:75-77`). Inside a Horizontal/Vertical box the positioning
  silently fails.

### B5. `WBP_DamageNumber` — parent `UDamageNumberWidget`
| Widget name | Widget type | Purpose |
|-------------|-------------|---------|
| `NumberText` | Common Text | the amount (C++ sets value, color fade, float animation) |

Root should size to content (AutoSize slot added by the pool).

### B6. `WBP_HUD` — parent `UHUDWidget`
Must contain all four named widgets (`BindWidgetOptional`; a missing one silently
does nothing):

| Widget name | Widget type |
|-------------|-------------|
| `PlayerHealthBar` | nested child widget **WBP_PlayerHealthBar** |
| `AbilityBar` | nested child widget **WBP_AbilityBar** |
| `TargetHUD` | nested child widget **WBP_TargetHUD**, direct child of a Canvas |
| `DamageNumberLayer` | **Canvas Panel** (the pool of up to 64 WBP_DamageNumber children is added here) |

Suggested tree:

```
Canvas Panel (root)
├─ PlayerHealthBar      (anchored bottom-center)
├─ AbilityBar           (anchored bottom-center, above health bar)
└─ DamageNumberLayer    (full-screen canvas)
   └─ WBP_TargetHUD     (repositioned each tick; collapsed until a target exists)
```

### B7. Wire-up: `Content/Input/MyOnsetPlayerController` — **PENDING**
Open `MyOnsetPlayerController.uasset` → **HUD → `HUDWidgetClass` = `WBP_HUD`**.
**Status: not done yet** — verified the asset has no `WBP_HUD` reference, so the runtime
falls back to plain `UHUDWidget` (no WBP styling) until this is set. The GameMode already
loads this controller (`OnsetGameModeBase.cpp:34-38`) and `CreateHUD` instantiates the
class (`OnsetPlayerController.cpp:289-310`).

---

## C. Cleanup (done)

- `Content/UI/Testing/*` (`WBP_HUD`, `WBP_AbilityButtons`, `WBP_PVPToggle`,
  `WBP_DebugHealth`, `WBP_GamepadCursorWidget`) and `Content/Game/OnsetHUD` deleted.
  No remaining asset, script, or C++ reference points at them (verified).
- `DemoLevel` moved to `Content/Maps/DemoLevel.umap` (package `/Game/Maps/DemoLevel`);
  the root `Content/DemoLevel.umap` stub is deleted. All references now use
  `/Game/Maps/DemoLevel`: `Config/DefaultEngine.ini` (ServerDefaultMap / EditorStartupMap /
  TransitionMap), `Config/DefaultGame.ini` (MapsToCook), `OnsetPlayerController.cpp:864,891`
  (travel URLs), `Test_All.ps1:38`.
- Import ability icon art if you have it, and assign `AbilityIcon` on `GA_AoE` / `GA_Cone` (A1/A2).

> **Note — gamepad cursor:** the Testing cleanup also deleted `WBP_GamepadCursorWidget`, the
> only WBP for the software gamepad cursor (`UGamepadCursorWidget` expects a `Crosshair`
> image child, `GamepadCursorWidget.cpp:58`). Until a replacement WBP is created (e.g.
> `Content/UI/HUD/WBP_GamepadCursorWidget`) and `GamepadCursorWidgetClass` is set on
> `MyOnsetPlayerController`, the gamepad software cursor is inert.

---

## D. Step-by-step

1. [x] Create the `/Game/UI/HUD/` folder.
2. [x] Create the 6 GEs (A4–A9) + `GA_AoE` / `GA_Cone` / `GA_Shadowstep` (A1–A3) in
   `Content/Game/Combat/`.
3. [x] Build the WBP set B1 → B6 in `Content/UI/HUD/` (`WBP_AbilitySlot`, `WBP_AbilityBar`,
   `WBP_PlayerHealthBar`, `WBP_TargetHUD`, `WBP_DamageNumber`, `WBP_HUD`).
4. [x] Cleanup §C (delete orphans; move `DemoLevel` to `Content/Maps/`; unify all
   references on `/Game/Maps/DemoLevel`).
5. [ ] **Set `HUDWidgetClass = WBP_HUD` on `MyOnsetPlayerController`** (B7) — still
   pending. Until set, the runtime falls back to plain `UHUDWidget` (no WBP styling).
6. [ ] Set `AbilityIcon` on `GA_AoE` / `GA_Cone` (A1/A2).
7. Open `DemoLevel` and run the PIE test plan below. Iterate on styling as needed.

---

## E. PIE test plan

- Slots **1 & 2** show the AoE/Cone icons; slots **3 & 4** show the `EmptyIcon`
  placeholder (all slots stay visible; Shadowstep is granted `INDEX_NONE` at
  `OnsetBaseCharacter.cpp:66`).
- Press **1** → AoE fires; slot 1 shows a fill overlay that empties over the GE
  duration (scaled animation, see B1). Press **2** → same for Cone.
- **Click** a slot button with the mouse → same as pressing the key
  (`HandleSlotClicked` → `AbilityLocalInputPressed`).
- Acquire a target → reticle + name + material fill appear above it and follow it;
  damage dealt spawns floating numbers in `DamageNumberLayer`.
- Player takes damage → health bar text updates + damage number appears.
- Target dies → TargetHUD clears itself.
