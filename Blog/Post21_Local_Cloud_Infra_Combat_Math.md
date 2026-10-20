# 🎬 Post 21 — Local Cloud Infrastructure, Healing, and the Combat Formula Engine

*11 commits · Aug 16–17, 2026 · covering DynamoDB Local Dev Workflow, Capacity Metrics, Ally/Heal Targeting, DoT/HoT Ticks, Combat Formula Spec, Equipment Stat Scaling*

---

## Introduction

As Onset grew from a prototype into a living ARPG, two distinct bottlenecks slammed the brakes on development at the exact same time.

First was what I call the **Cloud Dev Tax**. Back in Post 14, I built the cloud account system (`AccountApi`) on top of AWS DynamoDB. It worked great in staging, but daily local development was painful. Every integration test made real HTTPS calls to AWS, required valid cloud credentials on every development workstation, and failed immediately if my internet had a hiccup. Running test suites took minutes instead of seconds.

Second was the **flatness of combat**. Our characters could swing swords and cast data-driven AoEs, but the math under the hood was essentially `Health -= Damage`. There were no critical hits, no armor mitigation curves, no healing spells, no damage-over-time (DoTs), and no way for a paladin or priest to target an injured teammate without hitting an enemy.

Over two intense days and eleven commits, I resolved both: I brought the entire cloud infrastructure local with Docker Compose and DynamoDB Local, and I implemented the full 7-phase ARPG combat formula engine—complete with ally healing, periodic ticks, diminishing-returns crowd control, and equipment stat scaling.

---

## Technical Exposition

### 1. The Local-First Cloud Paradigm

Developing against live cloud resources for inner-loop testing is an anti-pattern. You run into rate limits, pay unnecessary pennies, and endure 80ms latency on every assertion.

```
Local Development Mode:
[ Unreal PIE / Test_All.ps1 ] 
         |
         v (HTTP / localhost:3000)
[ AccountApi Node Service ]
         |
         v (DynamoDB Protocol / localhost:8000)
[ Docker Container: amazon/dynamodb-local ]
```

By introducing `amazon/dynamodb-local` into a local `docker-compose.yml`, the entire account service boots in under a second. `AccountApi` detects the local environment, switches endpoint targets, provisions tables via an idempotent setup script, and records capacity metrics locally. The entire test runner (`Test_All.ps1`) now runs in **local mode by default**, with a `-Cloud` switch reserved strictly for pre-deployment staging verification.

### 2. Targeting Inversion: Friend vs. Foe

Before this sprint, all targeting logic assumed hostility:
```cpp
// Old assumption: if you target it, you want to hurt it
if (TargetActor && TargetActor != GetAvatarActorFromActorInfo())
{
    ApplyDamage(TargetActor);
}
```

Support abilities break this assumption completely. A "Lesser Heal" or "Prayer of Mending" requires targeting an ally—or even targeting the *target of your hostile target* (Target-of-Target healing in MMO raid design).

I updated the targeting architecture with three core primitives:
1. **`EOnsetAbilityType::Self`:** Instantly resolves to the caster without requiring target selection.
2. **`bFriendly` Per-Effect Flag:** Allows individual effects within a single ability row to discriminate targets (e.g., a Holy Nova that damages enemies while simultaneously healing allies).
3. **Unified `ShouldAffectActor()` Predicate:** A centralized class-derived team determination so enemies never cleave each other, and players never accidentally nuke party members.

### 3. The 7-Phase Combat Formula Engine

Damage is no longer a magic constant. It is calculated by a dedicated Gameplay Effect Execution (`UOnsetDamageExecution`) implementing a formal ARPG mathematical model:

$$\text{Damage} = (\text{Base} + \text{Power} \times \text{Scaling}) \times \text{CritMultiplier} \times \left(1 - \frac{\text{Armor}}{\text{Armor} + 100 \times \text{AttackerLevel}}\right) \times \text{ElementMult}$$

