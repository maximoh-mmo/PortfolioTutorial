# 🎬 Post 22 — From Corpses to Bags: Click-to-Loot, Inventory Architecture, and Progression

*8 commits · Aug 17–19, 2026 · covering Shared UInventoryComponent, Stacked Bags & Equipment Slots, Click-to-Loot Corpse Overlays, Leveling Curves & XP Persistence, Slate Enemy/Item/Loot Editors*

---

## Introduction

A combat engine without loot and progression is an arcade minigame. In an action RPG, the core emotional loop is the reward cycle: you fell a challenging enemy, watch its body collapse, spot the telltale shimmer of dropped equipment, walk over, pop open the loot window, slide a newly dropped broadsword into your paperdoll, and watch your stats climb as a golden level-up beam restores your health to full.

Until this sprint, Onset had none of that. Defeated enemies vanished into the ether, gear was an abstract concept in C++ structs, and character levels were hardcoded numbers.

Over three days and eight commits, I built the entire itemization, inventory, and progression pipeline from scratch:
* A unified `UInventoryComponent` that powers player bags, equipped gear, and corpse loot containers.
* A world-anchored click-to-loot interaction system with floating screen-space loot popups.
* A fully persisted experience, leveling, and attribute-point curve (including database migration 4 across SQLite and PostgreSQL).
* Two new in-editor Slate tools—the **Enemy Creation Tool** and the **Item & Loot Editor**—featuring automatic profile generation and persist-on-close validation.

---

## Technical Exposition

### The "Everything is an Inventory" Architecture

When designing an inventory system, a common trap is creating separate classes for every type of item container: `UPlayerInventory`, `UChestContainer`, `UCorpseLootHolder`, `UVendorStock`.

In Onset, I unified all item storage under a single component: `UInventoryComponent`.

```
                  +---------------------------+
                  |    UInventoryComponent    |
                  +---------------------------+
                  | - BagSlots (Array)        |
                  | - EquipmentSlots (Map)    |
                  | - MaxWeight / MaxSlots    |
                  +---------------------------+
                                |
        +-----------------------+-----------------------+
        |                                               |
        v                                               v
+-------------------------------+               +-------------------------------+
|      AOnsetPlayerCharacter    |               |          AOnsetCorpse         |
|  - 20 Bag Slots               |               |  - 0 Bag Slots                |
|  - 6 Equipped Slots           |               |  - Dynamic Loot Drops         |
|  - Replicated to Owner        |               |  - World Interaction Target   |
+-------------------------------+               +-------------------------------+
```

Because corpses and players share the exact same component:
1. **Looting is an Atomic Transfer:** Moving an item from a corpse to a player bag is simply `SourceInventory->RemoveItem()` followed by `TargetInventory->AddItem()`.
2. **Identical Serialization:** Both can be serialized to and from the same JSON format.
3. **Stat Recomputation:** When items are moved into equipment slots on a character, `UInventoryComponent` broadcasts `OnEquipmentChanged`, triggering immediate server-side stat recalculation.

### Click-to-Loot: World Interaction Meets Screen Overlay

Top-down ARPGs need frictionless looting. Forcing a player to stop, open their full-screen inventory modal, and drag items across the screen breaks the pacing of combat.

Instead, I implemented a lightweight, context-sensitive loot overlay:
* Hovering the cursor over an `AOnsetCorpse` outlines the mesh and changes the cursor to an interactive loot hand.
* Clicking the corpse invokes `UInteractionComponent::InteractWithTarget()`.
* If the player is within interaction range (200 units), the server sends the corpse's inventory contents to the client, which projects a compact `ULootOverlayWidget` directly adjacent to the corpse.
* Players can click individual items to loot them or click "Take All" (or press spacebar) to vacuum everything into their bag. Once empty, the corpse stops shimmering and enters its despawn timer.

### Leveling Math and the Progression Curve

Progression is governed by `UOnsetLevelingLibrary`, which reads configuration formulas from `DefaultEngine.ini`:

$$\text{XPRequired}(L) = \text{XPBase} \times (L)^{\text{XPGrowth}}$$

