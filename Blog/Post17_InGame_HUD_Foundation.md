# 🎬 Post 17 — The HUD Rises: Health, Abilities, and Pooled Damage Numbers

*1 commit · Aug 5, 2026 · covering In-Game HUD: Health Bar, Ability Bar, Target Frame, Pooled Damage Numbers*

---

## Introduction

For the entire project, the in-game feedback loop had been held together by debug text and log lines. Damage numbers? None. Health bars? Only in the world space of the NPC AI. Cooldowns? You guessed the timing by feel. Post 16 shipped a real ability kit, and a kit with no readout is a piano with no sheet music — the player is expected to play without seeing the notes.

This commit builds the foundation of the in-game HUD in C++: a container widget that binds to the possessed pawn's Ability System Component, a player health bar, an ability bar with four assignable slots, a target frame, and a pooled damage-number layer. The key architectural decision — which I want to be explicit about up front — is that **the C++ owns all the logic and the Widget Blueprint owns all the looks**. WBP_HUD nests the visual tree; C++ wires it to gameplay.

---

## Technical Exposition

### Why push the HUD from OnPossess

The HUD needs to bind to an ASC, and the ASC lives on the pawn. The natural moment is `OnRep_Pawn` — the client callback that fires when the server's possession replicates down. At that point the client has a valid pawn, the loading screen from zone travel is ready to dismiss, and we can build the HUD in one place:

```cpp
void AOnsetPlayerController::OnRep_Pawn()
{
    Super::OnRep_Pawn();

    if (!IsLocalController()) return;

    APawn* NewPawn = GetPawn();
    if (!NewPawn) return;

    // The client now possesses a pawn — dismiss the loading screen and build the HUD.
    if (UOnsetUISubsystem* UI = GetGameInstance()->GetSubsystem<UOnsetUISubsystem>())
    {
        UI->HideLoadingScreen();
    }

    CreateHUD(NewPawn);
    Server_OnClientPossessed();
}
```

The `IsLocalController()` guard is the important part: this runs on every client, and only the local player should build a HUD. CreateHUD then instantiates the widget class (overridable per project via `HUDWidgetClass`) and binds it to the pawn.

### Attribute-change delegates: the reactive core

Everything in this HUD is event-driven off the ASC's attribute-change delegates. The player health bar subscribes to `Health` and `MaxHealth`; the target frame subscribes to the target's `Health`; the damage numbers fire out of the HUD's own subscriptions to both. This is the same GAS idiom from Post 14's persistence — never poll attributes on tick when the ASC will tell you the moment they change.

---

## Implementation Details

### 1. The HUD container (`UHUDWidget`)

`BindToPlayer` is the single entry point. It caches the controller, pawn, targeting component, and ASC, then hands each sub-widget the references it needs:

```cpp
void UHUDWidget::BindToPlayer(AOnsetPlayerController* InController, AOnsetBaseCharacter* InPawn)
{
    BoundController = InController;
    BoundPawn = InPawn;
    BoundTargeting = InPawn->TargetingComponent;
    PlayerASC = InPawn->AbilitySystemComponent;

    if (PlayerHealthBar)
    {
        PlayerHealthBar->BindToASC(PlayerASC);
    }
    if (AbilityBar)
    {
        AbilityBar->BindToPlayer(InController, PlayerASC);
    }
    if (BoundTargeting)
    {
        BoundTargeting->OnTargetChanged.AddDynamic(this, &UHUDWidget::HandleTargetChanged);
        HandleTargetChanged(BoundTargeting->GetTarget());
    }
    if (PlayerASC)
    {
        PlayerASC->GetGameplayAttributeValueChangeDelegate(UOnsetAttributeSet::GetHealthAttribute())
            .AddUObject(this, &UHUDWidget::HandlePlayerHealthChanged);
    }
}
```

