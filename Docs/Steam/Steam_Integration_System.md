# 📘 **STEAM INTEGRATION SYSTEM**  
**File:** `/Docs/Steam/Steam_Integration_System.md`

---

# **Steam Integration System**

## **Purpose**
Provide **Steam‑authenticated multiplayer** using the Online Subsystem Steam (OSS), enabling:

- Steam user authentication  
- Auth ticket generation  
- Server‑side ticket verification  
- Steam‑authenticated dedicated server sessions  
- Steam‑based server discovery (optional)  

This system ensures that all multiplayer interactions are tied to verified Steam identities.

---

## **Responsibilities**
- Initialize the Steam Online Subsystem  
- Retrieve Steam user IDs and display names  
- Generate Steam auth tickets on the client  
- Send auth tickets to the server for verification  
- Validate tickets server‑side  
- Register dedicated servers with Steam  
- Support Steam‑authenticated session creation and joining  

---

## **Non‑Responsibilities**
- Matchmaking UI  
- Friends list integration  
- Achievements, stats, cloud saves  
- Anti‑cheat  
- Voice chat  
- Inventory or microtransactions  

---

## **Key Classes**

### **`USteamSubsystem` (custom wrapper)**
- Wraps Steam OSS calls  
- Provides clean C++ API for auth and server registration  

### **`APlayerController`**
- Requests auth ticket from Steam  
- Sends ticket to server via RPC  

### **`AGameMode`**
- Validates auth tickets server‑side  
- Rejects invalid clients  

### **`AGameSession`**
- Registers dedicated server with Steam  
- Handles session creation  

---

## **Key Functions**

### **Client‑Side**
- `RequestAuthTicket()`  
- `OnAuthTicketReceived()`  
- `Server_SendAuthTicket(TArray<uint8>)`  

### **Server‑Side**
- `ValidateAuthTicket()`  
- `OnAuthTicketValidated()`  
- `RegisterServerWithSteam()`  

---

## **Data Flow Diagram**

```
Client Login
    │
    ▼
Steam → Generate Auth Ticket
    │
    ▼
Client → Server RPC (Send Ticket)
    │
    ▼
Server → Steam Backend (Validate Ticket)
    │
    ▼
Validation Success → Allow Player
Validation Failure → Kick Player
```

---

## **Interactions With Other Systems**

### **[Multiplayer System](../Multiplayer/Multiplayer_System.md)**
- Steam auth is required before joining  
- Dedicated server uses Steam for registration  

### **[Player System](../Player/Player_System.md)**
- PlayerState stores Steam ID and display name  

### **[UI System](../Gameplay/UI_System.md)**
- Displays Steam name in UI  
- Shows connection/auth errors  

---

## **Replication Rules**
- Steam IDs replicate via PlayerState  
- Auth tickets **never** replicate  
- Server‑side validation only  

---

## **Edge Cases**
- Steam not running  
- Invalid or expired auth ticket  
- Ticket validation timeout  
- Dedicated server fails to register  
- Client disconnects mid‑auth  

---

## **Testing Checklist**
- [ ] Client generates auth ticket  
- [ ] Server validates ticket  
- [ ] Invalid tickets are rejected  
- [ ] Dedicated server registers with Steam  
- [ ] Clients can join Steam‑authenticated sessions  
- [ ] Works with multiple clients  

---

## **Future Extensions**
- Steam server browser  
- Steam friends list integration  
- Steam achievements  
- Steam stats and leaderboards  