To prevent high-level characters from trivializing progression by farming starter zones, kills apply a **Level Differential Multiplier**:
* **Skull/Red (+4 or higher):** $1.25\times$ XP bonus (high risk, high reward).
* **Yellow ($\pm 2$ levels):** $1.0\times$ standard XP.
* **Green ($-3$ to $-5$ levels):** $0.5\times$ reduced XP.
* **Grey ($-6$ or lower):** $0.0\times$ zero XP (trivial kills grant no progression).

When `AddExperience()` pushes XP past the threshold, the character triggers a level-up cascade: Level increments, `UnspentStatPoints` increases by 5, current Health and Mana are fully restored, and a gameplay cue (`TAG_Event_LevelUp`) triggers the particle burst and audio sting.

---

## Implementation Details

### 1. Shared Inventory Component (`UOnsetInventoryComponent.cpp`)

The component manages stacking, capacity, and equipment slots, with JSON serialization feeding directly into the database:

```cpp
bool UOnsetInventoryComponent::AddItem(FName ItemRowName, int32 Quantity, int32& OutRemainingQuantity)
{
    OutRemainingQuantity = Quantity;
    const FOnsetItemDefinition* Def = GetItemDefinition(ItemRowName);
    if (!Def) return false;

    // 1. Try stacking onto existing bag slots
    if (Def->MaxStack > 1)
    {
        for (FOnsetInventorySlot& Slot : BagSlots)
        {
            if (Slot.ItemRowName == ItemRowName && Slot.Quantity < Def->MaxStack)
            {
                const int32 Space = Def->MaxStack - Slot.Quantity;
                const int32 ToAdd = FMath::Min(Space, OutRemainingQuantity);
                Slot.Quantity += ToAdd;
                OutRemainingQuantity -= ToAdd;

                if (OutRemainingQuantity <= 0)
                {
                    OnInventoryUpdated.Broadcast();
                    return true;
                }
            }
        }
    }

    // 2. Find first empty slot for remainder
    for (FOnsetInventorySlot& Slot : BagSlots)
    {
        if (Slot.IsEmpty())
        {
            const int32 ToAdd = FMath::Min(Def->MaxStack, OutRemainingQuantity);
            Slot.ItemRowName = ItemRowName;
            Slot.Quantity = ToAdd;
            OutRemainingQuantity -= ToAdd;

            if (OutRemainingQuantity <= 0)
            {
                OnInventoryUpdated.Broadcast();
                return true;
            }
        }
    }

    OnInventoryUpdated.Broadcast();
    return OutRemainingQuantity < Quantity;
}
```

### 2. Click-to-Loot Corpse Interaction (`InteractionComponent.cpp`)

When a player clicks the world, the interaction component traces for interactive actors before routing movement:

```cpp
void UInteractionComponent::TryInteract(AActor* TargetActor)
{
    if (!TargetActor || !TargetActor->Implements<UOnsetInteractableInterface>())
    {
        return;
    }

    const float Distance = FVector::Dist(GetOwner()->GetActorLocation(), TargetActor->GetActorLocation());
    if (Distance > MaxInteractionDistance)
    {
        // Out of range: queue approach movement
        MoveToAndInteract(TargetActor);
        return;
    }

    // In range: execute interaction on server
    Server_ExecuteInteraction(TargetActor);
}
```

On the server, `AOnsetCorpse::OnInteract()` opens the loot session, locking the corpse from concurrent looting by other players and replicating the slot contents to the looter's `ULootOverlayWidget`.

### 3. Leveling & Stat Progression (`AOnsetPlayerCharacter.cpp`)

Experience gains flow through `AddExperience`, handling multi-level jumps and stat grants:

```cpp
void AOnsetPlayerCharacter::AddExperience(int32 Amount)
{
    if (Amount <= 0 || CharacterLevel >= LevelCap) return;

    CurrentXP += Amount;

    while (CurrentXP >= XPToNextLevel && CharacterLevel < LevelCap)
    {
        CurrentXP -= XPToNextLevel;
        CharacterLevel++;
        UnspentStatPoints += StatPointsPerLevel;
        XPToNextLevel = UOnsetLevelingLibrary::GetXPForLevel(CharacterLevel);

        // Level-up rewards: full restoration
        GetCombatAttributeSet()->SetHealth(GetCombatAttributeSet()->GetMaxHealth());
        GetCombatAttributeSet()->SetMana(GetCombatAttributeSet()->GetMaxMana());

        // Notify systems and clients
        Multicast_OnLevelUp(CharacterLevel);
    }

    OnProgressionChanged.Broadcast(CharacterLevel, CurrentXP, XPToNextLevel, UnspentStatPoints);
}
```