The sub-widgets are `BindWidgetOptional` members — the Widget Blueprint nests them by name, but if one is missing the HUD still works. That's deliberate: it makes the C++ HUD resilient to partial designer setup during iteration.

### 2. Player health bar (`UPlayerHealthBarWidget`)

The health bar subscribes to both Health and MaxHealth delegates and recomputes a 0..1 fill ratio plus a text readout:

```cpp
void UPlayerHealthBarWidget::RefreshHealth()
{
    const float MaxHealth = BoundASC->GetNumericAttribute(UOnsetAttributeSet::GetMaxHealthAttribute());
    const float Health = BoundASC->GetNumericAttribute(UOnsetAttributeSet::GetHealthAttribute());
    const float NewPercent = MaxHealth > 0.0f ? FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f) : 0.0f;

    if (HealthPercent != NewPercent)
    {
        HealthPercent = NewPercent;
        OnHealthPercentChanged(HealthPercent);
    }

    if (HealthText)
    {
        HealthText->SetText(FText::Format(FText::FromString(TEXT("{0} / {1}")),
            FText::AsNumber(FMath::RoundToInt(Health)), FText::AsNumber(FMath::RoundToInt(MaxHealth))));
    }
}
```

`OnHealthPercentChanged` is a `BlueprintImplementableEvent` — the WBP animates its progress bar (or shader fill) off that value. C++ provides the number; the designer provides the motion. Note the clamp: attribute change delegates can fire with out-of-range values mid-gameplay-effect, and the bar must never read outside [0,1].

### 3. Ability bar (`UAbilityBarWidget`)

This was the most interesting widget of the batch, because the slots are not hardcoded. `BuildSlots` creates four slot widgets, then `RebuildSlots` asks the ASC what's actually granted for each input ID:

```cpp
void UAbilityBarWidget::RebuildSlots()
{
    UnregisterCooldownEvents();

    for (FSlotEntry& Entry : Slots)
    {
        Entry.CooldownTag = FGameplayTag();
        Entry.Widget->SetLocked(true);

        if (FGameplayAbilitySpec* Spec = BoundASC->FindAbilitySpecFromInputID(Entry.InputID))
        {
            if (UOnsetGameplayAbility* AbilityCDO = Cast<UOnsetGameplayAbility>(Spec->Ability))
            {
                Entry.Widget->SetAbility(AbilityCDO->AbilityIcon, AbilityCDO->GetPrimaryCooldownTag());
                Entry.CooldownTag = AbilityCDO->GetPrimaryCooldownTag();
            }
        }

        if (Entry.CooldownTag.IsValid())
        {
            CooldownTagHandles.Add(
                BoundASC->RegisterGameplayTagEvent(Entry.CooldownTag, EGameplayTagEventType::AnyCountChange)
                    .AddUObject(this, &UAbilityBarWidget::HandleCooldownTagChanged));
        }
    }
}
```

This is where the ability base class I added in Post 16 earns its keep. `UOnsetGameplayAbility::GetPrimaryCooldownTag()` reads the ability's cooldown tag directly off its cooldown GE, so the bar discovers each ability's cooldown tag without a lookup table:

```cpp
FGameplayTag UOnsetGameplayAbility::GetPrimaryCooldownTag() const
{
    const FGameplayTagContainer* CooldownTags = GetCooldownTags();
    if (CooldownTags && CooldownTags->Num() > 0)
    {
        return CooldownTags->First();
    }
    return FGameplayTag();
}
```

Cooldown fills are driven entirely by tag count changes — when a cooldown tag goes active, the slot starts a fill; when it's removed, the fill ends. The duration is read live from the active cooldown effect so the animation scales correctly:

```cpp
void UAbilityBarWidget::SyncCooldownState(const FGameplayTag Tag, int32 NewCount)
{
    for (const FSlotEntry& Entry : Slots)
    {
        if (Entry.Widget && Entry.CooldownTag == Tag)
        {
            if (NewCount > 0)
                Entry.Widget->StartCooldown(GetCooldownDuration(Tag));
            else
                Entry.Widget->EndCooldown();
            break;
        }
    }
}
```

