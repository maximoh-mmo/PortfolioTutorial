# 🎬 Post 23 — Quests, Client-Side Click-to-Move, and Taming the Player StateTree

*15 commits · Aug 21–24, 2026 · covering Data-Driven Quests (DT_QuestDefinition), Client-Side Click-to-Move (UPathFollowingComponent), StateTree Lifecycle Hardening, Autoplay Interrupts, Prestige System*

---

## Introduction

In an action RPG, two things make or break the moment-to-moment experience: **how movement feels under your mouse** and **clear objectives guiding your progress**.

Up until this milestone, Onset suffered from a subtle but infuriating control sluggishness: every ground click routed through a server RPC. If you clicked to move, the character paused for a network round trip before taking its first step. In addition, the game's autoplay mode (powered by Unreal's StateTree) was fighting the player: toggling auto-combat on and off would lock up the StateTree, ignore manual WASD inputs, and freeze the character in place.

Across fifteen commits over four days, I resolved the control feel and gave the game structured purpose:
* Transferred click-to-move pathfinding directly to the client using `UPathFollowingComponent`, eliminating network lag on player locomotion.
* Tamed the Player StateTree lifecycle: implemented instant "any-input" interrupts and graceful hand-backs between manual and autonomous combat.
* Built the data-driven **Quest System** (`UOnsetQuestComponent`), integrating kill, collect, and exploration objectives with database persistence.
* Added a foundational prestige progression mechanic.

---

## Technical Exposition

### Client-Side Navigation: Why Server-RPC Locomotion Fails ARPGs

In slower-paced MMOs or turn-based games, sending a destination coordinate to the server via RPC and letting the server simulate movement is acceptable. In a fast-paced ARPG, it feels dreadful. Even with a 40ms ping, the delay between clicking and your character pivoting creates an unmistakable sensation of swimming in molasses.

```
Old Flow (Server-RPC):
[ Client Click ] ---> [ Server RPC: MoveTo ] ---> [ Server NavMesh Pathfind ] ---> [ Character Moves ]
                       (Lag: 1 Full RTT)

New Flow (Client-Side Path Following):
[ Client Click ] ---> [ Client Local NavMesh ] ---> [ UPathFollowingComponent Drives Locomotion ]
         |
         +----------> [ Replicate Destination to Server ] (Validation Only)
```

Unreal's `UPathFollowingComponent` is almost universally attached to AI controllers (`AAIController`). To give the player instant responsiveness, I brought the path follower into `AOnsetPlayerController` on the owning client:
1. Enabled `bAllowClientSideNavigation = true` in engine settings.
2. The client raycasts the cursor against terrain and projects the hit location onto the local navigation mesh.
3. `FindPathForMoveRequest()` computes waypoints locally and feeds `AddMovementInput()` on every tick.
4. Hostile clicks beyond ability range automatically path the character until within cast range, then seamlessly transition into ability activation.

### Taming the Player StateTree Lifecycle

Unreal Engine 5's StateTree is a massive upgrade over legacy Behavior Trees, but managing dynamic transitions between a human player and an autonomous AI brain is full of traps.

Previously, `AOnsetPlayerAIController` had `SetStartLogicAutomatically(true)`. The moment it possessed the pawn, it spun up an execution context. When a player toggled autoplay off, the tree was still running internally; toggling it back on refused `SetStateTree()` because Unreal prevents rebinding an active StateTree.

To fix this, I established strict **lifecycle ownership**:
* **Explicit Start/Stop:** The StateTree never auto-starts. It is explicitly started only when auto-combat is enabled, and explicitly stopped (`StopLogic()`) when disabled.
* **Any-Input Interrupt:** The moment the player touches a key—WASD, ability buttons 1–4, ground clicks—`EnsurePlayerControl()` immediately interrupts the StateTree, clears AI focus, and returns authority to the player controller.
* **Coalesced Pathing:** Spam-clicking the ground no longer aborts and rebuilds paths on every frame. A minimum-interval gate and goal-distance threshold prevent repath stuttering.

### The Objective-Driven Quest Pipeline

