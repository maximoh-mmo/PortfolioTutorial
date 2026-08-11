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
- `Source/Onset/Private/Core/OnsetBaseCharacter.cpp:73-95` (default grants in `GrantDefaultAbilities`)

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
**Static top-centered target lifebar.** It does **not** follow the target and there
is **no reticle and no name text** — the reticle is now a **ground decal** spawned on
the target actor (see C6), and the lifebar is anchored once (top-center) by the designer.

Health fill uses the material approach: C++ exposes `TargetHealthPercent`
(BlueprintReadOnly) and calls `OnTargetHealthPercentChanged(Percent)` on change — bind it
to your fill material.

**Skin swapping (Normal / Elite / Boss).** C++ exposes `TargetType` (BlueprintReadOnly,
enum `ETargetType`) and fires two blueprint events on acquisition/clear:

| Event | When | Purpose |
|-------|------|---------|
| `OnTargetAcquired(TargetType)` | a target is set | swap the lifebar skin (e.g. switch the fill material / colors based on the enum) |
| `OnTargetCleared()` | target removed/died | hide the lifebar |

Suggested tree (no bound child widgets needed):

```
Border/Image (root, e.g. lifebar frame; material-driven fill for the bar)
   └─ (optional) labels you style yourself — C++ never touches text here
```

Anchor the root **top-center** in `WBP_HUD`. Do **not** place it inside a
Horizontal/Vertical box.

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
| `TargetHUD` | nested child widget **WBP_TargetHUD** (static; anchored top-center) |
| `DamageNumberLayer` | **Canvas Panel** (the pool of up to 64 WBP_DamageNumber children is added here) |

Suggested tree:

```
Canvas Panel (root)
├─ PlayerHealthBar      (anchored bottom-center)
├─ AbilityBar           (anchored bottom-center, above health bar)
├─ TargetHUD            (anchored top-center; static, no tick repositioning)
└─ DamageNumberLayer    (full-screen canvas)
```

> `TargetHUD` is **never repositioned at runtime** anymore. It lives exactly where the
> designer anchors it; `TargetHUDWidget.cpp` only hides it (Collapsed) when there is no
> target. Remove any old "reposition every tick" logic you see in the WBP.

### B7. Wire-up: `Content/Input/MyOnsetPlayerController` — **DONE**
`MyOnsetPlayerController.uasset` → **HUD → `HUDWidgetClass` = `WBP_HUD`**.
**Status: done** — verified the asset now references `WBP_HUD`, so `CreateHUD`
instantiates the styled WBP tree (no longer the plain `UHUDWidget` fallback). The
GameMode loads this controller (`OnsetGameModeBase.cpp:34-38`) and `CreateHUD`
instantiates the class (`OnsetPlayerController.cpp:289-310`).

---

## C. Reticle decal + cleanup

### C6. Target reticle decal material (required for the ground reticle)

The targeting reticle is now a **world-space decal** rendered under the target
(`AOnsetBaseCharacter::TargetReticleDecal`), scaled to the enemy's capsule size.
It is shown/hidden by `SetTargetReticle(bool)` which the target HUD calls when the
player's target changes.

| Asset | Location | Parent | Required settings |
|-------|----------|--------|-------------------|
| `M_TargetReticle` | `Content/Materials/` | `Material` | Deferred Decal domain; blend a ring/arc using the `reticule` texture; scale-independent so it reads at any capsule size |

The C++ default loads `/Game/Materials/M_TargetReticle.M_TargetReticle`
(`OnsetBaseCharacter.cpp:41`), so the decal renders out of the box; if the asset is
missing it falls back to `/Engine/EngineDebugMaterials/DefaultDeferredDecalMaterial`.
Assign `M_TargetReticle` to the character(s) you target (base
`AOnsetBaseCharacter.TargetReticleMaterial`, or override per enemy/boss class).

> **Multiplayer note:** the decal is a client-side visual. `SetTargetReticle` toggles
> visibility locally (component visibility is not replicated), so only the targeting
> player sees the reticle under their own target.

### C7. Cleanup (previously §C)

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

> **Reticle cleanup:** the old screen-space reticle widgets/styling are now unused —
> `ReticleBorderStyle`, `ReticleTextStyle`, and the `reticule` texture (now only used by
> the decal material). The `ReticleBorder` / `NameText` bindings were removed from
> `UTargetHUDWidget`; delete those widgets from `WBP_TargetHUD` when you re-skin it.

---

## D. Step-by-step

1. [x] Create the `/Game/UI/HUD/` folder.
2. [x] Create the 6 GEs (A4–A9) + `GA_AoE` / `GA_Cone` / `GA_Shadowstep` (A1–A3) in
   `Content/Game/Combat/`.
3. [x] Build the WBP set B1 → B6 in `Content/UI/HUD/` (`WBP_AbilitySlot`, `WBP_AbilityBar`,
   `WBP_PlayerHealthBar`, `WBP_TargetHUD`, `WBP_DamageNumber`, `WBP_HUD`).
4. [x] Cleanup §C7 (delete orphans; move `DemoLevel` to `Content/Maps/`; unify all
   references on `/Game/Maps/DemoLevel`).
5. [x] Rework `WBP_TargetHUD` to the static top-centered lifebar (B4): remove
   `ReticleBorder` / `NameText`, bind `OnTargetAcquired` / `OnTargetCleared` /
   `OnTargetHealthPercentChanged`, anchor top-center.
6. [x] Create `M_TargetReticle` (C6) — exists at `Content/Materials/` and is assigned
   as the C++ default on `AOnsetBaseCharacter`.
7. [x] **Set `HUDWidgetClass = WBP_HUD` on `MyOnsetPlayerController`** (B7) — done;
   verified the asset references `WBP_HUD`.
8. [ ] Set `AbilityIcon` on `GA_AoE` / `GA_Cone` (A1/A2).
9. Open `DemoLevel` and run the PIE test plan below. Iterate on styling as needed.

---

## E. PIE test plan

- Slots **1 & 2** show the AoE/Cone icons; slots **3 & 4** show the `EmptyIcon`
  placeholder (all slots stay visible; Shadowstep is granted `INDEX_NONE` at
  `OnsetBaseCharacter.cpp:95`).
- Press **1** → AoE fires; slot 1 shows a fill overlay that empties over the GE
  duration (scaled animation, see B1). Press **2** → same for Cone.
- **Click** a slot button with the mouse → same as pressing the key
  (`HandleSlotClicked` → `AbilityLocalInputPressed`).
- Acquire a target → a **ground decal reticle** appears under the target (scaled to its
  capsule) and the **static top-center lifebar** shows the target's health fill (skin
  chosen from `TargetType`). Switching targets moves the decal and refreshes the lifebar.
  Damage dealt spawns floating numbers in `DamageNumberLayer`.
- Player takes damage → health bar text updates + damage number appears.
- Target dies → its decal reticle disappears and TargetHUD clears itself (lifebar hides).
- Verify the lifebar stays fixed top-center (it does **not** follow the target).
