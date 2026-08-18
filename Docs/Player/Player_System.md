# 📘 **PLAYER SYSTEM DOCUMENT**  
**File:** `/Docs/Player/Player_System.md`

---

# **Player System**

## **Purpose**
Provide top‑down ARPG controls (mouse + touch) and a UI‑driven PvP toggle that affects targeting and damage rules.

---

## **Responsibilities**
- Input handling (mouse + touch + gamepad)  
- Tap/click‑to‑move + screen‑relative WASD + gamepad L-Stick movement  
- Tap/click‑to‑target  
- Gamepad R-Stick software cursor  
- Ability activation (keyboard + touch buttons + gamepad)  
- PvP toggle UI → PlayerState (implemented)  
- Autoplay handoff (implemented)  

---

## **Non‑Responsibilities**
- AI behaviour (handled by [Player AI System](../AI/Player_AI_System.md))  
- NPC spawning (handled by [Spawner System](../AI/Spawner_System.md))  
- Ability definitions (handled by [GAS System](../GAS/GAS_System.md))  

---

## **Key Classes**
- **`AOnsetBaseCharacter`** — shared base for player and NPC, inherits `ACharacter`  
- **`AOnsetPlayerCharacter`** — player character, inherits `AOnsetBaseCharacter`, camera lives here  
- **`AOnsetPlayerController`** — routes input, cursor management, targeting, PvP toggle; owns `UInteractionComponent`  
- **`AOnsetPlayerState`** — stores and replicates `bIsPvPEnabled`, `bAutoplayEnabled`, `bContinueOnDisconnect`  
- **`UCursorManager`** — provides unified cursor position from mouse, touch, or gamepad R-Stick  
- **`UTargetingComponent`** — data holder for `CurrentTarget`, target validation — lives on pawn (shared base)  
- **`UInteractionComponent`** — click resolution extracted from controller (SRP): raycast → enemy targeting, corpse loot, or ground movement; resolves pawn via `GetPawn()`; no-op under AI control. Corpse branch handles click-to-loot with range-based auto-path (`TryLootCorpse` / `LootCorpse`)  
- **`UJoystickWidget`** — touch virtual joystick, injects axis into `IA_Move`  
- **`UGamepadCursorWidget`** — software crosshair overlay for gamepad R-Stick cursor  

---

## **Tap/Click‑to‑Move Flow**

`IA_Primary` (tap/click/A-button) produces a screen-space position that drives the same raycast pipeline. The PlayerController resolves context:

```mermaid
flowchart TD
    Input[IA_Primary: Tap / Click / A-Button] --> Cursor[UCursorManager<br/>GetCursorPosition]
    Cursor --> Interaction[UInteractionComponent<br/>ProcessPrimaryInteraction]
    Interaction --> Raycast[Screen → World Raycast]
    Raycast --> Branch{Hit what?}
    Branch -->|Corpse| Loot[TryLootCorpse<br/>in range → loot now<br/>else auto-path + poll]
    Branch -->|Enemy tag| Target[SetCurrentTarget via TargetingComponent]
    Branch -->|Player tag| TargetPVP{PVP On?}
    TargetPVP -->|Yes| Target
    TargetPVP -->|No| MoveTo
    Branch -->|Ground| MoveTo[MoveToLocation]
```

---

## **WASD + Gamepad L‑Stick Movement**

`IA_Move` (2D axis) drives movement via `OnMove` in `AOnsetPlayerController`. Movement is **screen‑relative**: the input axis is rotated by the camera's yaw (`GetControlRotation().Yaw`) so that:

| Input | Screen Direction | World Direction |
|-------|-----------------|-----------------|
| W / L‑Stick ↑ | Up on screen | Camera forward projected onto ground |
| S / L‑Stick ↓ | Down on screen | Camera backward projected onto ground |
| A / L‑Stick ← | Left on screen | Camera left (right vector negated) |
| D / L‑Stick → | Right on screen | Camera right |

