# 📘 **Account System**

**File:** `Docs/Player/Account_System.md`

---

## **Purpose**

Provide persistent player identity and progression across play sessions. Each Steam account owns up to 3 character slots; characters persist between DS restarts and survive crashes. The account backend is swappable — SQLite (dev), PostgreSQL (self-hosted), or HTTP REST API (Lambda + DynamoDB, live-service).

---

## **Responsibilities**

- Store and retrieve player account data linked to a platform identity (SteamID, Xbox Live ID, etc.)  
- Manage up to 3 character slots per account  
- Serialize character state (level, XP, position, equipment, inventory, quests)  
- Provide save-on-disconnect and periodic auto-save  
- Expose save/load RPCs for client interaction  

---

## **Non‑Responsibilities**

- Combat state (health resets on login)  
- Real-time per-tick persistence  
- Anti-cheat (server-authoritative storage provides integrity inherently)  
- Matchmaking or session management  

---

## **Key Concepts**

### **Platform Identity**

The system uses a composite key `(Platform, PlatformID)` to identify accounts:

| Platform | ID Format | Example |
|----------|-----------|---------|
| Steam | `uint64` as string | `"76561197960265728"` |
| Xbox (future) | XUID | `"XUID1234567890"` |
| PSN (future) | Account ID | `"PSN00000001"` |
| Switch (future) | Nintendo Account ID | `"NA00000001"` |

The `Platform` field enables cross-platform account coexistence without collisions.

### **Character Slots**

Each account has exactly 3 slots (indexed 0-2). Slots are:
- **Empty** — no character data; available for creation
- **Occupied** — has character name, level, and full save data

Slots are independent — deleting slot 1 does not affect slots 0 or 2.

### **Save Triggers**

| Trigger | When | What |
|---------|------|------|
| Character creation | First save | Full write |
| Character select | On world entry | Read |
| Level up (future) | XP threshold crossed | Write |
| Inventory change (future) | Pickup/drop/equip | Write |
| Quest stage (future) | Objective complete | Write |
| Death | OnDeath fires | Write |
| Disconnect | EndPlay / Logout | Write |
| Periodic auto-save | 5-min timer | Write |
| Manual save | Save point in world | Write |

Combat ephemera (current health, active cooldowns, temporary effects) are **never saved**.

---

## **Key Classes**

### **`AOnsetPlayerState`**
- `PlayerPlatformID` (FString) — platform user ID (e.g., SteamID)
- `PlayerPlatform` (FString) — platform name ("Steam", "Xbox", etc.)
- `SelectedCharacterSlot` (int32) — which slot is active this session

### **`AOnsetPlayerController`**
- `Client_AccountData(FOnsetAccountData)` — receive account overview
- `Client_CharacterData(FOnsetFullCharacterData)` — receive full character on select
- `Server_SelectCharacter(int32 SlotIndex)` — client picks a slot
- `Server_CreateCharacter(int32 SlotIndex, FString Name)` — create new character
- `Server_SaveCharacter()` — manual save request
- `Client_SaveComplete(bool bSuccess)` — confirm save

### **`AOnsetPlayerCharacter`**
- `ApplySaveData(const FOnsetFullCharacterData&)` — restore position, rotation, attributes
- `BuildSaveData()` → `FOnsetFullCharacterData` — snapshot current state for save

---

## **Key Data Structures**

| Struct | Fields | Purpose |
|--------|--------|---------|
| `FOnsetCharacterSlotData` | SlotIndex, CharacterName, Level, bOccupied | Account overview (lightweight, no full state) |
| `FOnsetAccountData` | PlatformID, Platform, Slots[3] | Full account sent to client |
| `FOnsetFullCharacterData` | SlotIndex, CharacterName, Level, XP, MaxHealth, Position, RotationYaw, InventoryJSON, EquipmentJSON, QuestsJSON | Full character state for save/load |

---

## **Data Flow**

### **Login Flow (HTTP API)**

