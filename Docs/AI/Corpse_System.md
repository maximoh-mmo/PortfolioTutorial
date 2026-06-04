## Corpse System — `/Docs/AI/Corpse_System.md`

# Corpse Actor System

## Purpose
Decouple world-debris (corpses) from high-cost AI/ASC actors. When an NPC dies, the `AOnsetEnemy` returns to the pool immediately while a lightweight `AOnsetCorpse` actor persists at the death location for visual feedback, timed cleanup, and future loot-container extension.

## Responsibilities
- Spawn a minimal corpse actor at the NPC's death location
- Freeze the NPC's last pose (static mesh or frozen skeletal mesh)
- Clean up the corpse after a configurable lifespan
- Serve as a loot-container extension point (walk-over-detect pickup)
- Maintain a hard global cap on simultaneous corpses to prevent world-bloat

## Non‑Responsibilities
- AI behaviour or combat logic
- Object pooling internals
- Respawning or spawner timers (spawner starts its timer on NPC death, not corpse despawn)

## Key Classes
- **`AOnsetCorpse`** — minimal `AActor` subclass with a static mesh component, timed self-destruct, optional collision for loot interaction. No Tick unless chasing a fade-out animation.
- **`UCorpseSubsystem`** (future) — engine subclass that manages the corpse cap, enforces cleanup, and tracks active corpses for the spawning system.

## Data Flow

```
NPC Health <= 0 (PostGameplayEffectExecute)
  │
  ├──> ReturnToPool() → NPC recycled (AI + ASC freed)
  │
  └──> SpawnCorpse(DeathLocation, DeathPoseData)
         │
         ├──> AOnsetCorpse placed in world
         ├──> Lifespan timer starts (e.g. 15s)
         │
         └──> OnTimerExpired → Destroy()

(Optional Loot Extension)
  Corpse → OnActorBeginOverlap → Detect Player → Open Interaction UI → Pickup
```

## Interactions
- **[GAS System](../GAS/GAS_System.md):** Fires death event — the two parallel paths (pool return + corpse spawn) are triggered from `PostGameplayEffectExecute` when `Health <= 0`
- **[Pooling System](Pooling_System.md):** The NPC returns to the pool immediately on death, independent of the corpse timeline — this is the core performance benefit
- **[Spawner System](Spawner_System.md):** Spawner receives the death notification at the same time as the corpse spawn — the respawn timer is not blocked by corpse cleanup
- **[Future: UI System](../Gameplay/UI_System.md):** Loot indicator widget, interact prompt when player overlaps corpse

## Design Decisions
- **Two-tier architecture:** High-cost AI actors (StateTree, Perception, ASC) recycle instantly; cheap visual debris (static mesh, no Tick) handles persistence. This is a common MMO/looter-shooter pattern (Borderlands, Destiny).
- **Corpse lifespan vs pool exhaustion:** A global cap on active corpses prevents edge cases where rapid killing outpaces despawn timers. When the cap is reached, the oldest corpse is destroyed immediately.
- **No replication:** Corpses are cosmetic only — server creates them, clients see their replicated location/mesh. No RPCs needed beyond standard actor replication.

## Edge Cases
- Corpse cap reached — destroy oldest corpse before spawning new one
- NPC dies off-navmesh (no valid corpse location) — use last valid location
- NPC returns to pool while player is interacting with its corpse (corpse ref is independent, safe)
- Rapid death cascade (AoE kills 10 NPCs) — each spawns its own corpse; cap enforcement kicks in
- Pool exhaustion during corpse flood — corpses don't use the pool; they use direct SpawnActor with timed Destroy

## Testing Checklist
- [ ] Corpse spawns at NPC death location
- [ ] NPC returns to pool immediately (hidden, collision off, tick off)
- [ ] Corpse despawns after configured lifespan
- [ ] Corpse cap is enforced (oldest removed when cap reached)
- [ ] Respawning functions correctly (spawner timer independent of corpse)
- [ ] No memory leaks under heavy death load
- [ ] Loot extension point works (optional, future)
