# 🎬 Post 15 — The Final Form: Character Classes, Loading Screens, and the Polish That Turns a Demo into a Game

*9 commits · Aug 2–4, 2026 · covering Character Classes, Loading Screens, API Access Control Hardening*

---

## Introduction

The game has a working multiplayer loop, persistent accounts, cloud auth, a CommonUI screen stack, and a PostgreSQL-backed account API. But it still has one character — the default Hero — and no loading screen. A game without character variety is a tech demo; a game without loading screens is a preview build.

Sprint A6 delivers the final polish. Nine commits over three days add three character classes (Warrior, Mage, Rogue), a loading screen with progress bar, API access control hardening, and the last round of bug fixes that turn the demo into something you'd actually want to show someone.

---

## Technical Exposition

### The character class system

The character class is a DataAsset (`UCharacterClassData`) that defines the visual and gameplay identity of a character. It's the same DataAsset pattern used for `UVisualProfile` — lightweight, serializable, and replicated by reference.

Each class defines:
- **Mesh** — the skeletal mesh for the character
- **Animation Blueprint** — the anim BP for the class
- **Color palette** — primary/secondary colors for materials
- **Base stats** — health, speed, attack damage
- **Ability set** — the three abilities the class starts with

The `CharacterClass` field on `AOnsetCharacter` is a `TSoftObjectPtr<UCharacterClassData>` — a soft reference that loads the DataAsset on demand. This means the game doesn't load all class DataAssets at startup; it loads only the one the player selected.

### The loading screen

The loading screen is a `UCommonActivatableWidget` (CommonUI, Post 13) that displays during `ServerTravel`. It shows a progress bar and a status message. The progress bar is driven by the engine's `LoadMap` progress delegate; the status message cycles through a set of tips.

The loading screen is pushed onto the `Menu` layer when travel begins and popped when the map loads. This is the same screen-stack pattern from Post 13 — the loading screen is a Menu-layer widget that the `UOnsetUISubsystem` manages.

### API access control hardening

The Lambda function (Post 14) gets a second round of hardening:
- **Rate limiting** — max 10 requests per minute per platform+platform_id
- **Input validation** — all string inputs are validated against a allowlist regex
- **Error masking** — error messages returned to the client are generic ("Request failed"); detailed errors are logged server-side only

---

## Implementation Details

### 1. The `UCharacterClassData` DataAsset

```cpp
UCLASS(BlueprintType)
class UCharacterClassData : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<USkeletalMesh> Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UAnimBlueprint> AnimBlueprint;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor PrimaryColor;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor SecondaryColor;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float BaseHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float BaseSpeed = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float BaseAttackDamage = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<TSoftObjectPtr<UAbilityData>> StartingAbilities;
};
```

Three class DataAssets are authored in the editor: Warrior, Mage, Rogue. Each has a different mesh, anim BP, color palette, and stat spread. The `StartingAbilities` array references ability DataAssets (the same pattern as `UVisualProfile`).

### 2. Applying the class at character creation

When the player selects a class and creates a character, the class DataAsset is stored in the character's save data. On spawn, the character reads the DataAsset and applies it:

```cpp
void AOnsetCharacter::ApplyCharacterClass(const UCharacterClassData* ClassData)
{
    if (!ClassData) return;

    if (ClassData->Mesh.IsValid())
    {
        GetMesh()->SetSkeletalMesh(ClassData->Mesh.LoadSynchronous());
        GetMesh()->SetAnimInstanceClass(ClassData->AnimBlueprint.LoadSynchronous()->GetAnimBlueprintClass());
    }

    PrimaryMaterial->SetVectorParameterValue(TEXT("PrimaryColor"), ClassData->PrimaryColor);
    SecondaryMaterial->SetVectorParameterValue(TEXT("SecondaryColor"), ClassData->SecondaryColor);

    BaseHealth = ClassData->BaseHealth;
    BaseSpeed = ClassData->BaseSpeed;
    BaseAttackDamage = ClassData->BaseAttackDamage;

    for (const auto& AbilitySoftRef : ClassData->StartingAbilities)
    {
        if (UAbilityData* Ability = AbilitySoftRef.LoadSynchronous())
        {
            AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(Ability, 1, 0));
        }
    }
}
```

The mesh, anim BP, and materials are applied on the server. The client receives the replicated visual profile and reapplies the same changes — no visual desync between server and client.

### 3. The loading screen widget

The loading screen is a `UCommonActivatableWidget` with a progress bar and a tip text block:

