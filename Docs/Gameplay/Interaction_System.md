# 📘 **INTERACTION SYSTEM**
**File:** `/Docs/Gameplay/Interaction_System.md`

---

# **Interaction System (`MHInteraction` + `Onset`)**

## **Purpose**
Provide a modular, network-authoritative interaction architecture split between the **MH Framework** (`MHInteraction` plugin — generic mechanisms) and **Onset** (`Source/Onset` — game meaning, NPCs, branching dialogue, quest hooks, doors, switches/triggers, and corpses).

---

## **Architecture Split**

### **1. `MHInteraction` Plugin (`Project/Plugins/MHInteraction`)**
Owns the game-agnostic contracts and validation rules:
- **`FMHInteractionDefinition`** & **`FMHInteractionContext`** (`MHInteractionTypes.h`) — interaction metadata (`InteractableId`, `DisplayName`, `ActionPrompt`, `InteractionRange`, `TriggerMode`, `bRequiresLineOfSight`) and execution context (`InstigatorActor`, `InstigatorController`, `TargetActor`, `bTriggeredByOverlap`).
- **`IMHInteractable` / `UMHInteractable`** (`IMHInteractable.h`) — `BlueprintNativeEvent` interface implementable on any `AActor` or `UActorComponent` (`GetInteractionDefinition`, `CanInteract`, `ExecuteInteraction`).
- **`UMHInteractableComponent`** (`UMHInteractableComponent.h`) — reusable, replicated component that can be dropped onto any actor. Supports `ExplicitOnly`, `OverlapOnly`, and `ExplicitAndOverlap` trigger modes, single-use latches, cooldowns, C++ predicate hooks (`OnCanInteractQuery`, `OnModifyDefinition`), and Blueprint/C++ multicast delegates (`OnInteracted`, `OnInteractedNative`).
- **`UMHInteractionLibrary`** (`UMHInteractionLibrary.h`) — static resolution, range check, line-of-sight check, and validation helpers (`ResolveInteractableObject`, `ValidateInteraction`, `TryExecuteInteraction`).

### **2. `Onset` Game Implementations (`Project/Source/Onset`)**
Consumes `MHInteraction` for concrete world actors and player workflows:
- **Player Input & Walk-Up (`UInteractionComponent` & `AOnsetPlayerController`):**
  1. **Click / Tap to Walk-Up & Interact:** Clicking any `IMHInteractable` clears combat targeting; if already in range, sends `Server_RequestInteract` immediately; if out of range, paths the pawn into range and automatically triggers the interaction upon arrival.
  2. **Proximity Scan & Dedicated Interact Key (`IA_Interact` / `E` / Gamepad West):** Periodically scans around the local pawn (`UpdateProximityFocus`), displays the floating `[E] <Action> — <Name>` prompt via `UOnsetInteractionPromptWidget`, and interacts on press.
  3. **Automatic Overlap Trigger:** Handled server-side when a player pawn overlaps a primitive bound to `UMHInteractableComponent::BindOverlapPrimitive`.
- **Non-Enemy NPCs & Branching Dialogue (`AOnsetNPC`, `UOnsetDialogueLibrary`, `OnsetDialogueTypes.h`):**
  - Driven by `DT_NPCs` and `DT_Dialogues` (overridable in `DefaultEngine.ini` under `[Onset.Gameplay]` with built-in runtime fallback rows).
  - Interacting with an `AOnsetNPC` automatically reports `EOnsetQuestObjectiveType::Interact` to `UOnsetQuestComponent` and opens `UOnsetDialogueOverlayWidget` on the client.
  - Dialogue choices can gate visibility by quest state (`RequiredQuestNotStarted`, `RequiredQuestActive`, `RequiredQuestComplete`) and execute server-authoritative quest actions (`GrantQuestOnSelect`, `CompleteQuestOnSelect`, `ReportGenericObjectiveTarget`).
- **World Doors (`AOnsetDoor`):**
  - Replicated open/closed and locked states with smooth procedural pivot swing (`OpenRotationOffset`) and/or slide (`OpenLocationOffset`).
  - Supports key-item requirements (`RequiredKeyItemRow`, `bConsumeKeyOnUnlock`), quest completion gates (`RequiredQuestComplete`), or remote unlocking via linked triggers.
- **World Switches / Triggers (`AOnsetInteractableTrigger`):**
  - Supports explicit levers/shrines or overlap pressure plates, toggles/unlocks `LinkedActors` (such as `AOnsetDoor`), and reports `Interact` and optional `ReachLocation` quest progress.
- **Corpse Looting (`AOnsetCorpse`):**
  - Uses `UMHInteractableComponent` (`ActionPrompt = "Loot"`) so corpses support both click-to-walk-up looting and proximity `[E] Loot`.