### 4. Database Schema Migration 4

To persist progression across sessions, SQLite and PostgreSQL stores received Migration 4, adding the `unspent_stat_points` column:

```sql
-- Migration 004: Add unspent stat points
ALTER TABLE characters ADD COLUMN unspent_stat_points INTEGER NOT NULL DEFAULT 0;
```

---

## Results & Validation

1. **Full Kill-to-Loot Cycle:**
   * Slain enemy transitions to an `AOnsetCorpse` with a subtle golden glow.
   * Right-clicking the corpse pops open the floating `ULootOverlayWidget` showing an *Iron Broadsword* and 2x *Lesser Healing Potions*.
   * Clicking "Loot All" transfers the items into the player's 20-slot bag and plays the item pickup audio.
2. **Equipment & Stat Scaling:**
   * Right-clicked the *Iron Broadsword* in the inventory bag. It transferred into the `MainHand` equipment slot.
   * Player Attack Power immediately rose from 20 to 45; basic attack damage against training dummies increased accordingly.
3. **Progression Verification:**
   * Defeating 5 training dummies yielded 350 XP, triggering the Level 2 announcement.
   * HUD XP bar animated smoothly, and the character gained 5 unspent stat points.
   * Logged out and back in: character level, current XP, equipped gear, and unspent stat points restored identically from the local PostgreSQL store.

---

## Challenges & Solutions

### Challenge 1: Progression Lost During Autoplay Disconnect

**Problem:** When players enabled autoplay (where AI temporarily commands the character) and disconnected, the `AOnsetPlayerAIController` possessed the pawn while `AOnsetPlayerController` detached. The save-on-disconnect logic assumed the player controller was always attached, resulting in progression earned during AI combat being dropped on server shutdown.

**Solution:** Shifted progression ownership and `SetPersistIdentity` calls to `AOnsetPlayerCharacter` itself. The pawn now stores its identity and progression fields independently of which controller (Player or AI) is currently possessing it.

### Challenge 2: Cross-Table Foreign Key Desynchronization in Slate

**Problem:** In the custom Item & Loot Editor, creating an ability scroll requires picking an ability ID from `DT_Abilities`. If a designer later renamed or deleted that ability in the Ability Editor, the item table retained a broken reference, causing crashes when players tried to read the scroll.

**Solution:** Added a cross-table verification pass in `OnsetItemEditorWidget::ValidateRowData()`. If an item or loot entry points to a missing DataTable row, the cell turns bright red in the editor list and disables the persist-on-close routine until the reference is resolved.

---

## Reflection & Lessons Learned

Treating corpses as temporary `UInventoryComponent` holders rather than building a bespoke looting subsystem eliminated huge amounts of boilerplate. When systems share fundamental data primitives, new features become natural extensions rather than architectural rewires.

The persist-on-close pattern in our Slate tools has completely eliminated "unsaved data" bugs. When editors save automatically upon closing—backed by strict validation that refuses to write corrupted data—the tool gets out of the designer's way.

---

## Forward-Looking Content

| Phase | Status |
|---|---|
| Local Cloud Infrastructure & Combat Math | ✅ Committed (Post 21) |
| **Bag Inventory, Click-to-Loot & Leveling** | ✅ **Committed** |
| Quests, Click-to-Move & Autoplay Polish | ⏳ Next Up (Post 23) |
| The MH Framework Architecture | ⏳ Planned (Post 24) |
| Steam Packet Authentication & Security | ⏳ Planned (Post 25) |

Now that players have gear and levels, we need structure: an objective-driven Quest system, silky-smooth client-side click-to-move pathfinding, and fixing the player StateTree lifecycle so auto-combat and manual control seamlessly hand control back and forth.

---

*Arc commits: `f7e6570`, `6bae296`, `b69cb5f`, `6070d41`. Related docs: `Docs/Inventory/Inventory_System.md`, `Docs/Player/Leveling_System.md`, `Docs/Tools/Item_Loot_Editor.md`.*
