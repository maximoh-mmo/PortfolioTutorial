# 🎬 Post 26 — The Humanoid Transformation: Modular Appearance Pipelines and 3D Preview Rigs

*14 commits · Oct 20–23, 2026 · covering Humanoid Customization Architecture, SceneCapture2D RenderTarget Rigs, 4:5 Aspect Ratio Framing (UScaleBox), Idle Animation Autoplay, Multi-Slot Character Select Rigs, Pawn Possession Appearance Restoration, Economy & Progression Engine*

---

## Introduction

In the early prototyping phases of an Action RPG, greyboxing with UE default mannequins is standard procedure. Mannequins validate capsule collision, movement speeds, combat animations, and networking. But as Onset matured past core networking, dedicated server packet security, and data-driven abilities, our placeholder mannequin system reached the end of its useful life.

A true RPG relies on **character identity**. Players expect to craft unique avatars—selecting hairstyles, facial features, skin tones, modular armor pieces, and equipment—and see their avatar seamlessly represented everywhere: in character creation, on the character select screen, during live gameplay, and across multiplayer server transitions.

Between October 20 and October 23, 2026, I overhauled Onset's visual identity pipeline across 14 commits:
* Replaced the legacy mannequin setup with a production **Humanoid Appearance System** supporting adult and youth skeletons with modular mesh layers.
* Built an isolated 3D **SceneCapture2D & TextureRenderTarget2D preview rig**, replacing clunky embedded UI viewports.
* Eliminated optical distortion and clipping: implemented eye/chest camera elevation, downward full-body framing, and strict 4:5 aspect ratio preservation with `UScaleBox`.
* Added autoplay idle animations to preview models so characters feel alive before clicking "Enter World".
* Reused the 3D preview rig across all character select slots with fast asynchronous loading.
* Guaranteed appearance recipe and progression restoration upon pawn possession on both client and dedicated server.
* Scaffolded our **Progression & Economy Workbench** to balance equipment tiers, drop curves, and leveling paces.

---

## Technical Exposition

### 1. Isolated RenderTarget Rig vs. In-World Spawning

Rendering a dynamic 3D character inside Slate or CommonUI typically presents two choices:
1. **Spawn in the active gameplay level:** Position the character in front of the active camera and hope level lighting or background props don't interfere.
2. **Dedicated SceneCapture2D Studio:** Spawn the preview character and lighting rig in an isolated, dark sub-world or out-of-bounds space, capture the view into a `UTextureRenderTarget2D`, and stream the texture directly into a Slate brush.

We chose the isolated `SceneCapture2D` approach. This guarantees pristine studio 3-point lighting, consistent background tone, zero interference from gameplay effects, and complete alpha transparency propagation:

```
+-------------------------------------------------------------+
|                     WBP_CharacterCreation                   |
|  +-----------------------+     +-------------------------+  |
|  |   Customization UI    |     |   UScaleBox (4:5 Ratio) |  |
|  | - Hair, Eyes, Skin    |     |  +--------------------+ |  |
|  | - Armor & Outfits     |     |  |   UImage Brush     | |  |
|  +-----------------------+     |  +---------^----------+ |  |
|              |                 +------------|------------+  |
+--------------|------------------------------|---------------+
               | Updates Recipe               | Material Instance
               v                              |
+---------------------------------------------|---------------+
|          AOnsetAppearancePreviewActor       |               |
|  +-------------------------+     +----------+------------+  |
|  | UOnsetAppearanceComp    |     | USceneCaptureComponent|  |
|  | - Modular Skeletal Mesh |     | - Target: RT_Preview  |  |
|  | - Material Instances    |     | - Alpha: Enabled      |  |
|  +-------------------------+     +-----------------------+  |
+-------------------------------------------------------------+
```

### 2. Solving Optical & Body Distortion: The 4:5 Aspect Ratio

One of the subtlest bugs encountered during UI development was character elongation. When rendering a 3D character into a responsive UI layout, Slate's default layout rules stretch image brushes to fill available vertical space. This resulted in characters appearing unnaturally tall, squished, or warped depending on screen resolution.

To solve this permanently, we enclosed the preview image inside a `UScaleBox`:
* Set `Stretch = EStretch::ScaleToFit`.
* Set fixed `4:5` aspect ratio constraints.
* Positioned the camera at eye level (`CameraEyeZ = 160.0f`) with a slight downward pitch (`-4.5f`), giving a flattering, full-body portrait framing without perspective warping.

---

## Implementation Details

### 1. The Humanoid Appearance Component & Recipe Restoration

The player's visual appearance is defined by an immutable, serializable `FOnsetAppearanceRecipe`. When a player connects to a server or possesses a new pawn, `AOnsetPlayerController` pushes this recipe down to the newly possessed character:

