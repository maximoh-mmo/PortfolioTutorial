
## 📘 Player AI System — `/Docs/AI/Player_AI_System.md`

# **Player AI System**

## Purpose
Provide an **autoplay/testing mode** where the player character is controlled by AI instead of human input, useful for stress tests, demos, and debugging.

## Responsibilities
- Control player character via AIController  
- Use StateTree to decide movement, targeting, and ability usage  
- Select targets based on proximity/threat  
- Trigger abilities intelligently  

## Non‑Responsibilities
- Human input handling  
- Ability definitions (handled by [GAS System](../GAS/GAS_System.md))  
- UI (handled by [UI System](../Gameplay/UI_System.md))  

## Key Classes
- **`APlayerAIController`** — AI controller for player pawn  
- **`UPlayerAIStateTreeComponent`** — runs player AI StateTree  

## Key Functions
- `EnableAutoplay(bool)` — toggle AI control  
- `FindBestTarget()` — choose target based on rules  
- `ShouldUseAbility()` — simple decision logic  

## StateTree Overview
- **Idle** → **SeekTarget** → **MoveToTarget** → **Attack/UseAbility**  
- Optional **Kite/Flee** state  

## Interactions
- **[Player System](../Player/Player_System.md):** possession switching between PlayerController and PlayerAIController  
- **[GAS](../GAS/GAS_System.md):** triggers abilities  
- **[UI](../Gameplay/UI_System.md):** debug toggle for autoplay  
- **[Targeting System](../Gameplay/Targeting_System.md):** target selection for AI  

## Replication
- Player AI logic runs **server‑side** in multiplayer  
- Inputs are simulated on server; movement/abilities replicate normally  

## Edge Cases
- No valid targets  
- Target dies mid‑attack  
- Autoplay toggled mid‑state  

## Testing Checklist
- [ ] Autoplay toggles cleanly  
- [ ] Player AI picks reasonable targets  
- [ ] Abilities fire correctly  
- [ ] Works in multiplayer (server‑side AI)  