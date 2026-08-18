## Corpse System — `/Docs/AI/Corpse_System.md`

# Corpse Actor System

## Purpose
Decouple world-debris (corpses) from high-cost AI/ASC actors. When an NPC dies, the `AOnsetEnemy` returns to the pool immediately while a lightweight `AOnsetCorpse` actor persists at the death location for visual feedback, timed cleanup, and loot-container duty. Corpses host a replicated inventory component; players loot them by clicking.

## Responsibilities
- Spawn a minimal corpse actor at the NPC's death location
- Freeze the NPC's last pose (static mesh or frozen skeletal mesh)
- Roll the NPC's loot table (`DT_Loot`) into a replicated `UOnsetInventoryComponent` on death
- Clean up the corpse after a configurable lifespan; empty-loot corpses auto-expire after 4s
- Act as a click-to-loot container (range-based auto-path via `UInteractionComponent`)
- Maintain a hard global cap on simultaneous corpses to prevent world-bloat

## Non‑Responsibilities
- AI behaviour or combat logic
- Object pooling internals
- Respawning or spawner timers (spawner starts its timer on NPC death, not corpse despawn)
- Bag/equipment logic (see [Inventory & Loot System](../Inventory/Inventory_System.md))
- Loot overlay rendering (see [UI System](../Gameplay/UI_System.md))

## Key Classes
- **`AOnsetCorpse`** — minimal `AActor` subclass with a static mesh component, a `UOnsetInventoryComponent` (loot contents, replicated to all clients), a replicated `bLooted` flag (server-authoritative double-loot guard), and timed self-destruct. No Tick unless chasing a fade-out animation.
- **`UOnsetCorpseSubsystem`** — `UWorldSubsystem` that manages the corpse cap (`SweepDeadCorpses` destroys oldest when the cap is reached), tracks active corpses, and spawns them via `SpawnCorpse(Transform, CorpseMesh)`.
- **`UOnsetLootLibrary`** — rolls `DT_Loot` rows into `TArray<FOnsetInventoryEntry>` (level/zone gating, sub-table expansion, drop chance, quantity ranges).
- **`UInteractionComponent`** — click-to-loot branch: `TryLootCorpse` (range check → loot or auto-path + 0.2s arrival poll), `LootCorpse` (transfer → overlay RPC → `bLooted` → destroy).

## Data Flow

```
NPC Health <= 0 (PostGameplayEffectExecute)
  │
  ├──> ReturnToPool() → NPC recycled (AI + ASC freed)
  │
  └──> SpawnCorpse(DeathLocation, DeathPoseData)
         │
         ├──> Corpse->InventoryComponent->SetItems(RollLoot(Stats->LootTable, Context))
         ├──> Empty loot → SetLifeSpan(4.0f)  (auto-expire)
         ├──> Loot contents replicate to all clients
         │
         └──> Player clicks corpse → UInteractionComponent::TryLootCorpse
                ├── In LootRange (250u)? → LootCorpse
                │     ├── PawnInventory->AddItems(Loot)
                │     ├── Client_ShowLootOverlay(Loot)   [client RPC]
                │     ├── bLooted = true
                │     └── Destroy()
                └── Out of range → auto-path to corpse, poll every 0.2s until in range
```

## Interactions
- **[GAS System](../GAS/GAS_System.md):** Fires death event — the two parallel paths (pool return + corpse spawn) are triggered from `PostGameplayEffectExecute` when `Health <= 0`
- **[Pooling System](Pooling_System.md):** The NPC returns to the pool immediately on death, independent of the corpse timeline — this is the core performance benefit
- **[Spawner System](Spawner_System.md):** Spawner receives the death notification at the same time as the corpse spawn — the respawn timer is not blocked by corpse cleanup
- **[Inventory & Loot System](../Inventory/Inventory_System.md):** Corpse hosts the loot container; `OnDeath` rolls the loot table; click-to-loot consumes it
- **[Player System](../Player/Player_System.md):** `UInteractionComponent::ProcessPrimaryInteraction` branches to `TryLootCorpse` on corpse clicks
- **[UI System](../Gameplay/UI_System.md):** `Client_ShowLootOverlay` popup lists the just-looted items

## Design Decisions
- **Two-tier architecture:** High-cost AI actors (StateTree, Perception, ASC) recycle instantly; cheap visual debris (static mesh, no Tick) handles persistence. This is a common MMO/looter-shooter pattern (Borderlands, Destiny).
- **Corpse lifespan vs pool exhaustion:** A global cap on active corpses prevents edge cases where rapid killing outpaces despawn timers. When the cap is reached, the oldest corpse is destroyed immediately.
- **Loot on the corpse, not the NPC:** The NPC returns to the pool instantly, so loot must live on the corpse actor. The corpse inventory replicates to all clients (owner-only disabled) so every player sees the same drops.
- **Click-to-loot with auto-path:** Clicking a corpse needs no range gate (consistent with click-to-target); loot is attempted immediately when in range and auto-paths + polls otherwise. Empty corpses expire in 4s but still despawn on click.
- **Replicated `bLooted` guard:** Server-authoritative flag prevents double-looting across players/clients.
- **No replication of the corpse actor itself beyond standard actor replication:** contents and looted state replicate as component/actor properties.

## Edge Cases
- Corpse cap reached — destroy oldest corpse before spawning new one
- NPC dies off-navmesh (no valid corpse location) — use last valid location
- NPC returns to pool while player is interacting with its corpse (corpse ref is independent, safe)
- Rapid death cascade (AoE kills 10 NPCs) — each spawns its own corpse; cap enforcement kicks in
- Pool exhaustion during corpse flood — corpses don't use the pool; they use direct SpawnActor with timed Destroy
- Empty loot table / table not loaded — corpse gets no items and uses the 4s expire path
- Two players click the same corpse — `bLooted` guard; first looter wins, corpse destroyed
- Player out of range clicking a corpse — auto-path via `PendingMoveTarget`, 0.2s arrival poll
- Player dead/no pawn while polling — `TryLootCorpse` guards on pawn validity

## Testing Checklist
- [ ] Corpse spawns at NPC death location
- [ ] NPC returns to pool immediately (hidden, collision off, tick off)
- [ ] Corpse despawns after configured lifespan
- [ ] Empty-loot corpse auto-expires after 4s
- [ ] Corpse cap is enforced (oldest removed when cap reached)
- [ ] Respawning functions correctly (spawner timer independent of corpse)
- [ ] NPC death rolls loot into corpse inventory and replicates to all clients
- [ ] Click-to-loot in range transfers loot, fires overlay, marks `bLooted`, destroys corpse
- [ ] Click-to-loot out of range auto-paths and loots on arrival
- [ ] Double-click / second player cannot double-loot a corpse
- [ ] No memory leaks under heavy death load