The quest system is driven by `DT_QuestDefinition` and tracked by `UOnsetQuestComponent` on `AOnsetBaseCharacter`:

```
+--------------------------------------------------------------+
|                     DT_QuestDefinition                       |
|   (Stages -> Objectives: Kill, CollectItem, ReachLocation)   |
+--------------------------------------------------------------+
                               |
                               v
+--------------------------------------------------------------+
|                     UOnsetQuestComponent                     |
|  - Server Authoritative                                      |
|  - Replicates Active Quest State to Owning Client            |
|  - JSON Serialization for DB Persistence                     |
+--------------------------------------------------------------+
        ^                      ^                      ^
        |                      |                      |
[ Enemy OnDeath ]     [ Inventory OnItemAdded ] [ Checkpoint Volume ]
```

Because objectives hook into existing engine delegates (`AOnsetEnemy::OnDeath`, `UOnsetInventoryComponent::OnItemAdded`), quests require zero bespoke scripting. Dropping a wolf updates "Kill 5 Wolves", picking up herbs updates "Collect 10 Sunflowers", and walking into a dungeon entrance triggers "Explore the Ruins".

---

## Implementation Details

### 1. Client-Side Click-to-Move (`AOnsetPlayerController.cpp`)

The player controller builds and follows local navigation paths directly:

```cpp
void AOnsetPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    if (bIsMoveActive && PathFollowingComponent)
    {
        // Follow local navmesh waypoints via AddMovementInput
        UpdatePathFollowing(DeltaTime);
    }
}

bool AOnsetPlayerController::MoveToLocation(const FVector& Dest, float AcceptanceRadius)
{
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys || !GetPawn()) return false;

    FPathFindingQuery Query;
    const FVector StartLocation = GetPawn()->GetActorLocation();
    Query.StartLocation = StartLocation;
    Query.EndLocation = Dest;
    Query.NavAgentProperties = GetPawn()->GetNavAgentPropertiesRef();

    FPathFindingResult PathResult = NavSys->FindPathSync(Query);
    if (PathResult.IsSuccessful() && PathResult.Path.IsValid())
    {
        CurrentPath = PathResult.Path;
        CurrentWaypointIndex = 1;
        bIsMoveActive = true;
        return true;
    }

    return false;
}
```

### 2. The Any-Input Autoplay Interrupt (`OnsetPlayerController.cpp`)

Any gameplay intent immediately yields autoplay control back to the human player:

```cpp
void AOnsetPlayerController::EnsurePlayerControl()
{
    if (bIsAutoCombatActive)
    {
        bIsAutoCombatActive = false;

        // Notify AI controller to stop StateTree execution
        if (AutoCombatAIController)
        {
            AutoCombatAIController->StopAutoCombatLogic();
        }

        // Reclaim input focus and update UI
        OnAutoCombatToggled.Broadcast(false);
    }
}

void AOnsetPlayerController::Input_AbilitySlot1()
{
    EnsurePlayerControl();
    ExecuteAbility(1);
}

void AOnsetPlayerController::Input_MoveForward(float Value)
{
    if (FMath::Abs(Value) > 0.1f)
    {
        EnsurePlayerControl();
        AddMovementInput(GetActorForwardVector(), Value);
    }
}
```

### 3. Quest Objective Tracking (`UOnsetQuestComponent.cpp`)

Objectives advance generically through `ReportObjectiveProgress`:

```cpp
void UOnsetQuestComponent::ReportObjectiveProgress(EOnsetObjectiveType Type, FName TargetID, int32 Amount)
{
    if (!GetOwner()->HasAuthority()) return;

    for (FOnsetActiveQuest& ActiveQuest : ActiveQuests)
    {
        const FOnsetQuestDefinition* Def = GetQuestDefinition(ActiveQuest.QuestID);
        if (!Def || !Def->Stages.IsValidIndex(ActiveQuest.CurrentStageIndex)) continue;

        const FOnsetQuestStage& CurrentStage = Def->Stages[ActiveQuest.CurrentStageIndex];

        for (int32 ObjIdx = 0; ObjIdx < CurrentStage.Objectives.Num(); ++ObjIdx)
        {
            const FOnsetQuestObjective& Obj = CurrentStage.Objectives[ObjIdx];
            if (Obj.Type == Type && Obj.TargetID == TargetID)
            {
                ActiveQuest.ObjectiveProgress[ObjIdx] = FMath::Min(
                    ActiveQuest.ObjectiveProgress[ObjIdx] + Amount,
                    Obj.RequiredCount
                );
            }
        }

        CheckStageCompletion(ActiveQuest);
    }

    OnRep_ActiveQuests();
}
```

