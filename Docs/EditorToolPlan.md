# EditorToolPlan.md — Ability Editor Tool

## 1. Goal
A low/no-code, in-engine ability editor for designers: add/edit/remove abilities of the
existing three types (single-target, AoE, cone) with a comfortable form UI and **no new
C++ per ability**. Introduces the **Slow** vs **Snare** distinction and a proper
**damage execution pipeline**.

Complements `docs/UI_ASSET_CHECKLIST.md` (the HUD/asset work this plan depends on).

## 2. Mechanic definitions
- **Snare** = reduce *movement speed*. Modifier on `MovementSpeed`
  (`UOnsetMovementAttributeSet`, already wired to `MaxWalkSpeed` via `PostAttributeChange`).
- **Slow** = reduce *attack speed* = increase *cooldown duration*. New replicated
  attribute `CooldownMultiplier` (default `1.0`); cooldowns applied as
  `CooldownSeconds × CooldownMultiplier`. A slowed target attacks less often.
  - Enemies automatically benefit: `EnemyEngageTask.cpp:165-169` gates attacks on the
    `Cooldown.BasicAttack` tag, which now lasts longer.
  - Caveat: the player's passive `AutoAttackInterval` timer (`OnsetPlayerController.h:160`)
    is not attribute-driven; scaling it is an optional Phase 5 follow-up.

## 3. Data model — `DT_Abilities`
Location: `Content/Game/Combat/DT_Abilities`.

```cpp
UENUM(BlueprintType)
enum class EOnsetAbilityType : uint8 { SingleTarget, AoE, Cone };

UENUM(BlueprintType)
enum class EOnsetAbilityEffectType : uint8 { Damage, Snare, Slow };  // extensible: Heal, Buff, ...

USTRUCT(BlueprintType)
struct FOnsetAbilityEffect
{
    EOnsetAbilityEffectType Type;
    float Magnitude;                 // Damage: amount | Snare: move-speed mult (<1) | Slow: cooldown mult (<1)
    float Duration = 0.f;            // 0 = instant (Damage); >0 = debuff window (Snare/Slow)
    FGameplayTag DamageTypeTag;      // Damage.Physical | Damage.Magical (Damage effects only)
};

USTRUCT(BlueprintType)
struct FOnsetAbilityDefinition : public FTableRowBase
{
    FText DisplayName;
    TSoftObjectPtr<UTexture2D> AbilityIcon;
    EOnsetAbilityType AbilityType;
    int32 InputID = INDEX_NONE;      // -1 = unbound/passive
    float CooldownSeconds = 1.f;
    float AttackRange = 300.f;       // SingleTarget
    float Radius = 300.f;            // AoE
    float ConeRange = 500.f;         // Cone
    float ConeHalfAngle = 90.f;      // Cone
    TSoftObjectPtr<UAnimMontage> Montage;  // optional
    float DamageTime = 0.3f;         // optional montage hit time
    TArray<FOnsetAbilityEffect> Effects;
};
```
RowName = stable ability ID (e.g. `AoE`, `SlowStrike`). Matches the existing
`FOnsetCharacterClassInfo` DataTable pattern (`OnsetClassInfoTypes.h`).

## 4. Damage execution pipeline (from day one)
- **`UOnsetDamageExecution : UGameplayEffectExecutionCalculation`** (mirrors the existing
  `ExecCalc_Stagger` pattern). Reads SetByCaller magnitudes for `Damage.Physical` and
  `Damage.Magical` from the spec, sums them, applies the mitigation formula, and outputs an
  additive (negative) modifier on `Health`.
- **Formula**: v1 = pass-through (`damage × 1.0`) with an **invulnerability gate**
  (`TAG_State_Invulnerable` → 0 damage), centralizing damage negation in one place.
  Extensible by adding captured attributes (future `Armor`/`Resistance` on a combat
  attribute set) — the SetByCaller seam never changes.
- **`GE_GenericDamage`**: Instant; Health modifier magnitude = SetByCaller
  `Damage.Physical` / `Damage.Magical`; executed by `UOnsetDamageExecution`.
  **No damage value is ever baked into a GE asset.**
- Damage numbers + target HUD are unaffected (they read the Health delta via delegates).

## 5. Other shared template GEs
- `GE_GenericSnare` — Has Duration; `MovementSpeed` modifier = SetByCaller `MoveSpeedMod`
  (reuse the name from `OnsetMovementSpeedModifierEffect.cpp:13`).
- `GE_GenericSlow` — Has Duration; `CooldownMultiplier` modifier = SetByCaller
  `CooldownRateMod` (MultiplyCompound).
- `GE_GenericCooldown` — Has Duration; grants **no** tags by default (tag added dynamically
  at apply time).

## 6. Runtime — `UOnsetGA_Generic` + support
- `ActivateAbility`: resolve the definition row from the spec's `DynamicAbilityTags`
  (ability-ID tag) via a new `UOnsetAbilityLibrary` helper; dispatch on `EOnsetAbilityType`:
  - SingleTarget → existing `UOnsetGA_BasicAttack` flow (range check, optional montage +
    `DamageTime`, single-target apply).
  - AoE → existing sphere-overlap flow (`OnsetGA_AoE.cpp:76-133`, PvP filter kept).
  - Cone → existing directional cone flow.
  Apply each effect with SetByCaller magnitudes and, for debuffs, `SetDuration`.
