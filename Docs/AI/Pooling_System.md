## 📘 Pooling System — `/Docs/AI/PoolingSystem.md`

# **Pooling System**

## Purpose
Provide efficient reuse of NPC instances to avoid frequent spawn/destroy calls and reduce runtime allocation overhead.

## Responsibilities
- Pre‑allocate NPC instances  
- Hand out NPCs on request  
- Reset NPC state on reuse  
- Return NPCs to pool on death/cleanup  

## Non‑Responsibilities
- Spawning logic (when/where)  
- AI behaviour  
- Combat logic  

## Key Classes
- **`ANPCPoolManager`** — owns and manages pooled NPCs  
- **`FPooledNPCEntry`** — struct with NPC reference + state  

## Key Functions
- `GetNPC()` — returns an available NPC instance  
- `ReleaseNPC(ANPC*)` — returns NPC to pool  
- `ResetNPC(ANPC*)` — clears health, AI state, visuals  

## Data Flow
Spawner → PoolManager.GetNPC → NPC → (Death) → PoolManager.ReleaseNPC

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