This gives consistent movement regardless of which way the character is facing — W always moves toward the top of the screen, S toward the bottom, etc.

Any WASD/L‑Stick input also calls `Server_DisableAutoCombat()` and `StopMovement()` to interrupt active pathfinding or auto‑combat.

---

## **Tap/Click‑to‑Loot Flow** *(implemented)*

Clicking a corpse (raycast hits `AOnsetCorpse`) routes to the loot branch:

```
IA_Primary → UInteractionComponent::ProcessPrimaryInteraction
    → Cast<AOnsetCorpse>(HitActor)?
        ├── TryLootCorpse(Corpse)
        │     ├── bLooted? → abort
        │     ├── Within LootRange (250u)? → LootCorpse
        │     │     ├── PawnInventory->AddItems(Corpse Loot)
        │     │     ├── Client_ShowLootOverlay(Loot)  [client RPC]
        │     │     ├── Corpse->bLooted = true; Destroy()
        │     └── Out of range → auto-path to corpse, 0.2s arrival poll → loot
```

- No range gate on the click (consistent with click-to-target); range only decides "loot now" vs "path then loot".
- Loot transfers into the player's `UOnsetInventoryComponent` bag; overlay is informational.
- Full details in [Inventory & Loot System](../Inventory/Inventory_System.md).

---

## **PvP Toggle Flow** *(implemented)*

### UI → PlayerController
Player clicks PvP toggle:

```
OnPvPToggleChanged(bool bEnabled)
```

### PlayerController → Server
```
Server_SetPvPEnabled(bEnabled)
```

### Server → PlayerState
```
bIsPvPEnabled = bEnabled;
```

### Replication → All Clients
`OnRep_PvPEnabled()` updates UI + targeting rules.

---

## **Targeting Integration**
*(See [Targeting System](../Gameplay/Targeting_System.md) for current state.)*
- If [PvP System](../Gameplay/PVP_System.md) disabled → ignore player actors via [Targeting System](../Gameplay/Targeting_System.md)  
- If PvP enabled → include player actors  
- If `CurrentTarget` becomes invalid due to PvP toggle → auto‑select nearest NPC  

## **GAS Integration** *(implemented)*
PlayerController routes ability input → ASC via [GAS System](../GAS/GAS_System.md)  
ASC checks PvP rules before applying damage.

## **UI Integration** *(partial — see [UI System](../Gameplay/UI_System.md))*
[UI System](../Gameplay/UI_System.md) displays:

- Virtual joystick (touch) [done]  
- Gamepad cursor overlay [done]  
- PvP ON/OFF [implemented]  
- Color‑coded indicator [implemented]  

---

## **Replication** *(implemented)*
- PvP flag replicates via `AOnsetPlayerState`  
- UI updates on `OnRep_PvPEnabled`  
- Server enforces all PvP rules  

---

## **Testing Checklist**
- [x] Tap/click‑to‑move moves character to target location (mouse + touch)  
- [x] WASD + gamepad L‑Stick movement is screen‑relative (camera yaw, not character facing)  
- [x] Tap/click‑to‑target sets `CurrentTarget` correctly (mouse + touch)  
- [x] PvP toggle replicates to all clients  
- [x] Targeting respects PvP flag (players filtered when OFF)  
- [x] Player AI autoplay can be enabled/disabled  
- [x] Works in multiplayer with multiple clients  
- [ ] Click corpse in range loots immediately (transfer + overlay + destroy)
- [ ] Click corpse out of range auto-paths and loots on arrival
- [ ] Double-click cannot double-loot a corpse

---

## **Edge Cases**
- Player toggles PvP mid‑combat  
- Player AI must respect PvP rules  
- Player targeting a player when PvP is turned OFF
- Player clicks a corpse out of range while in combat — auto-paths; movement interrupts combat
- Corpse already looted (`bLooted`) — interaction aborts  