```cpp
void UOnsetLoadingScreenWidget::NativeOnActivated()
{
    Super::NativeOnActivated();
    ProgressBar->SetPercent(0.0f);
    TipText->SetText(GetRandomTip());
    OnLoadProgressDelegate.BindUObject(this, &ThisClass::HandleLoadProgress);
    UGameplayStatics::GetStreamingLevelManager(this)->OnLevelLoadProgress.Add(OnLoadProgressDelegate);
}

void UOnsetLoadingScreenWidget::HandleLoadProgress(float Progress)
{
    ProgressBar->SetPercent(Progress);
    if (Progress >= 1.0f)
    {
        UOnsetUISubsystem::Get(this)->CloseScreen(this);
    }
}
```

The progress bar is driven by the engine's streaming level manager. The tip text cycles through a set of lore tips stored in a DataTable. The loading screen is pushed onto the `Menu` layer when `ServerTravel` begins and popped when the map finishes loading.

### 4. API access control hardening

The Lambda function gets three new features:

**Rate limiting** — a sliding window counter per `platform+platform_id`:
```python
def check_rate_limit(platform, platform_id):
    key = f"ratelimit:{platform}:{platform_id}"
    current = redis.incr(key)
    if current == 1:
        redis.expire(key, 60)
    if current > 10:
        return False
    return True
```

**Input validation** — all string inputs are validated against an allowlist regex:
```python
PLATFORM_RE = re.compile(r'^[a-zA-Z0-9_-]{1,32}$')
PLATFORM_ID_RE = re.compile(r'^[a-zA-Z0-9_-]{1,64}$')

def validate_input(value, pattern):
    return bool(pattern.match(value))
```

**Error masking** — error messages returned to the client are generic; detailed errors are logged server-side only:
```python
def mask_error(error):
    return {"error": "Request failed"}
```

### 5. The character select UI update

The character select widget now shows the class icon, name, and stats for each class. The player can preview the class before creating a character:

```cpp
void UOnsetCharacterSelectWidget::PreviewClass(const UCharacterClassData* ClassData)
{
    PreviewMesh->SetSkeletalMesh(ClassData->Mesh.LoadSynchronous());
    PreviewAnimBP->SetAnimBlueprintClass(ClassData->AnimBlueprint.LoadSynchronous()->GetAnimBlueprintClass());
    PreviewName->SetText(FText::FromString(ClassData->GetName()));
    PreviewHealth->SetText(FText::AsNumber(ClassData->BaseHealth));
    PreviewSpeed->SetText(FText::AsNumber(ClassData->BaseSpeed));
    PreviewDamage->SetText(FText::AsNumber(ClassData->BaseAttackDamage));
}
```

The preview uses a separate skeletal mesh component that doesn't affect the actual character. The player can cycle through classes with arrow keys before confirming.

---

## Results & Validation

- 3 character classes: Warrior, Mage, Rogue — each with unique mesh, anim BP, colors, stats, and abilities
- `UCharacterClassData` DataAsset pattern — lightweight, serializable, replicated by reference
- Loading screen with progress bar + rotating tips — CommonUI Menu-layer widget
- API access control hardening: rate limiting, input validation, error masking
- Character preview in select UI — arrow keys to cycle, preview mesh + stats
- A6 = 12/12 (100%)

---

## Challenges & Solutions

### The soft object pointer doesn't load in the editor

**Problem:** `TSoftObjectPtr` references to DataAssets don't resolve in the editor if the asset hasn't been loaded. The preview mesh shows nothing.

**Solution:** Call `LoadSynchronous()` in the preview function. This forces the asset to load from disk. The cost is acceptable for a preview — the actual character uses the same pattern but only loads once at spawn time.

### The loading screen doesn't pop on fast loads

**Problem:** On fast loads (local server), the progress bar reaches 100% before the loading screen widget is fully initialized. The widget pops immediately, and the player sees a flash of the loading screen.

**Solution:** Add a minimum display time (2 seconds). The loading screen stays visible for at least 2 seconds regardless of progress. This prevents the flash and gives the player time to read the tips.

### The rate limiter uses Redis but the Lambda doesn't have Redis access

**Problem:** The rate limiter implementation uses Redis, but the Lambda function runs in a VPC without Redis access.

**Solution:** Switch to DynamoDB-based rate limiting. Use a DynamoDB table with `platform+platform_id` as the partition key and a TTL attribute for automatic expiration. This is slower than Redis but works within Lambda's VPC constraints.

### The character preview mesh conflicts with the actual character mesh

**Problem:** The preview mesh and the actual character mesh share the same skeletal mesh component reference. When the player confirms a class, the preview mesh is replaced with the actual mesh, but the reference isn't properly cleared.

