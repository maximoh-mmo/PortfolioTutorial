# 📘 **GAS SYSTEM DOCUMENT**  

---

# **GAS System**

## **Purpose**
Provide ability execution and attribute modification with PvP‑aware damage filtering.

---

## **Responsibilities**
- Execute abilities  
- Apply GameplayEffects  
- Replicate attribute changes  
- Enforce PvP damage rules  

---

## **Non‑Responsibilities**
- UI rendering (handled by [UI System](../Gameplay/UI_System.md))  
- AI decision‑making (handled by [NPC AI System](../AI/NPC_AI_System.md))  
- Target selection (handled by [Targeting System](../Gameplay/Targeting_System.md))  

---

## **Ability Flow Diagram**

```mermaid
flowchart TD
    InputOrAI[Input or AI Decision] --> ASC[AbilitySystemComponent]
    ASC --> Ability[GameplayAbility]
    Ability --> TargetData
    TargetData --> Execution[GameplayEffect Execution]
    Execution --> Attributes[AttributeSet]
    Attributes --> DeathOrHit[Death / Hit Reaction]
```

---

## **PvP Damage Filtering**

### In Damage Execution Calculation:
```
if (Source is Player && Target is Player)
{
    if (!SourcePlayerState->bIsPvPEnabled)
    {
        // Block damage
        OutDamage = 0;
        return;
    }
}
```

### Applies to:
- Single‑target abilities  
- AoE abilities  
- Directional abilities  
- Projectile abilities  

### Does NOT apply to:
- NPC → Player damage  
- Player → NPC damage  

---

## **Targeting Integration**
If [Targeting System](../Gameplay/Targeting_System.md) rejects a player target due to PvP, GAS never receives invalid target data.

---

## **Key Classes**
- **`UAbilitySystemComponent`** — executes abilities, manages tags and effects  
- **`UGameplayAbility`** — base class for all abilities  
- **`UGameplayEffect`** — applies modifiers, damage, and cooldowns  
- **`UAttributeSet`** — defines attributes (Health, MaxHealth, Damage)  

---

## **Replication**
- Damage filtering occurs **server‑side only** via the [Multiplayer System](../Multiplayer/Multiplayer_System.md)  
- Clients receive replicated attribute changes  
- No client‑side prediction of [PvP System](../Gameplay/PVP_System.md) rules  

---

## **Testing Checklist**
- [ ] Abilities execute on input/AI trigger  
- [ ] Damage applies correctly  
- [ ] PvP filtering blocks player→player damage when OFF  
- [ ] AoE abilities respect PvP rules  
- [ ] Cooldowns work and replicate  
- [ ] Death triggers correctly (health ≤ 0)  

---

## **Edge Cases**
- AoE overlaps players when PvP disabled  
- Player toggles PvP mid‑ability  
- Projectile fired before PvP toggle hits a player  