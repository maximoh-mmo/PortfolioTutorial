## 📘 Spawner System — `/Docs/AI/SpawnerSystem.md`

# **Spawner System**

## Purpose
Manage the creation of NPC groups in the world, assigning them to spawn points, groups, and initial states. It is responsible for *when* and *where* enemies appear, not how they behave.

## Responsibilities
- Spawn NPCs in groups at predefined locations  
- Assign NPCs to Group System  
- Configure enemy types per spawn point  
- Trigger respawn after group death or wave completion  
- Expose simple controls (enable/disable, wave count, etc.)

## Non‑Responsibilities
- Individual AI behaviour  
- Combat logic  
- Object pooling internals  
- UI or wave presentation  

## Key Classes
- **`AEnemySpawner`** — main spawner actor placed in the level  
- **`FSpawnConfig`** — struct for enemy type, count, spacing, etc.  

## Key Functions
- `SpawnGroup()` — spawns or activates a group of NPCs  
- `OnGroupWiped()` — called when all members are dead/pooled  
- `ScheduleRespawn()` — sets timers for respawn  
- `InitializeNPC(ANPC*)` — assigns group, type, initial state  

## Data Flow
Spawner → Pooling/Spawn → Group System → NPC AI

## Interactions
- **Pooling System:** requests NPC instances  
- **Group System:** registers members into groups  
- **NPC AI:** starts in Idle/Roam  
- **Final Demo Loop:** may trigger waves via spawners  

## Replication
- Spawner logic is **server‑only**  
- Spawner state (active/inactive) may replicate if needed for UI  

## Edge Cases
- Spawner disabled mid‑wave  
- No pooled NPCs available  
- Respawn while player is too close (optional rule)  

## Testing Checklist
- [ ] Spawns correct number and type of NPCs  
- [ ] Groups are registered correctly  
- [ ] Respawn works after wipe  
- [ ] Works with pooling  
- [ ] Works in multiplayer (server‑only logic)  

---