- `ApplyCooldown` override (base `UOnsetGameplayAbility`): look up `CooldownSeconds` +
  cooldown tag; build `GE_GenericCooldown` spec → add `Cooldown.<RowName>` to
  `DynamicGrantedTags` → `SetDuration(CooldownSeconds × CooldownMultiplier, true)` → apply.
- New `UOnsetCombatAttributeSet` with replicated `CooldownMultiplier`, added as a default
  subobject in `AOnsetBaseCharacter` ctor (same pattern as `OnsetBaseCharacter.cpp:24-25`).
- Existing `UOnsetGA_BasicAttack` / `UOnsetGA_AoE` / `UOnsetGA_Cone` classes + their BPs
  remain as legacy/templates.

### Cooldown tags
Per-definition `Cooldown.<RowName>` (e.g. `Cooldown.AoE`), registered natively in
`OnsetGameplayTags.h`. Existing 4 cooldown tags stay valid for legacy abilities.

## 7. Ability bar + loadout
- `GrantDefaultAbilities` (`OnsetBaseCharacter.cpp:48-66`) reads `DT_Abilities`: for each
  row with `InputID >= 0`, grant a `UOnsetGA_Generic` spec with its ID tag in
  `DynamicAbilityTags` and that InputID.
- `UAbilityBarWidget::RebuildSlots` (`AbilityBarWidget.cpp:164-179`) resolves icon +
  cooldown tag from the **definition row** (via the spec's ID tag) instead of the GA CDO,
  keeping slots data-driven.

## 8. Editor tool (Editor Utility Widget)
- **Required plugins** (edit `Onset.uproject`): `Blutility` + `EditorScriptingUtilities`
  (both editor-only, engine-provided).
- **UI composition**:
  - Left: ability list (from `DT_Abilities`).
  - Right: a **`PropertyView` bound to the selected row** wrapped in a transient UObject —
    the details panel auto-generates the form (asset pickers for icon/montage, enums for
    type/effect, number fields). No hand-built fields.
  - Buttons: **Add** (new row with defaults), **Delete** (remove row),
    **Save** (`MarkPackageDirty()` + save `DT_Abilities`).
- **C++ support**: `UOnsetAbilityEditorWidget` (EUW base class, editor-only) exposing
  `UFUNCTION`s: `LoadDefinitions`, `SaveDefinition(RowName, Row)`, `AddDefinition`,
  `DeleteDefinition(RowName)`, `RefreshFromTable`.
- Optional: "grant to PIE pawn" test button (`TargetingComponent::SetTarget` exists for
  target setup); toolbar entry via a small `OnsetEditor` module.

## 9. Phases & verification
1. **Cooldown plumbing** — `UOnsetCombatAttributeSet` (`CooldownMultiplier`, replicated),
   `GE_GenericSlow`, `ApplyCooldown` SetDuration override. Verify: applying a slow to an
   enemy extends their cooldown.
2. **Damage pipeline** — `UOnsetDamageExecution` + `GE_GenericDamage` + invuln gate.
   Verify: damage + damage numbers still correct; invulnerable targets take 0.
3. **Data-driven runtime** — `FOnsetAbilityDefinition`, `UOnsetGA_Generic`,
   `GE_GenericSnare`/`GE_GenericCooldown`, `UOnsetAbilityLibrary`, tag-based row
   resolution, data-driven `GrantDefaultAbilities`, bar row-based slot resolution.
   Verify parity: AoE/Cone/single-target behave identically to today; slots 1/2 show row
   icons; cooldown bars fill.
4. **The tool** — enable plugins; EUW + PropertyView + Add/Delete/Save; build
   `DT_Abilities` with the demo loadout (AoE→1, Cone→2). Verify add/edit/delete persists
   to the table and shows up in PIE.
5. **Extras (optional)** — player `AutoAttackInterval` driven by `CooldownMultiplier`;
   more effect types; assignment/unlock menu; ExecCalc mitigation formula (armor/resist).

## 10. Dependencies & notes
- Requires the **HUD asset work** first (6 WBPs + `MyOnsetPlayerController.HUDWidgetClass`
  per `docs/UI_ASSET_CHECKLIST.md`) — unaffected by this plan.
- `Cooldown.AoE/Cone/Shadowstep` tags already registered (`OnsetGameplayTags.cpp:10-13`).
- The 6 per-ability GEs + `GA_AoE`/`GA_Cone` BPs in `docs/UI_ASSET_CHECKLIST.md` §A become
  **optional/legacy** once Phase 3 lands; the checklist is updated then.
- SetByCaller is already proven in-project (`OnsetStateTreeTask.cpp:100`,
  `OnsetGA_Shadowstep.cpp:107`).
- Damage values live in exactly one place: `DT_Abilities` rows. Never bake damage into GE
  assets (this is the "standard default damage value" approach and does not fit the
  data-driven tool).
