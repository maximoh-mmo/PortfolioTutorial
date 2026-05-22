## 📘 Ability Targeting System — `/Docs/Gameplay/Ability_Targeting_System.md`

# **Ability Targeting System**

## Purpose
Provide a unified way to select targets and locations for abilities: single‑target, AoE, and directional abilities, driven by screen input (mouse/touch) and/or AI.

## Responsibilities
- Interpret screen position (mouse cursor / touch location)  
- Provide targeting data (actor, location, direction)  
- Show targeting indicators (circles, cones, etc.)  
- Validate targets (range, line of sight, team)  

## Non‑Responsibilities
- Ability execution (handled by [GAS System](../GAS/GAS_System.md))  
- AI decision making  
- [UI System](UI_System.md) outside of targeting indicators  

## Key Classes
- **`UAbilityTargetingComponent`** — attached to player  
- **`FAbilityTargetData`** — actor, location, direction, radius, etc.  

## Key Functions
- `GetTargetUnderCursor()` — single‑target  
- `GetGroundLocationUnderCursor()` — AoE  
- `GetDirectionFromPlayerToCursor()` — directional abilities  
- `ShowTargetIndicator()` / `HideTargetIndicator()`  

## Data Flow
Input → TargetingComponent → TargetData → [GAS System](../GAS/GAS_System.md) Ability → Execution

## Interactions
- **[Player System](../Player/Player_System.md):** feeds screen input (mouse/touch)  
- **[Targeting System](Targeting_System.md):** provides target selection context  
- **[GAS System](../GAS/GAS_System.md):** consumes TargetData  
- **[UI](UI_System.md):** renders indicators  

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