**Solution:** The preview uses a dedicated `USkeletalMeshComponent` (`PreviewMesh`) that's separate from the character's mesh. When the player confirms, `PreviewMesh` is set to nullptr and the character's mesh is updated via `ApplyCharacterClass()`.

---

## Reflection & Lessons Learned

The character class system is the simplest and most satisfying feature in the series. It's a DataAsset with a few fields, applied at spawn time, and it immediately makes the game feel like a game instead of a tech demo. The pattern is the same as `UVisualProfile` and `UAbilityData` — lightweight DataAssets that define identity and behavior. The consistency of the pattern across the codebase is the arc's quietest achievement.

The loading screen is the feature that makes the game feel professional. Without it, `ServerTravel` is a black screen with a loading cursor. With it, the player sees a progress bar and lore tips — the difference between a demo and a game. The CommonUI screen-stack pattern makes this trivial: push the loading screen on travel, pop it on load.

The API access control hardening is the feature that makes the demo safe to share. Rate limiting prevents abuse; input validation prevents injection; error masking prevents information leakage. These are the same concerns that any live-service game faces, and they're all addressed in 9 commits.

---

## Forward-Looking Content

| Phase | Status |
|---|---|
| A5 — Multiplayer + Steam | ✅ **100% (42/42)** |
| A5b — Persistence & Account System | ✅ **100% (30/30)** |
| A5c — Auth Extraction & Login Server | ✅ **100% (38/38)** |
| CommonUI UI Migration | ✅ **100% (4/4)** |
| A6 — Character Classes + Final Polish | ✅ **100% (12/12)** |

**The series is complete.** 15 posts, 15 episodes, covering the full arc from repo bootstrap to polished multiplayer ARPG. The game is playable, persistent, multiplayer-ready, and cloud-connected.

---

## Series Retrospective

### What we built

| Post | Episode | Key Feature | Commits |
|---|---|---|---|
| 1 | Repo + Docs Bootstrap | Git history, CMake, README | 192 total |
| 2 | Project Bootstrap + Player Core | A1 player controller, movement, camera | ~6 |
| 3 | Ability + Input + Targeting | A1.4–A1.6 ability pipeline, PvP toggle | ~3 |
| 4 | NPC Spawner + Pooling + Groups | A2 NPC infrastructure, fallback cube saga | ~16 |
| 5 | AI Perception + StateTree + GAS | A3.1–A3.3 StateTree, A4.1 GAS foundation | ~17 |
| 6 | Hit Reactions + Death + Corpses | A4.3–A4.5b combat feedback, death lifecycle | ~9 |
| 7 | Group Assist + Combat Loop + Profile Split | A3.4 + E21/E22 combat loop | ~12 |
| 8 | Chase Fixes + Movement Attribute | E22 correctness + chase tuning | ~7 |
| 9 | Player AI Autoplay | A3.5 Player AI autoplay | ~4 |
| 10 | Threat System + Codebase Audit | A3.6 threat, codebase audit | ~23 |
| 11 | Multiplayer + Steam Sprint A5 | A5 multiplayer, Steam auth, DS build | ~16 |
| 12 | Persistence Sprint A5b | SQLite, SteamID, ServerTravel, PostgreSQL | ~25 |
| 13 | CommonUI UI Migration | Screen stack, input subsystem, UMG migration | ~4 |
| 14 | Cloud Auth + Multi-Server A5c | Lambda, DynamoDB, JWT tokens, two-server | ~14 |
| 15 | Character Classes + Final Polish | Classes, loading screen, API hardening | ~9 |

### The through-line

Every post in this series is about **one thing**: making the game feel like a game, not a tech demo. The player core makes the character feel responsive. The ability pipeline makes combat feel deep. The AI makes the world feel alive. The multiplayer makes the game feel social. The persistence makes death feel meaningful. The UI makes the game feel professional. The classes make the game feel replayable.

The pattern across all 15 posts is the same: a single commit that changes everything, followed by a cascade of fixes that make it work. This is the reality of game development — the big idea is always simple; the execution is always hard.

### What's next

The game is playable, persistent, multiplayer-ready, and cloud-connected. The next steps are:
1. **Content** — more maps, more enemies, more quests
2. **Polish** — VFX, sound, animation polish
3. **Scale** — stress testing with 20+ concurrent players
4. **Release** — packaging, distribution, marketing

The foundation is solid. The rest is iteration.

---

*Arc commits: `b1262f6 → c1cd967`. Related docs: `Docs/Architecture/Architecture Overview.md`, `Planning/Sprint_A6_Final_Polish.md`.*
