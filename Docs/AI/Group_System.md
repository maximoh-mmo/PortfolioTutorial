# 📘 **GROUP SYSTEM — SYSTEM DOCUMENT**  
**File:** `/Docs/AI/Group_System.md`

---

# **Group System**

## **Purpose**
The Group System manages **collections of NPCs that behave as a cohesive unit**.  
It provides:

- Group‑level data (center, direction, alive count)  
 - Attack alerts (all members notified when one is attacked; response handled by AI Perception)  
- Cohesion for roaming behaviour  
- A lightweight communication layer between NPCs  

This system is the backbone of believable group AI.

---

## **Responsibilities**
- Assign NPCs to groups on spawn  
- Track all members of a group  
- Compute group‑level metrics:
  - Group center  
  - Group forward direction  
  - Alive count  
- Broadcast attack notifications when a member is attacked:
  - Notify all group members (no distance filter)  
  - Individual response (agro, assist, ignore) determined per-NPC via AI Perception hearing  
- Provide group data to NPC StateTrees  
- Reset group state when NPCs return to pool  

---

## **Non‑Responsibilities**
- Individual NPC behaviour  
- Combat logic  
- Perception (sight/hearing)  
- Navigation  
- Animation  
- Spawning (handled by [Spawner System](Spawner_System.md))  

---

## **Key Classes**

### **`UGroupComponent`**
Attached to each NPC.

- Stores group ID  
- Reference to group manager  
- Provides access to group data  
- Handles registration/unregistration  

### **`UGroupManagerComponent`**
Hosted on `AOnsetSpawner`.

- Tracks all members  
- Computes group metrics  
- Broadcasts attack notifications to all group members  
- Provides data to StateTrees  

### **`FGroupData`**
Lightweight struct containing:

- `FVector Center`  
- `int32 AliveCount`  

---

## **Key Functions**

### **`RegisterMember(AOnsetEnemy*)`**
Adds NPC to group.

### **`UnregisterMember(AOnsetEnemy*)`**
Removes NPC from group.

### **`GetGroupData()`**
Returns center + alive count (computed on demand).

### **`GetNearbyAllies(AOnsetEnemy* Source, float Radius)`** *(private helper)*
Returns members within `Radius` of `Source`. Used internally for cohesion queries (A3.3 Roam). Not part of the assist flow.  

---

## **Data Flow Diagram**

```
NPC Takes Damage
        │
        ▼
OnHit → UAIPerceptionSystem::ReportEvent(FAINoiseEvent)
        │
        ▼
UAIPerceptionSystem distributes to all AIControllers
within range (filtered by each receiver's HearingRange)
        │
        ▼
OnPerceptionUpdated → is noise maker an ally?
        │
        ▼
Set StateTree assist flag → Agro transition
```

> **Note:** The Group System itself does not emit noise events or filter by range. It only provides group membership for the group-identity check in `OnPerceptionUpdated`.  

---

## **Interactions With Other Systems**

### **[NPC AI System](NPC_AI_System.md)**
- Uses group membership to identify allies in `OnPerceptionUpdated`  
- Group System does not trigger AI state transitions directly — that flows through AI Perception  

### **[Spawner System](Spawner_System.md)**
- Assigns NPCs to groups  
- Hosts a `UGroupManagerComponent`  

### **[Pooling System](Pooling_System.md)**
- Resets group membership on reuse  

### **StateTree**
- Reads group data (center, direction)  
- Uses assist flag to trigger transitions  

---

## **Replication Rules**
- Group data is **server‑only**  
- Group notifications are **server‑only**  
- NPC state changes replicate normally  
- No group‑level replication required  

---

## **Edge Cases**
- NPC dies while allies are assisting  
- Group has only one member  
- NPC is attacked by another NPC (ignore)  
- GroupManagerComponent destroyed before cleanup  

---

## **Testing Checklist**
- [ ] NPCs register/unregister correctly  
- [ ] Group center updates correctly  
- [ ] Works with pooled NPCs  
- [ ] Works in multiplayer (server‑only logic)  

---

## **Future Extensions**
- Group‑level tactics  
- Group leaders  
- Formation movement  
- Group morale system  

---