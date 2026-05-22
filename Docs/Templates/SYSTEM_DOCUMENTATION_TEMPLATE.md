# 📚 **SYSTEM DOCUMENTATION TEMPLATE**

Below are templates you can use for each system (see the [Architecture Overview](../Architecture/Architecture%20Overview.md) for context on how systems fit together).  
They're structured for clarity and consistency.

---

# **SYSTEM DOCUMENTATION TEMPLATE**

## **System Name**
(e.g., NPC AI System)

---

## **Purpose**
What this system exists to do.

---

## **Responsibilities**
List the responsibilities of the system.

---

## **Non‑Responsibilities**
Clarify what this system *does not* handle.

---

## **Key Classes**
- `ClassName` — description  
- `ClassName` — description  

---

## **Key Data Structures**
- `StructName` — description  

---

## **Key Functions**
- `FunctionName()` — description  

---

## **Data Flow Diagram**
(Optional ASCII diagram)

---

## **Interactions With Other Systems**
Describe how this system communicates with (link to the relevant doc):

- [Player](../Player/Player_System.md)  
- [AI](../AI/NPC_AI_System.md)  
- [GAS](../GAS/GAS_System.md)  
- [Spawner](../AI/Spawner_System.md)  
- [Pooling](../AI/Pooling_System.md)  
- [Multiplayer](../Multiplayer/Multiplayer_System.md)  
- [Steam](../Steam/Steam_Integration_System.md)  

---

## **Replication Rules (if applicable)**
- What replicates  
- What is server‑only  
- What is client‑only  

---

## **State Machines / Behaviour Trees / StateTrees**
Include diagrams or descriptions.

---

## **Edge Cases**
List tricky situations the system must handle.

---

## **Testing Checklist**
- Does X work?  
- Does Y replicate?  
- Does Z reset correctly?  

---

## **Future Extensions**
Ideas for later improvements.

---
