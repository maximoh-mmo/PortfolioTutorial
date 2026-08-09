# 🎬 Post 18 — HUD v2: Ground Reticles, Target Skins, and the Content Pipeline

*2 commits · Aug 7–8, 2026 · covering HUD v2: Targeting, Damage Numbers, Cooldowns, Ground Reticle Decal, Content Pipeline*

---

## Introduction

Post 17 built a HUD and called it a foundation. It was — but foundations are for building on, and the v2 commit (plus a polish pass the next day) tore up some of the v1 assumptions. The screen-space reticle I'd sketched for the target frame? Gone, replaced by a ground decal that actually sits under the target's feet. The target lifebar that was supposed to float over the world? Anchored static in the HUD instead, with skins for Normal/Elite/Boss targets. The ability slots got real icons driven by the ability Blueprints, and the whole content tree got reorganized out of a flat `Content/` root into `Content/Maps` and `Content/Game/Combat`.

This post is less about "new systems" and more about the second pass — the one where you admit your first UI instinct was wrong, restructure for how the game actually feels, and fix the coordinate-space bugs that only show up at high DPI.

---

## Technical Exposition

### Two target-feedback philosophies

In v1 (Post 17) the target frame was a floating element meant to track the enemy in world space. Screen-space reticles are the classic ARPG approach — the HUD pins a marker over the target's head. They look great in screenshots and feel wrong in practice: they jitter, they lag the target's movement through a re-projection, and they compete with the player's actual attention on the character.

v2 inverts the philosophy. **The marker moves into the world** — a decal component on the target character itself, so it's rendered by the game world at the character's feet, perfectly stable, with no projection at all. And **the lifebar stays in the HUD** — anchored once, top-center, reading the target's health through the existing attribute delegates. The two halves of "target feedback" split along the axis that makes each one stable: world-owned visuals for position, screen-owned visuals for information.

### Why the content restructure

The commit moved `DemoLevel.umap` into `Content/Maps/` and the ability Blueprints into `Content/Game/Combat/`. Two reasons. First, a flat `Content/` root was already drowning — the combat GEs alone needed 9+ files. Second, the C++ hardcodes several asset paths via `ConstructorHelpers::FObjectFinder` and `LoadObject`, so the layout had to be decided once and committed to. The checklist doc I wrote alongside (`UI_ASSET_CHECKLIST.md`) now pins those paths so they're a contract, not an accident.

---

## Implementation Details

### 1. The ground reticle decal

Each character now owns a `UDecalComponent` in its constructor:

```cpp
TargetReticleDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("TargetReticleDecal"));
TargetReticleDecal->SetupAttachment(GetCapsuleComponent());
TargetReticleDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
TargetReticleDecal->SetRelativeLocation(FVector(0.0f, 0.0f, -(GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 60.0f)));
TargetReticleDecal->DecalSize = FVector(120.0f, 50.0f, 50.0f);
TargetReticleDecal->SetHiddenInGame(true);
```

It starts hidden. `SetTargetReticle(bool)` shows or hides it, and while visible, the character re-projects it every tick so it hugs terrain:

```cpp
void AOnsetBaseCharacter::UpdateTargetReticle()
{
    const FVector TraceStart = GetCapsuleComponent()->GetComponentLocation();
    const FVector TraceEnd = TraceStart - FVector(0.0f, 0.0f, 2000.0f);

    FHitResult Hit;
    FCollisionQueryParams QueryParams(FName(TEXT("TargetReticleTrace")), false, this);
    QueryParams.bTraceComplex = false;

    if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams) && Hit.bBlockingHit)
    {
        const float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
        constexpr float ReticleDepth = 80.0f;

        // Decals project along local +X, so point +X against the surface normal.
        const FRotator SurfaceRotation = FRotationMatrix::MakeFromX(-Hit.ImpactNormal).Rotator();

        // Slight margin above the surface so the ground plane is strictly inside the box.
        const FVector ReticleCenter = Hit.ImpactPoint - Hit.ImpactNormal * (ReticleDepth - 5.0f);
        TargetReticleDecal->SetWorldLocationAndRotation(ReticleCenter, SurfaceRotation);
        TargetReticleDecal->DecalSize = FVector(ReticleDepth, Radius * 1.4f, Radius * 1.4f);
    }
}
```

