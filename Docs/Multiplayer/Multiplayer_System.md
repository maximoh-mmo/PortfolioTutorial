# 📘 **Multiplayer System**

## Purpose
Make the entire demo **server‑authoritative and multiplayer‑safe**, supporting dedicated servers and Steam‑authenticated sessions.

## Responsibilities
- Define server/client authority rules  
- Ensure AI runs server‑only  
- Ensure abilities and effects replicate correctly  
- Support dedicated server builds  
- Integrate with Steam for authentication  

## Non‑Responsibilities
- Low‑level networking (engine)  
- Matchmaking UI  
- Anti‑cheat  

## Key Concepts
- Server authority over game state  
- Clients send input, server simulates  
- Replication of movement, abilities, health, etc.  
- PvP flag is server‑authoritative and replicated to all clients

## Key Classes
- **`AGameMode` / `AGameState`** — server rules + shared state  
- **`APlayerController` / `APlayerState`** — per‑player state  
- **`ANPCAIController`** — server‑only AI  

## Key Functions
- `Server_` RPCs for client → server requests  
- `Multicast_` RPCs for server → all clients (sparingly)  

## Data Flow
Client Input → Server RPC → Server Simulation → Replication → Client View

## Interactions
- **[NPC AI System](../AI/NPC_AI_System.md):** runs only on server  
- **[GAS System](../GAS/GAS_System.md):** server‑authoritative abilities  
- **[PvP System](../Gameplay/PVP_System.md):** server-authoritative PvP flag replication  
- **[Steam Integration](../Steam/Steam_Integration_System.md):** auth + session validation  

## Replication Rules
- NPCs, abilities, health, and effects replicate  
- Group/assist logic is server‑only  
- Targeting visuals are client‑side; final validation on server  

## Edge Cases
- High latency  
- Packet loss  
- Client disconnects mid‑combat  
- Host migration (out of scope)  

## Testing Checklist
- [ ] Dedicated server runs correctly  
- [ ] Clients can connect and play  
- [ ] AI behaves identically in multiplayer  
- [ ] Abilities replicate correctly  
- [ ] No client‑side authority exploits  

---