```cpp
void AOnsetPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (AOnsetPlayerCharacter* PlayerChar = Cast<AOnsetPlayerCharacter>(InPawn))
    {
        // Restore saved appearance recipe from player state / account cache
        if (PlayerChar->GetAppearanceComponent() && CachedAppearanceRecipe.IsValid())
        {
            PlayerChar->GetAppearanceComponent()->ApplyRecipe(CachedAppearanceRecipe);
        }

        // Re-synchronize GAS attributes and equipped loadouts
        PlayerChar->InitializeAttributesFromPersistence(CachedProgressionData);
    }
}
```

### 2. Autoplaying Idle Animations on Preview Rigs

A static T-pose or reference pose looks dead in a character creator. In commit `0529f59`, we added automated idle animation playback to `AOnsetAppearancePreviewActor`:

```cpp
bool AOnsetAppearancePreviewActor::ConfigureAnimation()
{
    USkeletalMeshComponent* BodyMesh = Appearance ? Appearance->GetBodyMesh() : nullptr;
    if (!BodyMesh)
    {
        return false;
    }

    if (PreviewAnimation.IsValid() || !PreviewAnimation.IsNull())
    {
        UAnimSequence* Anim = PreviewAnimation.LoadSynchronous();
        if (Anim)
        {
            BodyMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
            BodyMesh->PlayAnimation(Anim, bLoopAnimation);
            bPreviewPlaying = true;
            return true;
        }
    }

    return false;
}
```

When changing body presets or hair styles, the idle animation continues cycling smoothly without restarting or snapping bones back to origin.

### 3. Multi-Slot Preview in Character Select

Rather than creating a separate static thumbnail generator, we extended the preview architecture to our `WBP_CharacterSelectScreen` (commit `e1855ce`). Each character slot dynamically instances its own lightweight preview rig, loading the character's exact saved equipment, colors, and level, rendering a live 3D lineup of the player's roster.

---

## Economy & Progression Foundation

Alongside the appearance overhaul, commits `d9fa437`, `c7168ca`, and `4cd8473` introduced the mathematical backbone of Onset's economy and leveling:

* **Item Drop Curves:** Authored probabilistic drop brackets modeled by Pareto and geometric distribution functions, ensuring rare scroll and weapon drops remain meaningful across tiers.
* **Tier Scaling Curves:** Defined normalized baseline attributes for Level 1–50 progression, setting the foundation for defense mitigation formulas:
  $$\text{Mitigation} = \frac{\text{DEF}}{\text{DEF} + K_{\text{DEF}}}$$
* **Progression Workbench Scaffold:** Created an editor simulation tool for balance designers to simulate hours of player kills, calculate average drops, verify XP pace, and spot progression dead-ends before shipping content.

---

## Results & Validation

1. **Seamless Appearance Flow:**
   * Created custom characters in `WBP_CharacterCreation` (skin tones, hair styles, outfits).
   * Confirmed appearance accurately serialized into persistence payloads.
   * Loaded into the game world: the player spawned with 100% fidelity to the creator preview.
2. **Dedicated Server Verification:**
   * Tested client-server possession: client disconnects and reconnects re-applied the exact appearance recipe without client desync or mesh flickering.
3. **Aspect Ratio & Framing:**
   * Resized viewport across 1080p, 1440p, 4K, and ultrawide resolutions; the character preview stayed locked in crisp 4:5 portrait scale without stretching or distortion.

---

## Challenges & Solutions

### Challenge 1: Mesh Yaw Desync with Movement Capsule

**Problem:** In commit `ae21323`, we discovered that while characters faced forward in the preview rig, spawning in the world caused the character mesh to run perpendicular to the movement velocity.

**Solution:** The skeletal mesh component had an inherited -90° yaw offset relative to Unreal's capsule coordinate system. We standardized the base character hierarchy so the skeletal mesh relative rotation strictly aligns with capsule forward vectors, keeping movement and aiming vectors aligned.

### Challenge 2: Transparent Render Target Alpha Artifacts

**Problem:** Using standard translucent materials on the preview image resulted in black halo artifacts around hairstyles and semi-transparent armor fringes.

**Solution:** Updated the scene capture pipeline to propagate linear scene color with separate alpha channel generation, and configured the UI material to use premultiplied alpha blending (`M_CharacterPreview`).

---

## Looking Ahead

With character identity, modular clothing, 3D preview viewports, and progression curves firmly in place, our attention turned to the physical world. In the next post, we dive into **Proving Grounds, AI Leashing, and the MHInteraction Framework**—building our testing colosseum, solving StateTree evade loops, and architecting an independent world interaction plugin.

---

*Arc commits: `01957c6`, `047d992`, `45f1cdd`, `ae21323`, `d9fa437`, `c7168ca`, `e1855ce`, `785c6ec`, `4cd8473`, `a9e95e6`, `edb2e73`, `3385b51`, `0529f59`, `cc82e08`.*