A couple of details here are the difference between "works" and "looks right":
- Decals project along **local +X**, so the surface rotation is built from the *negated* normal (`MakeFromX(-ImpactNormal)`).
- The decal box has depth, so it's sunk by `ReticleDepth - 5` to keep its top face at the surface without poking into the character's capsule.
- The radius is derived from the character's capsule, so a boss with a fat capsule gets a bigger ring for free.

### 2. Per-character target type

Each character carries a `TargetType` enum (Normal / Elite / Boss), set by the designer in the editor:

```cpp
UENUM(BlueprintType)
enum class ETargetType : uint8
{
    Normal  UMETA(DisplayName = "Normal"),
    Elite   UMETA(DisplayName = "Elite"),
    Boss    UMETA(DisplayName = "Boss")
};
```

When the target HUD acquires a target, it reads that type and hands it to the Blueprint so the lifebar can swap skins:

```cpp
void UTargetHUDWidget::SetTarget(AOnsetBaseCharacter* InTarget)
{
    // ... unbind previous target ASC, retire its ground reticle ...
    TrackedTarget = IsValid(InTarget) ? InTarget : nullptr;

    if (TrackedTarget)
    {
        TrackedTarget->SetTargetReticle(true);
        TargetType = TrackedTarget->TargetType;
        // ...
        OnTargetAcquired(TargetType);
    }
    else
    {
        TargetType = ETargetType::Normal;
        OnTargetCleared();
    }
    RefreshHealth();
    SetVisibleState(TrackedTarget != nullptr);
}
```

`OnTargetAcquired(ETargetType)` is a `BlueprintImplementableEvent` — the WBP_TargetHUD responds by swapping its lifebar's fill material/colors for that tier. The decal also gets a per-class material override (`TargetReticleMaterial`), so an elite can have a different-colored ring than a grunt.

### 3. The static target lifebar

The lifebar is now explicitly **static** — designer-anchored once (top-center of WBP_HUD), never following the target. The header documents this as a deliberate contract:

> *The widget does NOT move or follow the target — the designer anchors it once. It is used for every enemy/boss; the WBP picks a "skin" based on the exposed TargetType when a target is acquired. The screen-space reticle is gone: a ground decal on the target actor marks the current target instead.*

The health fill is a shader material in WBP_TargetHUD driven by `TargetHealthPercent`, updated through the target ASC's Health attribute delegate — the same `RefreshHealth` pattern from v1. The only change to the logic is that the widget self-clears when its tracked target dies:

```cpp
void UTargetHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    if (!IsValid(TrackedTarget) || !TrackedTarget->IsAlive())
    {
        if (TrackedTarget)
        {
            SetTarget(nullptr);
        }
        else
        {
            SetVisibleState(false);
        }
    }
}
```

Kill the target, the lifebar vanishes — no projection, no chasing a corpse.

### 4. Data-driven ability slots

The ability bar's slot lookup went from "ask the ASC by input ID" to "ask the ASC, then read the ability's Blueprint for its icon and cooldown tag." `RebuildSlots` now pulls both from the ability CDO:

```cpp
if (UOnsetGameplayAbility* AbilityCDO = Cast<UOnsetGameplayAbility>(Spec->Ability))
{
    Entry.Widget->SetAbility(AbilityCDO->AbilityIcon, AbilityCDO->GetPrimaryCooldownTag());
    Entry.CooldownTag = AbilityCDO->GetPrimaryCooldownTag();
}
```

The icons come from `UOnsetGameplayAbility::AbilityIcon` — a `TSoftObjectPtr<UTexture2D>` set per ability in its Blueprint. Slots render locked/empty when no ability is granted for that input ID. The designer's `UI_ASSET_CHECKLIST.md` pins exactly which Blueprint property feeds which slot.

### 5. DPI/geometry-scale alignment fixes

The v1 HUD had a latent bug I'd flagged: `ProjectWorldLocationToScreen` returns viewport pixels, but UMG positions widgets in layout units. v2 fixed it for real, dividing by the widget geometry scale:

```cpp
const float GeometryScale = GetCachedGeometry().Scale > 0.0f ? GetCachedGeometry().Scale : 1.0f;
OutScreenPos = ViewportPixels / GeometryScale;
```

