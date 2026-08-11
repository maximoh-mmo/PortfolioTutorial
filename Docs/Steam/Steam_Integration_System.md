# 📘 **STEAM INTEGRATION SYSTEM**  
**File:** `/Docs/Steam/Steam_Integration_System.md`

---

# **Steam Integration System**

## **Purpose**
Provide **Steam‑authenticated multiplayer** using the Online Subsystem Steam (OSS), enabling:

- Steam user authentication  
- Auth ticket generation  
- Server‑side ticket verification  
- Steam‑based dedicated server sessions  

This system ensures that all multiplayer interactions are tied to verified Steam identities.

---

## **Responsibilities**
- Initialize the Steam Online Subsystem  
- Retrieve Steam user IDs (`FUniqueNetId`)  
- Generate Steam auth tickets on the client  
- Send auth tickets to the server for verification  
- Validate tickets server‑side  
- Feed the platform identity `(Platform, PlatformID)` to the Account System  
- Support Steam‑authenticated session creation and joining  

---

## **Non‑Responsibilities**
- Matchmaking UI  
- Friends list integration  
- Achievements, stats, cloud saves  
- Anti‑cheat  
- Voice chat  
- Inventory or microtransactions  
- Server registration with Steam's server browser (not implemented)

---

## **Key Classes**

### **`IPlatformAuth` (platform abstraction interface)** — `Onset/Public/Data/IPlatformAuth.h`
- Wraps platform-specific auth validation
- `bool ValidateTicket(APlayerController* PC, const FString& Ticket)` — returns whether the ticket is accepted
- `FString GetPlatformID(APlayerController* PC)` — extracts the platform user ID (e.g. SteamID64 from `PS->GetUniqueId()`)
- `FString GetPlatformName() const` — e.g. `"Steam"`

### **`FSteamAuth`** — `Onset/Public/Data/SteamAuth.h`
- Current `IPlatformAuth` implementation
- `ValidateTicket` accepts any non-empty ticket: **transport-level Steam auth is handled by OnlineSubsystemSteam on connection**; full `BeginAuthSession` validation is deferred until the Steamworks SDK headers are directly accessible from the game module (see the code comment)
- `GetPlatformID` reads `AOnsetPlayerState::GetUniqueId()` → SteamID64 as string
- **Note:** `FSteamAuth` is currently *not instantiated/wired into the auth flow* — the live path goes through `UOnsetAuthSubsystem::ValidateAuthTicket` (below)

### **`UOnsetAuthSubsystem`** (world subsystem, server-only)
- Owns all auth logic: `PreLoginTokenAuth` / `PreLoginDirect`, `HandlePostLogin`, `HandleLogout`
- `ValidateAuthTicket(NewPlayer, AuthTicket)` — accepts any non-empty ticket (empty → kicks the player), clears the auth timeout, stores the ticket on PlayerState
- `GenerateToken` / `ValidateToken` — HMAC-SHA256 session tokens for the two-server flow (see [Multiplayer System](../Multiplayer/Multiplayer_System.md))
- Two modes via `[Onset.Auth] AuthMode`: **Direct** (default) and **Token**
- On login, `HandlePostLogin` resolves `(Platform, PlatformID)` and calls `UOnsetPlayerDataSubsystem` to load/create the account

### **`AOnsetPlayerController`**
- `RequestSteamAuth()` — requests auth ticket via `IOnlineIdentity::GetAuthToken(0)` (`OnsetPlayerController.cpp:60-92`)
- Sends ticket to server via RPC: `Server_SendAuthTicket(const FString& AuthTicket)`
- `OnAuthTimeout()` — fires after 10s if the server hasn't confirmed; **logs an error only** (does not disconnect)

### **`AOnsetPlayerState`**
- `PlayerPlatformID` — replicated `FString` (SteamID64 as string)
- `PlayerPlatform` — replicated `FString` ("Steam")
- `SteamAuthTicket` — **server-only**, never replicated

---

## **Key Functions**

### **Client‑Side**
- `RequestSteamAuth()` — calls `IOnlineIdentity::GetAuthToken(0)`
- `Server_SendAuthTicket(const FString& AuthTicket)` — reliable RPC

### **Server‑Side**
- `UOnsetAuthSubsystem::ValidateAuthTicket(NewPlayer, AuthTicket)` — accepts non-empty tickets, clears timeout, stores ticket on PlayerState; empty ticket → kick
- `HandlePostLogin(NewPlayer)` — resolves platform + platform ID (Steam unique ID or dev identity), loads/creates the account, sends `Client_AccountData`
- `GenerateToken` / `ValidateToken` — HMAC-SHA256 session tokens (Token auth mode)

---

## **Data Flow Diagram**

```mermaid
flowchart TD
    Client[Client] --> Steam[Steam API<br/>GetAuthToken(0)]
    Steam --> Client
    Client --> Server[Server_SendAuthTicket RPC]
    Server --> Sub[UOnsetAuthSubsystem<br/>ValidateAuthTicket]
    Sub -->|empty ticket| Kick[Kick player]
    Sub -->|accepted| ClearTimeout[Clear auth timeout]
    ClearTimeout --> PostLogin[HandlePostLogin<br/>resolve Platform + PlatformID]
    PostLogin --> Account[Account System<br/>LoadAccount / CreateAccount]
```

---

## **Interactions With Other Systems**

### **[Multiplayer System](../Multiplayer/Multiplayer_System.md)**
- Steam auth is required before joining (Direct mode on the login server)
- The two-server flow hands identity forward via HMAC-signed session tokens (`?Token=...`)

### **[Account System](../Player/Account_System.md)**
- Provides the platform identity `(Platform, PlatformID)` that anchors all persistent data
- SteamID extracted from `GetUniqueId()` becomes the primary key for the account row

### **[Player System](../Player/Player_System.md)**
- PlayerState stores Steam ID and platform name for persistence lookup  
- Triggers account load after successful auth  

### **[UI System](../Gameplay/UI_System.md)**
- Displays Steam name in character select  
- Shows connection/auth errors  

---

## **Replication Rules**
- Steam IDs replicate via PlayerState (`PlayerPlatformID` / `PlayerPlatform`)
- Auth tickets **never** replicate (`SteamAuthTicket` is server-only)
- Server‑side validation only

---

## **Edge Cases**
- Steam not running (Null OSS fallback → dev identity)  
- Invalid or expired auth ticket  
- Empty ticket → player kicked  
- Ticket validation timeout — logs after 10s (no disconnect)  
- Client disconnects mid‑auth  

---

## **Testing Checklist**
- [x] Client generates auth ticket  
- [x] Server validates ticket (non-empty check)  
- [ ] Invalid tickets are rejected (needs full `BeginAuthSession` validation)  
- [ ] Dedicated server registers with Steam (not implemented)  
- [x] Clients can join Steam‑authenticated sessions  
- [ ] Works with multiple clients  

---

## **Future Extensions**
- Full `BeginAuthSession` ticket validation (needs Steamworks SDK headers accessible from the game module)  
- Wire `FSteamAuth` into `UOnsetAuthSubsystem`  
- Steam server browser  
- Steam friends list integration  
- Steam achievements  
- Steam stats and leaderboards  