Slots are also clickable — `HandleSlotClicked` routes the click back through `AbilityLocalInputPressed(InputID)`, which means the on-screen slots are a first-class input path, not just a readout.

### 4. Target frame (`UTargetHUDWidget`)

The target frame tracks whichever actor `UTargetingComponent` currently targets. The targeting component got an `OnTargetChanged` multicast delegate for exactly this purpose — it fires on `SetTarget`, `ClearTarget`, and on `OnRep_CurrentTarget` when a new target replicates down:

```cpp
void UTargetingComponent::OnRep_CurrentTarget()
{
    OnTargetChanged.Broadcast(CurrentTarget);
}

void UTargetingComponent::SetTarget(AActor* NewTarget)
{
    if (!NewTarget) { ClearTarget(); return; }
    if (!IsActorTargetValid(NewTarget)) return;
    CurrentTarget = NewTarget;
    OnTargetChanged.Broadcast(CurrentTarget);
}
```

The HUD's `HandleTargetChanged` does the swap: unbind the previous target's ASC, cache the new one, and bind its Health attribute delegate. The target frame and the damage-number layer both consume the resulting health-change events.

### 5. Pooled damage numbers (`UDamageNumberWidget`)

The damage numbers are the detail I'm happiest with. `UHUDWidget::BuildDamageNumberPool` pre-allocates 64 widget instances into the designer's canvas layer at construct time:

```cpp
for (int32 i = 0; i < MaxDamageNumbers; ++i)
{
    UDamageNumberWidget* DamageNumber = nullptr;
    if (DamageNumberWidgetClass)
    {
        DamageNumber = CreateWidget<UDamageNumberWidget>(OwningPC, DamageNumberWidgetClass);
    }
    if (!DamageNumber) continue;

    if (UCanvasPanelSlot* DamageNumberSlot = DamageNumberLayer->AddChildToCanvas(DamageNumber))
    {
        DamageNumberSlot->SetAnchors(FAnchors(0.0f, 0.0f));
        DamageNumberSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        DamageNumberSlot->SetAutoSize(true);
    }
    DamageNumber->SetVisibility(ESlateVisibility::Collapsed);
    DamageNumberPool.Add(DamageNumber);
}
```

Spawns are round-robin: index advances mod 64, recycling the oldest entry if the pool is saturated. The widget never creates or destroys itself — it starts a float animation, returns to collapsed, and waits to be reused. Allocation cost is paid once, up front:

```cpp
void UDamageNumberWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Elapsed += InDeltaTime;
    const float Alpha = FMath::Clamp(Elapsed / Lifetime, 0.0f, 1.0f);

    FWidgetTransform Transform;
    Transform.Translation = StartPosition + FVector2D(0.0f, -FloatDistance * Alpha);
    SetRenderTransform(Transform);

    NumberText->SetColorAndOpacity(StartColor.CopyWithNewOpacity(1.0f - Alpha));

    if (Alpha >= 1.0f) Deactivate();
}
```

The damage numbers come from the same attribute-change delegates as the bars. When the *player's* Health drops, the HUD spawns a red number over the player (damage taken); when the *target's* Health drops, a gold number spawns over the target (damage dealt). Both funnel through `SpawnDamageNumber`, which projects the world location to screen space.

### 6. The DPI pitfall

The projection code hides a subtle bug I want to flag, because it will bite every UMG HUD eventually. `ProjectWorldLocationToScreen` returns *viewport pixels*, but UMG positions widgets in *layout units* that are scaled by the widget's geometry scale (DPD scaling):

```cpp
FVector2D ViewportPixels;
const bool bProjected = PC->ProjectWorldLocationToScreen(WorldLocation, ViewportPixels, false);
if (bProjected)
{
    const float GeometryScale = GetCachedGeometry().Scale > 0.0f ? GetCachedGeometry().Scale : 1.0f;
    OutScreenPos = ViewportPixels / GeometryScale;
}
```