* **Attributes:** Primary attributes (STR, AGI, INT, VIT, LUK) derive secondary combat ratings (Physical Power, Haste/Cooldown Rate, Armor, Magic Resist, Crit Chance).
* **Element Chart:** A $5\times 5$ matrix mapping Physical, Fire, Ice, Lightning, and Holy/Shadow affinities.
* **Crowd Control Diminishing Returns:** Chaining stuns or freezes on the same actor exponentially reduces subsequent CC durations via `UOnsetCCDiminishingComponent`.

---

## Implementation Details

### 1. Local DynamoDB Client Routing (`AccountApi/src/db/client.js`)

In the Node backend, the DynamoDB client dynamically branches based on the environment configuration, pointing to the local Docker container without code changes:

```javascript
const { DynamoDBClient } = require('@aws-sdk/client-dynamodb');
const { DynamoDBDocumentClient } = require('@aws-sdk/lib-dynamodb');
const config = require('../config');

const clientConfig = {
  region: config.region,
};

// Route to local DynamoDB container when running locally
if (config.isLocal) {
  clientConfig.endpoint = config.dynamoDbEndpoint || 'http://localhost:8000';
  clientConfig.credentials = {
    accessKeyId: 'local',
    secretAccessKey: 'local',
  };
}

const rawClient = new DynamoDBClient(clientConfig);
const docClient = DynamoDBDocumentClient.from(rawClient, {
  marshallOptions: { removeUndefinedValues: true },
});

module.exports = { docClient, rawClient };
```

A companion script (`create-local-table.js`) provisions the `Accounts` table with on-demand capacity and local secondary indexes in 120 milliseconds.

### 2. Periodic Effects: DoTs and HoTs (`UOnsetGA_Generic.cpp`)

To support abilities like *Corruption* (Damage over Time) and *Renew* (Heal over Time), I expanded `FOnsetAbilityEffect` with a `Period` property. When `Period > 0.0f`, `UOnsetGA_Generic` instantiates a periodic gameplay effect:

```cpp
if (EffectDef.Period > 0.0f && EffectDef.Duration > 0.0f)
{
    // Configure periodic GameplayEffect spec
    TSubclassOf<UGameplayEffect> PeriodicClass = (EffectDef.Type == EOnsetAbilityEffectType::Heal) 
        ? UOnsetGenericHealOverTimeEffect::StaticClass() 
        : UOnsetGenericDamageOverTimeEffect::StaticClass();

    FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(PeriodicClass, GetAbilityLevel());
    if (SpecHandle.IsValid())
    {
        SpecHandle.Data->SetDuration(EffectDef.Duration, false);
        SpecHandle.Data->Period = EffectDef.Period;

        // Pass tick magnitude via SetByCaller
        FGameplayTag DataTag = (EffectDef.Type == EOnsetAbilityEffectType::Heal)
            ? TAG_Data_Heal
            : EffectDef.DamageTypeTag;
            
        SpecHandle.Data->SetSetByCallerMagnitude(DataTag, EffectDef.Magnitude);
        ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, SpecHandle, TargetHandle);
    }
}
```

The periodic damage ticks reuse the exact same `UOnsetDamageExecution` pipeline as instant attacks, ensuring armor mitigation and crits apply consistently per tick.

### 3. Authoritative Combat Execution (`UOnsetDamageExecution.cpp`)

Inside the damage execution calculation, primary and secondary attributes are captured from both source and target to compute mitigation and critical strikes:

