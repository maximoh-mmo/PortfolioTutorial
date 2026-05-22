## 📘 Ability Targeting System — `/Docs/Gameplay/AbilityTargetingSystem.md`

# **Ability Targeting System**

## Purpose
Provide a unified way to select targets and locations for abilities: single‑target, AoE, and directional abilities, driven by mouse input and/or AI.

## Responsibilities
- Interpret mouse position / target under cursor  
- Provide targeting data (actor, location, direction)  
- Show targeting indicators (circles, cones, etc.)  
- Validate targets (range, line of sight, team)  

## Non‑Responsibilities
- Ability execution (handled by GAS)  
- AI decision making  
- UI outside of targeting indicators  

## Key Classes
- **`UAbilityTargetingComponent`** — attached to player  
- **`FAbilityTargetData`** — actor, location, direction, radius, etc.  

## Key Functions
- `GetTargetUnderCursor()` — single‑target  
- `GetGroundLocationUnderCursor()` — AoE  
- `GetDirectionFromPlayerToCursor()` — directional abilities  
- `ShowTargetIndicator()` / `HideTargetIndicator()`  

## Data Flow
Input → TargetingComponent → TargetData → GAS Ability → Execution

## Interactions
- **Player System:** feeds mouse input  
- **GAS System:** consumes TargetData  
- **UI:** renders indicators  

## Replication
- Targeting is mostly **client‑side** visual  
- Final target data is validated on server when ability activates  

## Edge Cases
- No valid target under cursor  
- Target out of range  
- Obstructed line of sight  

## Testing Checklist
- [ ] Single‑target selection works  
- [ ] AoE location selection works  
- [ ] Directional abilities aim correctly  
- [ ] Indicators match actual ability behaviour  