If you skip the divide, damage numbers drift off their targets as DPI scale changes — and on a high-DPI monitor they can land a full character-width away. Dividing by the cached geometry scale reconciles the two coordinate spaces.

---

## Results & Validation

- **HUD pushed from `OnRep_Pawn`** on the local client; dismissed loading screen + built HUD in one flow
- **Player health bar** — ASC-driven fill + numeric readout, clamped to [0,1]
- **Ability bar** — 4 slots discovered from the ASC by input ID, icons from ability CDOs, cooldown fills from tag events, clickable slots
- **Target frame** — tracks `OnTargetChanged`, binds target ASC Health
- **Damage numbers** — 64-slot pool, round-robin reuse, per-spawn jitter, gold (dealt) / red (taken)
- **DPI correction** — screen projection divided by geometry scale

The loop finally has a readout: you see your health, your cooldowns, who you're targeting, and every hit landing.

---

## Challenges & Solutions

### Widgets shouldn't poll

**Problem:** The naive approach to a HUD is ticking every frame and reading attributes. That's wasted work and a source of jitter — widgets update even when nothing changed.

**Solution:** Everything binds to ASC attribute-change delegates or the targeting component's `OnTargetChanged`. Updates only fire when the underlying value actually changes. The pattern is uniform across all four widgets, which keeps the mental model simple.

### The ability bar shouldn't know your abilities

**Problem:** Hardcoding "slot 1 = AoE" couples the UI to the ability roster and breaks the moment a class grants a different kit.

**Solution:** The bar queries `FindAbilitySpecFromInputID` and reads the cooldown tag off the ability's own CDO. Add an ability with an input ID and it appears in the bar automatically. This is the same data-driven instinct that drove the profile system back in Post 5 — let the data tell the UI what to show.

### The damage pool must not stutter

**Problem:** Creating and destroying a widget per hit would cause garbage-collection hitches under a rapid kill cascade, exactly when the HUD is busiest.

**Solution:** Pre-allocate 64 collapsed widgets once; reuse round-robin. `SpawnDamageNumber` forcibly deactivates the oldest entry if the pool is saturated, so the newest hit always wins the screen.

---

## Reflection & Lessons Learned

The single most valuable decision here was the C++/Blueprint split: **C++ owns logic, WBP owns looks**. Every widget exposes BlueprintImplementableEvents (percent changed, cooldown started/ended, target acquired) and the designer animates those into progress bars, shader fills, and color swaps. That's why a two-line change to a material or a slot layout never requires a code recompile — and it's why the HUD could be iterated visually in Post 18 without touching the C++.

The event-driven architecture is the quiet hero. The ASC's delegate system is designed for exactly this — UMG listening to gameplay — and using it consistently meant the HUD has essentially zero polling. Every widget updates only when its underlying gameplay changed.

The DPI bug is a reminder that UMG has two coordinate spaces and they aren't the same. It took a first-person debugging session with a high-DPI monitor to catch it, and it's the kind of bug that silently looks "slightly off" in every build after.

---

## Forward-Looking Content

| Phase | Status |
|---|---|
| A4.6 — Phase 1 Abilities | ✅ Committed (Post 16) |
| **A6.1 — In-Game HUD Foundation** | ✅ **Committed** |

The HUD exists and it's wired to GAS. What it still needs is polish: the target frame is a static element rather than a world-tracked element, the damage numbers need the DPI correction verified across devices, and the ability bar needs icons that match the abilities. That's the content of Post 18, which also moves the game's content into a proper `Content/Maps` + `Content/Game/Combat` pipeline.

---

*Arc commit: `5b59384`. Related docs: `Docs/Gameplay/UI_System.md`, `Docs/Gameplay/Targeting_System.md`.*