```cpp
void UOnsetDamageExecution::Execute_Implementation(
    const FGameplayEffectCustomExecutionParameters& ExecutionParams,
    FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
    // 1. Capture Attacker and Defender attributes
    float AttackerPower = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().AttackPowerDef, EvaluateArgs, AttackerPower);

    float DefenderArmor = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ArmorDef, EvaluateArgs, DefenderArmor);

    float AttackerLuck = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().LuckDef, EvaluateArgs, AttackerLuck);

    // 2. Roll Critical Strike (LUK-based curve)
    const float CritChance = FMath::Clamp(AttackerLuck * 0.002f, 0.05f, 0.75f);
    const bool bIsCrit = FMath::FRand() < CritChance;
    const float CritMultiplier = bIsCrit ? 1.5f : 1.0f;

    // 3. Calculate Armor Mitigation (standard diminishing curve)
    const float Mitigation = DefenderArmor / (DefenderArmor + 400.0f);
    const float MitigatedDamage = RawBaseDamage * (1.0f - Mitigation) * CritMultiplier;

    // 4. Output final health delta
    OutExecutionOutput.AddOutputModifier(
        FGameplayModifierEvaluatedData(DamageStatics().HealthProperty, EGameplayModOp::Additive, -MitigatedDamage)
    );
}
```

---

## Results & Validation

1. **Instantaneous Test Suite:**
   Running `.\Test_All.ps1` boots Docker, checks table health, runs account integration tests, and executes character save/load validation in under 4 seconds. AWS credentials are no longer required on dev machines.
2. **Support & Healer Kit:**
   * Created **Holy Light**: 2.0s cast, 600 range, single-target heal restoring 180 HP to friendly targets.
   * Created **Renew**: Instant cast, applies a HoT ticking for 25 HP every 1.5s for 9 seconds.
   * Tested casting Holy Light on an enemy: the cast fizzled cleanly with "Invalid Target" feedback.
3. **Combat Math in Action:**
   Equipping a test ring (+40 Luck, +15 Physical Power) raised character crit chance from 5% to 13%. Critical hits landed with distinct orange damage numbers and a 1.5x damage coefficient.

---

## Challenges & Solutions

### Challenge 1: The Disappearing Enemies Bug (`bIsAlive`)

**Problem:** Immediately after hooking up enemy stat tables (`DT_EnemyStats`), enemies spawned in the arena but were invisible and non-collidable on multiplayer clients.

**Solution:** In `AOnsetEnemy::ResetStats()`, the code was resetting the entire struct to defaults, which inadvertently set `bIsAlive = false`. On the client, the replication hook for `bIsAlive` was hiding the mesh and turning off collision on spawn. Protecting `bIsAlive` across stat recalculations fixed the bug immediately.

### Challenge 2: DynamoDB Local Docker Timing in Scripts

**Problem:** In PowerShell scripts, calling `docker compose up -d` returned as soon as the container started, but the Java process inside DynamoDB Local took ~800ms to open port 8000. Subsequent calls to create tables immediately threw connection refused errors (`ECONNREFUSED`).

**Solution:** Updated `create-local-table.js` with an exponential backoff probe that checks `rawClient.send(new ListTablesCommand({}))` up to 10 times with 250ms delays before attempting table creation.

---

## Reflection & Lessons Learned

Local-first development is not a luxury; it is the cornerstone of engineering momentum. The moment tests require external internet access or cloud IAM permissions, developers stop running them. Bringing DynamoDB into Docker made testing joyful again.

On the gameplay side, standardizing combat formulas into a unified execution pipeline proved its worth instantly. Because DoTs, AoEs, and basic attacks all flow through `UOnsetDamageExecution`, armor mitigation and elemental resistance apply universally without duplicating math in individual abilities.

---

## Forward-Looking Content

| Phase | Status |
|---|---|
| Data-Driven Abilities & Ability Workbench | ✅ Committed (Post 20) |
| **Local Cloud Infrastructure & Combat Math** | ✅ **Committed** |
| Bag Inventory, Click-to-Loot & Leveling | ⏳ Next Up (Post 22) |
| Quests, Click-to-Move & Autoplay Polish | ⏳ Planned (Post 23) |
| The MH Framework Architecture | ⏳ Planned (Post 24) |

With combat math solid and abilities functioning for both damage and healing, players need reasons to fight: enemies need to drop loot, items need bags to live in, and killing monsters needs to reward experience and levels.

---

*Arc commits: `0cd1ad6`, `b2c5d10`, `33a6b3d`. Related docs: `Docs/combat-formulas.md`, `Docs/Server/Account_Api.md`.*