---

## Results & Validation

1. **Snappy Controls:**
   Clicking terrain now produces instantaneous acceleration. The character turns and follows corners around obstacles with zero perceived latency.
2. **Smooth Autoplay Transitions:**
   * Turned on Autoplay: character automatically locked onto the nearest training dummy, executed rotation, cast basic attacks, and fired abilities on cooldown.
   * Tapped the `S` key: Autoplay disengaged instantly, the character stepped backward, and the UI toggle button unlit without glitching.
   * Toggled Autoplay back on: the StateTree re-initialized smoothly and resumed combat.
3. **Quest Execution:**
   * Picked up quest *Goblin Infestation* (Objective: Kill 3 Goblins).
   * Slain goblins triggered `ReportObjectiveProgress(EOnsetObjectiveType::Kill, "Goblin", 1)`.
   * On the third kill, the quest stage auto-advanced, granted 150 XP, and deposited 50 gold into the inventory bag.
   * Disconnected, reconnected: quest progress restored faithfully from `characters.quests_json`.

---

## Challenges & Solutions

### Challenge 1: The Micro-Stutter Spam-Click Bug

**Problem:** Players in ARPGs spam right-click constantly while moving. Because each click triggered `FindPathSync`, rapid clicks caused the path follower to cancel its current velocity, calculate a new path to nearly the same spot, and restart acceleration, causing noticeable visual hitching.

**Solution:** Added goal coalescing in `AOnsetPlayerController::IssueClickMove`. If a new click is within 60 units of the previous target and less than 150ms has elapsed, the controller ignores the redundant pathfind and allows current momentum to continue.

### Challenge 2: StateTree Instance Data Stale-Lock

**Problem:** When `AOnsetPlayerAIController` unpossessed the pawn, the StateTree instance data held dangling pointers to the previous character actor, causing crashes when re-possessing.

**Solution:** Hooked `OnUnPossess()` to call `StopLogic(TEXT("Unpossess"))` and clear all cached actor parameters before releasing control.

---

## Reflection & Lessons Learned

Movement responsiveness is holy in action games. The instinct to route everything through the server for "security" must never compromise the direct tactile connection between player input and pawn movement. By offloading pathfinding to the client while retaining server validation on actual ability execution and collision, we gained crisp controls without sacrificing anti-cheat boundaries.

Furthermore, building quests around decoupled events (`OnItemAdded`, `OnDeath`) rather than hardcoded quest triggers proved how cleanly a game can scale when systems communicate via message contracts.

---

## Forward-Looking Content

| Phase | Status |
|---|---|
| Bag Inventory, Click-to-Loot & Leveling | ✅ Committed (Post 22) |
| **Quests, Click-to-Move & Autoplay Polish** | ✅ **Committed** |
| The MH Framework & Async Multi-Store Persistence | ⏳ Next Up (Post 24) |
| Steam Packet Authentication & Dedicated Server Hardening | ⏳ Planned (Post 25) |

With gameplay, inventory, combat, and quests firing on all cylinders, our attention shifts to enterprise-grade infrastructure. It's time to build the **MH Framework**: a modular architecture of independent Unreal plugins providing cancellable async contracts and unified multi-backend persistence across SQLite, PostgreSQL, and HTTP.

---

*Arc commits: `b0f2051`, `0de2b4a`, `279ef74`. Related docs: `Docs/AI/Player_AI_System.md`, `Docs/Gameplay/Targeting_System.md`.*
