# 📘 **GROUP SYSTEM — SYSTEM DOCUMENT**  
**File:** `/Docs/AI/Group_System.md`

---

# **Group System**

## **Purpose**
The Group System manages **collections of NPCs that behave as a cohesive unit**.  
It provides:

- Group‑level data (center, direction, alive count)  
- Assist behaviour (allies respond when a member is attacked)  
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
- Handle assist logic:
  - When one NPC is attacked  
  - Notify nearby allies  
  - Trigger assist/agro behaviour  
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

### **`AGroupManager`**
One per group.

- Tracks all members  
- Computes group metrics  
- Handles assist events  
- Provides data to StateTrees  

### **`FGroupData`**
Lightweight struct containing:

- `FVector Center`  
- `FVector Direction`  
- `int32 AliveCount`  
- `float AssistRadius`  

---

## **Key Functions**

### **`RegisterMember(ANPC*)`**
Adds NPC to group.

### **`UnregisterMember(ANPC*)`**
Removes NPC from group.

### **`UpdateGroupData()`**
Recalculates center, direction, alive count.

### **`NotifyMemberAttacked(ANPC* Victim, AActor* Instigator)`**
Broadcasts assist event to nearby allies.

### **`GetNearbyAllies(ANPC* Source, float Radius)`**
Returns list of allies within assist radius.

---

## **Data Flow Diagram**

```
NPC Takes Damage
        │
        ▼
GroupComponent → GroupManager
        │
        ▼
Find Nearby Allies
        │
        ▼
Send Assist Event
        │
        ▼
Allies Transition to Agro State
```

---

## **Interactions With Other Systems**

### **[NPC AI System](NPC_AI_System.md)**
- Receives assist events  
- Transitions into Agro state  

### **[Spawner System](Spawner_System.md)**
- Assigns NPCs to groups  
- Creates GroupManagers  

### **[Pooling System](Pooling_System.md)**
- Resets group membership on reuse  

### **StateTree**
- Reads group data (center, direction)  
- Uses assist flag to trigger transitions  

---

## **Replication Rules**
- Group data is **server‑only**  
- Assist events are **server‑only**  
- NPC state changes replicate normally  
- No group‑level replication required  

---

## **Edge Cases**
- NPC dies while allies are assisting  
- Group has only one member  
- NPC is attacked outside assist radius  
- NPC is attacked by another NPC (ignore)  
- GroupManager destroyed before cleanup  

---

## **Testing Checklist**
- [ ] NPCs register/unregister correctly  
- [ ] Group center updates correctly  
- [ ] Assist radius triggers correctly  
- [ ] Allies transition to Agro  
- [ ] No assist when attacker is out of range  
- [ ] Works with pooled NPCs  
- [ ] Works in multiplayer (server‑only logic)  

---

## **Future Extensions**
- Group‑level tactics  
- Group leaders  
- Formation movement  
- Group morale system  

---