```
Client                          DS (GameMode)                   Account API (Lambda)
------                          ---------------                 --------------------
    │                               │                                  │
    ├── Connect ──────────────────► │                                  │
    │                               │  Steam Auth handshake            │
    │                               │  ValidateAuthTicket() → SteamID  │
    │   ◄─── Client_ClearAuthTimeout│                                  │
    │                               │                                  │
    │                               │  PostLogin:                      │
    │                               │  FHttpStore::LoadAccount() ────► │  GET /account/Steam/{id}
    │                               │    ◄── 404 (first time) ─────── │
    │                               │  FHttpStore::CreateAccount() ──► │  POST /account/Steam/{id}
    │                               │    ◄── 201 Created ──────────── │
    │                               │  FHttpStore::LoadAccount() ────► │  GET /account/Steam/{id}
    │                               │    ◄── 200 + account data ────  │
    │   ◄─── Client_AccountData ─── │                                  │
    │                               │                                  │
    │  [Character Select Screen]    │                                  │
    │                               │                                  │
    ├── Server_SelectCharacter(1) ► │                                  │
    │                               │  FHttpStore::LoadCharacter() ──► │  GET /account/Steam/{id}/character/1
    │                               │    ◄── 200 + character data ─── │
    │                               │  Spawn AOnsetPlayerCharacter     │
    │                               │  ApplySaveData(CharacterData)    │
    │   ◄─── Client_CharacterData ── │                                  │
    │                               │                                  │
    │  [Enter World]                │                                  │
```

When using SQLite or PostgreSQL, the store is called directly on the DS (no REST hop). The HTTP API path uses `FHttpStore` which serializes requests as JSON and sends them to the Lambda Function URL.

### **Save Flow**

```
PlayerController           UOnsetPlayerDataSubsystem       IPlayerDataStore
───────────────            ─────────────────────────       ───────────────
     │                              │                            │
     │  Save trigger fires          │                            │
     │  (level-up, death,           │                            │
     │   disconnect, 5-min          │                            │
     │   timer)                     │                            │
     │                              │                            │
     ├── CharacterData ───────────► │                            │
     │   from pawn snapshot         │                            │
     │                              ├── SaveCharacter() ──────► │
     │                              │                            │  SQL: UPDATE ...; COMMIT;
     │                              │   ◄──── success ────────── │
     │   ◄─── SaveComplete ──────── │                            │
```

---

## **Interactions With Other Systems**

- **[Persistence Data Store](../Server/Persistence_Data_Store.md)** — `IPlayerDataStore` interface used for all reads/writes; `FHttpStore` proxies to the [Account API](../Server/Account_Api.md) (Lambda + DynamoDB) when `Type=HttpApi`  
- **[Steam Integration](../Steam/Steam_Integration_System.md)** — provides the SteamID that anchors the account  
- **[Multiplayer](../Multiplayer/Multiplayer_System.md)** — all RPCs are server-authenticated and replicated  
- **[Player System](../Player/Player_System.md)** — character pawn loads/saves state via `ApplySaveData` / `BuildSaveData`  
- **[UI System (future)](../Gameplay/UI_System.md)** — character select screen, save indicators  

---

## **Replication Rules**

- Account and character data is **never replicated** — it travels via RPCs (Client_ / Server_)  
- `PlayerPlatformID` and `PlayerPlatform` on PlayerState are **server-only** (not replicated)  
- `SelectedCharacterSlot` is **server-only**  
- Auth tickets are **never stored** beyond the validation flow  

---

## **Edge Cases**

- **First login (no account)** — auto-create account with 3 empty slots  
- **First login (account exists, no characters)** — all 3 slots empty, force creation  
- **All 3 slots full** — must delete a slot before creating another (not implemented in Wave 4 — show dialog)  
- **Save fails (DB error)** — client receives `Client_SaveComplete(false)`, retry on next trigger  
- **Disconnect during save** — transaction safety: partial write rolls back  
- **DS crash** — last auto-save checkpoint survives; at most 5 minutes of progress lost  
- **Slot deletion** — confirm dialog before deleting a character (deferred)  

---

## **Testing Checklist**

- [ ] First-time login auto-creates account
- [ ] Character creation fills a slot
- [ ] Character selection loads correct data
- [ ] Position/rotation restored correctly on enter world
- [ ] Save-on-disconnect persists latest state
- [ ] 5-min auto-save persists state mid-session
- [ ] MaxHealth attribute restored on login
- [ ] Empty slots cannot be selected
- [ ] Occupied slots show correct name + level
- [ ] Save after level-up (future) updates XP/level
- [ ] RPCs validated server-side (no client spoofing)
- [ ] DB survives DS restart (read-after-reboot)

---

## **Future Extensions**

- **Slot deletion** — confirm dialog, remove from DB
- **Character rename** — in-game or on character select screen
- **Appearance save data** — mesh, material, color choices
- **Last-selected slot auto-pick** — skip character select if only one character
- **Read-only spectator** — login without selecting a character (watch others play)
- **Cross-platform merge** — link multiple platform IDs to one account
