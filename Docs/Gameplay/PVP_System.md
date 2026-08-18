# 📘 **PvP SYSTEM DOCUMENT**  
**File:** `/Docs/Gameplay/PVP_System.md`

---

# **PvP System**

## **Purpose**
Provide a **player‑controlled PvP/PvE toggle** that determines whether a player can:

- Target other players  
- Damage other players  
- Be targeted by Player AI (optional)  

This system ensures **player agency** in multiplayer combat while keeping the rules deterministic and server‑authoritative.

---

## **Responsibilities**
- Store a replicated PvP flag per player  
- Provide UI controls for toggling PvP mode  
- Filter valid targets based on PvP state  
- Prevent player‑to‑player damage when PvP is disabled  
- Update UI and targeting indicators accordingly  

---

## **Non‑Responsibilities**
- NPC behaviour (NPCs always treat players as valid targets)  
- Ability logic (handled by [GAS System](../GAS/GAS_System.md))  
- UI rendering (handled by [UI System](UI_System.md))  

---

## **Key Classes**

### `AOnsetPlayerState`
Authoritative storage for:

```
bool bIsPvPEnabled;
```

Replicated to all clients.

### `AOnsetPlayerController`
- Receives UI toggle input  
- Sends `Server_SetPvPEnabled(bool)` RPC  

### `UTargetingComponent`
- `IsActorTargetPVPValid(Target, Source)` rejects player targets when the source player has PvP disabled (called by the player's `InteractionComponent` during click-to-target before `SetTarget`)  

### `UGameplayEffectExecution` / Damage Execution
- Blocks damage if PvP disabled  

---

## **Key Functions** *(implemented)*

### AOnsetPlayerController
- `Server_SetPvPEnabled(bool)`  
- `OnRep_PvPEnabled()`  

### TargetingComponent (via AOnsetPlayerController / InteractionComponent)
- `IsActorTargetPVPValid(TargetActor, SourceActor)` called in `UOnsetInteractionComponent` during click-to-target (see [Player System](../Player/Player_System.md))
  - Rejects players if PvP disabled  

### [GAS System](../GAS/GAS_System.md) Damage Abilities
- AoE/Cone abilities skip player targets when PvP is disabled (checked inline in `OnsetGA_AoE` / `OnsetGA_Cone` via `bIsPvPEnabled`)  
  - Blocks player→player damage if PvP disabled  

---

## **Data Flow Diagram**

```mermaid
flowchart TD
    UI[UI PvP Toggle] --> PC[PlayerController]
    PC --> RPC[Server_SetPvPEnabled RPC]
    RPC --> PS[PlayerState.bIsPvPEnabled]

    PS --> Targeting
    PS --> GAS

    Targeting -->|PvP OFF| FilterOutPlayers
    Targeting -->|PvP ON| AllowPlayers

    GAS -->|PvP OFF| BlockPlayerDamage
    GAS -->|PvP ON| AllowPlayerDamage
```

---

## **Interactions**

### [Targeting System](Targeting_System.md)
- Filters out players when PvP disabled  
- Allows targeting players when PvP enabled  

### [GAS System](../GAS/GAS_System.md)
- Blocks damage to players when PvP disabled  

### [UI System](UI_System.md)
- Displays PvP toggle  
- Shows PvP status indicator  

### [Multiplayer System](../Multiplayer/Multiplayer_System.md)
- PvP flag is server‑authoritative  
- Replicates to all clients

---

## **Replication Rules**
- `bIsPvPEnabled` replicates via PlayerState  
- UI updates on `OnRep_PvPEnabled`  
- Server enforces all PvP rules  

---

## **Edge Cases**
- Player toggles PvP mid‑combat  
- Player has a player target when toggling PvP OFF  
- AoE abilities overlapping players  
- Player AI autoplay respecting PvP rules  

---

## **Testing Checklist**
- [x] PvP toggle replicates correctly  
- [x] Player cannot target players when PvP disabled  
- [x] Player cannot damage players when PvP disabled  
- [x] AoE abilities ignore players when PvP disabled  
- [x] Player can target/damage players when PvP enabled  
- [x] Works in multiplayer with multiple clients  

---

## **Future Extensions**
- PvP zones  
- PvP cooldown timer (anti‑toggle abuse)  
- PvP matchmaking  
- PvP ranking  
