## 📘 Spawner System — `/Docs/AI/Spawner_System.md`

# **Spawner System**

## Purpose
Manage the creation and ongoing respawn of NPC groups in the world, assigning them to spawn points, groups, and initial states. Each NPC respawns independently on its own timer — the group does not wait for all members to die before respawning.

## Responsibilities
- Spawn NPCs in groups at predefined slot locations  
- Assign NPCs to Group System  
- Configure enemy types per slot via three profiles: `UVisualProfile` (mesh/anim), `UAIProfile` (behaviour), `UPerceptionProfile` (sight/hearing)  
- Assign a per-NPC respawn rate; each killed NPC respawns independently on its own timer  
- On NPC death, receive notification and start respawn timer (independent of corpse lifecycle)  
- Expose simple controls (enable/disable, wave count, etc.)

## Non‑Responsibilities
- Individual AI behaviour  
- Combat logic  
- Object pooling internals  
- UI or wave presentation  

## Key Classes
- **`AOnsetSpawner`** (`Onset/Source/Onset/Public/Spawning/`) — main spawner actor placed in the level  
- **`FSpawnConfig`** — struct for enemy profile, count, spacing, respawn rate, etc.  
- **`FSpawnerSlot`** (`Onset/Source/Onset/Public/Spawning/SpawnerSlot.h`) — struct pairing a spawn transform with an occupant reference  

## Key Functions
- `InitSlots()` — pre‑computes slot transforms from `SpawnPoints` or fallback ring scatter on `BeginPlay`  
- `SpawnGroup()` — fills all empty slots; calls `SpawnEnemyAtSlot()` for each  
- `SpawnEnemyAtSlot(int32 SlotIndex)` — retrieves an NPC from `UOnsetPoolSubsystem.GetPooledEnemy()`, calls `ApplyProfile(Config.EnemyVisualProfile)` on the pawn, calls `ApplyAIProfile(Config.EnemyAIProfile)` + `ApplyPerceptionProfile(Config.EnemyPerceptionProfile)` on the controller, then possesses. Registers with Group System.  
- `DestroyGroup()` — iterates all slots, destroys any occupant, clears slot references  
- `DebugKillLast()` — kills the most recently spawned occupant (test helper)  
- `OnNPCDeath(AOnsetEnemy*)` — called when any single NPC dies; starts its individual respawn timer (future)  

## Data Flow

```mermaid
flowchart TD
    Start[Spawner BeginPlay] --> InitSlots
    InitSlots --> SlotsReady[Slots Ready]
    SlotsReady --> SpawnGroup
    SpawnGroup --> Loop[For Each Empty Slot]
    Loop --> SpawnAtSlot[SpawnEnemyAtSlot]
    SpawnAtSlot --> Profile[Read UAIProfile]
    Profile --> Spawn[FActorSpawnParameters]
    Spawn --> RegisterGroup[Register with GroupManagerComponent]
    RegisterGroup --> Occupied[Slot Occupied]

    Occupied --> NPCDies{NPC Dies}
    NPCDies -->|Path 1| ReturnPool[ReturnToPool]
    NPCDies -->|Path 2| SpawnCorpse[SpawnCorpse]
    NPCDies -->|Path 3| StartTimer[Start Respawn Timer]
    ReturnPool --> Pool[Inactive Pool]
    SpawnCorpse --> Corpse[Corpse Actor]
    StartTimer --> ClearSlot
    ClearSlot --> SpawnGroup

`AOnsetSpawner.InitSlots()` → `FSpawnerSlot[]` → `SpawnEnemyAtSlot(i)` → `UOnsetPoolSubsystem.GetPooledEnemy()` → `AIController->ApplyAIProfile(UAIProfile)` → `AIController->ApplyPerceptionProfile(UPerceptionProfile)` → `AIController->Possess(Enemy)` → `UGroupManagerComponent.RegisterMember()`

## Death Flow
When an NPC dies (health ≤ 0), three parallel paths execute:
1. **Pool Return** — `UOnsetPoolSubsystem.ReturnToPool()`: ungroup → unpossess → remove GEs → clear visuals → hide.
2. **Corpse Spawn** — A lightweight `AOnsetCorpse` appears at the death location for visual persistence. See the [Corpse System](Corpse_System.md).
3. **Respawn Timer** — The spawner starts a per-slot timer. When it fires, `SpawnEnemyAtSlot()` reuses the same slot.

The respawn timer is **independent** of the corpse despawn timer. The slot can refill even if the corpse from the previous occupant is still visible.

## Interactions
- **[Pooling System](Pooling_System.md):** requests NPC+controller instances via `UOnsetPoolSubsystem.GetPooledEnemy()` / `GetPooledController()`; `ReturnToPool` handles full reset  
- **[Corpse System](Corpse_System.md):** spawner's death notification is sent at the same time as the corpse spawn — the two are parallel, not sequential  
- **[Group System](Group_System.md):** registers members into groups via `UGroupManagerComponent`  
- **[NPC AI System](NPC_AI_System.md):** three profiles (`UVisualProfile`, `UAIProfile`, `UPerceptionProfile`) configure pawn visuals, controller StateTree, and perception  
- **Final Demo Loop:** may trigger waves via spawners  

## Replication
- Spawner logic is **server‑only**  
- Spawner state (active/inactive) may replicate if needed for UI  

## Edge Cases
- Spawner disabled mid-respawn — pending timers should cancel  
- No pooled NPCs available — retry after a short delay or queue the respawn  
- Respawn while player is too close (optional rule)  
- Multiple NPCs die simultaneously — each gets its own independent timer  

## Testing Checklist
- [ ] Spawns correct number and type of NPCs  
- [ ] Slot transforms match spawn points or fallback scatter  
- [ ] Groups are registered correctly via `UGroupManagerComponent`  
- [ ] Individual NPC respawns on its own timer after death  
- [ ] Respawn timers are independent — killing multiple NPCs does not cascade  
- [ ] Respawn timer starts immediately on death, not after corpse despawn  
- [ ] DebugKillLast works  
- [ ] Works with pooling — pool pre-allocates, spawner always retrieves from pool  
- [ ] Works in multiplayer (server‑only logic)  

---