This is the fix that makes damage numbers land on their targets at 150% DPI instead of drifting a character-width away. I left a one-time diagnostic log behind to catch regressions — a `[HUDDiag]` line that logs the first projection, viewport size, DPI scale, and geometry scale together.

---

## Results & Validation

- **Ground reticle** — decal under the target's feet, re-projected per frame to follow terrain, radius scaled to the character's capsule, per-class material override
- **Target skins** — `ETargetType` drives lifebar skin + decal material on target acquisition
- **Static lifebar** — designer-anchored, self-clears on target death, no projection cost
- **Ability slots** — icons + cooldown tags read from ability Blueprints via `AbilityIcon`
- **Content pipeline** — `Content/Maps` + `Content/Game/Combat`, paths pinned in the UI asset checklist
- **DPI fix** — damage numbers align at high DPI via geometry-scale division

The target feedback loop finally looks finished: a stable ring under the enemy, a clean lifebar at the top of the screen, and damage numbers that land where the hits land.

---

## Challenges & Solutions

### The screen-space reticle was the wrong instinct

**Problem:** v1's plan was a HUD-pinned marker projected over the target. In practice it jittered, lagged the target through re-projection, and read as noise over the action.

**Solution:** Move the marker into the world as a decal owned by the target actor. It's rendered in world space, so it's perfectly stable and costs nothing to project. The information (health) stays in the HUD where it belongs. This split — world visuals for position, screen visuals for information — is now the guiding rule for all targeting UI.

### Decals have a projection axis, not a surface

**Problem:** Slapping a decal onto a rotation built from `+ImpactNormal` made the ring render sideways or through the floor.

**Solution:** Decals project along local +X, so the rotation has to come from `-ImpactNormal`, and the decal must be sunk by its own depth so its top face sits at the surface. Both details are documented in `UpdateTargetReticle` because they're non-obvious and easy to "fix" wrongly.

### Content paths became a contract

**Problem:** The C++ hardcodes asset paths (`/Game/Game/Combat/GA_AoE`, etc.) for the ability Blueprints. A designer moving an asset would silently break the lookup and fall back to the C++ class with no icon.

**Solution:** The `UI_ASSET_CHECKLIST.md` doc pins every path and every bind name as a requirement, and `GrantDefaultAbilities` keeps the C++ class fallback so the game still runs (just without icons) if an asset is missing. The contract is explicit; the failure is graceful.

---

## Reflection & Lessons Learned

The most important lesson of v2: **your first UI instinct is a hypothesis, not a plan.** The screen-space reticle worked on paper and failed on screen, and I didn't realize how badly until I saw the ground decal version side by side. The refactor wasn't expensive — the decal already existed on the character class, the delegates already existed on the ASC — because the underlying systems were structured around the right abstractions the first time.

The `ETargetType` enum is a tiny piece of code with outsized value. It's the seam where the designers can skin bosses differently without touching gameplay code, and it's the same data-driven instinct that keeps showing up across this project — let the asset tell the UI what to show.

The content restructure is the least glamorous commit in this post and arguably the most important for the project's future. A `Content/` root with 50 flat assets is where games go to die; `Content/Maps` + `Content/Game/Combat` is a structure that scales. Getting the paths pinned in a checklist doc means the structure outlives my memory of where everything lives.

---

## Forward-Looking Content

| Phase | Status |
|---|---|
| A4.6 — Phase 1 Abilities | ✅ Committed (Post 16) |
| A6.1 — In-Game HUD Foundation | ✅ Committed (Post 17) |
| **A6.1 — HUD v2 (Targeting, Reticle, Skins)** | ✅ **Committed** |

The HUD is now feature-complete for the current ability set. The obvious next candidates: the ability editor tool documented in `Docs/EditorToolPlan.md` (a data-driven `DT_Abilities` table replacing hardcoded ability grants), and the demo-loop work that stitches everything into a playable wave. But first, a bug surfaced that was eating player data — the kind that only shows up across a full login/logout cycle. That's Post 19.

---

*Arc commits: `5361653`, `2e61a43`. Related docs: `Docs/UI_ASSET_CHECKLIST.md`, `Docs/EditorToolPlan.md`.*
