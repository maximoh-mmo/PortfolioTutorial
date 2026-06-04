## 📘 Pooling System — `/Docs/AI/Pooling_System.md`

# **Pooling System**

## Purpose
Provide efficient reuse of NPC instances to avoid frequent spawn/destroy calls and reduce runtime allocation overhead.

## Responsibilities
- Pre‑allocate NPC instances  
- Hand out NPCs on request  
- Reset NPC state on reuse  
- Return NPCs to pool on death/cleanup  
- Hand off world-debris lifecycle to the Corpse System on NPC death  

## Non‑Responsibilities
- Spawning logic (when/where)  
- AI behaviour  
- Combat logic  

## Key Classes
- **`AOnsetPoolManager`** — owns and manages pooled NPCs; hardcodes `AOnsetEnemy::StaticClass()` for pre-allocation (no `PoolClass` property — all NPCs share the same base class)  

## Key Functions
- `GetPooledEnemy()` — returns an available NPC instance (creates new one on exhaustion as fallback)  
- `ReleasePooledEnemy(AOnsetEnemy*)` — returns NPC to pool  
- `ReturnToPool(AOnsetEnemy*)` — resets location, collision, state; calls `ApplyProfile(nullptr)` to clear profile-driven visuals  

## Data Flow

```mermaid
flowchart TD
    RequestNPC --> PoolCheck{NPC Available?}
    PoolCheck -->|Yes| ActivateNPC
    PoolCheck -->|No| CreateNewNPC

    ActivateNPC --> ResetState
    ResetState --> Spawned

    NPCDies --> ReturnToPool
    ReturnToPool --> Inactive
    Inactive --> RequestNPC
```

```
Spawner → PoolManager.GetPooledEnemy → NPC
NPC → (Death) → PoolManager.ReleasePooledEnemy (NPC recycled immediately)
               → Corpse System.SpawnCorpse() (lightweight debris persists)
```

## Two-Tier Architecture
The pooling system operates in two tiers:

1. **AI Actor Pool** — Full NPCs with StateTree, Perception, AbilitySystemComponent, targeting. These are high-cost actors that recycle instantly into the pool when the NPC dies.
2. **Corpse Actor Pool** (or timed-life spawns) — Minimal `AOnsetCorpse` actors with a static mesh, no Tick, and a self-destruct timer. Created on death, destroyed after a lifespan, never block the AI pool.

This separation frees high-cost AI actors immediately on death while maintaining visual persistence in the world. The two lifecycles are independent — a corpse can outlive several pool-recycles of the same NPC type.

For full documentation, see the [Corpse System](Corpse_System.md).

## Interactions
- **[Spawner System](Spawner_System.md):** main consumer of pooled NPCs  
- **[NPC AI System](NPC_AI_System.md):** must reset StateTree/AI state on reuse  
- **[Group System](Group_System.md):** must re‑register NPCs on reuse  

## Replication
- Pooling is **server‑only**  
- Clients only see replicated NPCs as usual  

## Edge Cases
- Pool exhaustion  
- NPC released while still referenced by AI  
- Incorrect reset (old target, old health, etc.)  

## Testing Checklist
- [ ] NPCs reset correctly (health, AI, visuals)  
- [ ] No stale targets or group data  
- [ ] No crashes when pool is exhausted  
- [ ] Works under heavy spawn/respawn cycles  
- [ ] NPC returns to pool on death — hidden, collision off, tick off  
- [ ] Corpse spawns independently — pool return not blocked by corpse cleanup  
- [ ] Corpse cap enforced under rapid death cascade  