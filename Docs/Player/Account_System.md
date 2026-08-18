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
| Direct (dev, no Steam) | `"<host>-<login>-C<client>"` | `"MORPHEUS-maxhe-C1"` |

The `Platform` field enables cross-platform account coexistence without collisions.

When Steam OSS is active, `UOnsetAuthSubsystem::HandlePostLogin` uses the client's SteamID64 as the PlatformID. When Steam is unavailable (Null OSS fallback), the server builds a stable per-client dev ID from the machine name, OS login ID, and a `ClientIndex` passed via the connect URL (`127.0.0.1:7777?ClientIndex=N`, added by `Test_All.ps1`). This yields IDs like `MORPHEUS-maxhe-C1` — stable across restarts and distinct per client instance. If a valid unique ID exists but no `ClientIndex` was provided, the unique ID string is used; `"DEV_" + client network address` only fires when the unique ID is invalid/empty.

### **Character Slots**

Each account has exactly 3 slots (indexed 0-2). Slots are:
- **Empty** — no character data; available for creation
- **Occupied** — has character name, level, and full save data

Slots are independent — deleting slot 1 does not affect slots 0 or 2.

Slot UI is driven in C++ by `UCharacterSlot` (occupied/empty display state, click routing); Blueprints only style it via `BP_OnSlotDataChanged`. The select screen builds slots dynamically into a bound panel container and refreshes on `OnAccountDataChanged`.

### **Save Triggers**

| Trigger | When | What |
|---------|------|------|
| Character creation | First save | Full write |
| Character select | On world entry | Read |
| Level up (future) | XP threshold crossed | Write |
| Inventory change (future) | Pickup/drop/equip | Write |
| Quest stage (future) | Objective complete | Write |
| Death | OnDeath fires | Respawn (no write — position/health are runtime) |
| Disconnect | PawnLeavingGame / EndPlay | Write (via `SaveCharacterPreservingIdentity`) |
| Periodic auto-save | 5-min timer | Write (via `SaveCharacterPreservingIdentity`) |
| Manual save | `Server_SaveCharacter` | Write |
| Auto-combat handoff | `OnAbandonedTimeout` (player AI despawn) | Write |

Combat ephemera (current health, active cooldowns, temporary effects) are **never saved**.

---

## **Key Classes**

### **`AOnsetPlayerState`**
- `PlayerPlatformID` (FString) — platform user ID (e.g., SteamID)
- `PlayerPlatform` (FString) — platform name ("Steam", "Xbox", etc.)
- `SelectedCharacterSlot` (int32) — which slot is active this session

### **`AOnsetPlayerController`**
- `Client_AccountData(FOnsetAccountData)` — receive account overview; broadcasts `OnAccountDataChanged` so open screens refresh in place
- `Client_CharacterData(FOnsetFullCharacterData)` — receive full character on select
- `Server_SelectCharacter(int32 SlotIndex)` — client picks a slot; on the login server this generates a session token and travels to the game server
- `Server_CreateCharacter(int32 SlotIndex, FString Name, EOnsetCharacterClass Class, int32 AppearancePresetIndex)` — create new character; on success auto-selects the new character and enters the world
- `Server_DeleteCharacter(int32 SlotIndex)` — delete an occupied slot
- `Server_SaveCharacter()` — manual save request
- `Client_SaveComplete(bool bSuccess)` — confirm save

### **`AOnsetPlayerCharacter`**
- `ApplySaveData(const FOnsetFullCharacterData&)` — restore position, rotation, attributes
- `BuildSaveData()` → `FOnsetFullCharacterData` — snapshot current state for save

---

## **Key Data Structures**

| Struct | Fields | Purpose |
|--------|--------|---------|
| `FOnsetCharacterSlotData` | SlotIndex, CharacterName, Level, CharacterClass, bOccupied | Account overview (lightweight, no full state) |
| `FOnsetAccountData` | PlatformID, Platform, Slots | Full account sent to client |
| `FOnsetFullCharacterData` | SlotIndex, CharacterName, Level, Experience, CurrentZone, SavedMaxHealth, SavedPosition, SavedRotationYaw, InventoryJSON, EquipmentJSON, QuestsJSON, CharacterClass, AppearanceJSON | Full character state for save/load |

**Inventory/equipment JSON** is produced by `UOnsetInventoryComponent::SerializeInventoryJSON()` / `SerializeEquipmentJSON()` on the pawn. The bag serializes as a stacked-entries array `[{c, r, n}]` where `c` = `EOnsetItemCategory`, `r` = row name, `n` = count (equipment as `[{slot, row}]`). Both restore via `AOnsetBaseCharacter::DeserializeInventoryJSON` on save load. See [Inventory & Loot System](../Inventory/Inventory_System.md).

---

## **Data Flow**

### **Login Flow (HTTP API)**

> **Two-server note:** In the current build, the login steps below happen on the **login server** (port 7777, Direct auth). `Server_SelectCharacter` / `Server_CreateCharacter` generate a session token and the client travels to the **game server** (port 7778, Token auth) via `Client_TravelToGameServer`, where the pawn is spawned and `ApplySaveData` runs in `OnPossess`. See [Multiplayer System](../Multiplayer/Multiplayer_System.md). The diagram below is the flow *within* the login server plus the character-select exchange.

```
Client                          Login Server (GameMode)         Account API (Lambda)
------                          -----------------------         --------------------
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
    │                               │  GenerateToken() + Client_SessionToken
    │                               │  Client_TravelToGameServer(...?Token=...)
    │                               │                                  │
    │  [Loading Screen → travel]    │                                  │
    │                               │                                  │
    │   ...on game server (7778) ...│                                  │
    │                               │  PreLogin validates token        │
    │                               │  OnPossess → LoadCharacter → ApplySaveData
    │   ◄─── Client_CharacterData ──│                                  │
    │                               │                                  │
    │  [Enter World]                │                                  │
```

When using SQLite or PostgreSQL, the store is called directly on the DS (no REST hop). The HTTP API path uses `FHttpStore` which serializes requests as JSON and sends them to the Lambda Function URL.

> **Note (character data travel):** The pawn spawn and `ApplySaveData` restore happen on the **game server** in `AOnsetPlayerController::OnPossess`. The login server only performs auth + account overview + character select/creation, then hands off via the session token.

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
- **[UI System](../Gameplay/UI_System.md)** — CommonUI screen stack, character select/creation screens, C++ character slots, loading screen  

---

## **Replication Rules**

- Account and character data is **never replicated** — it travels via RPCs (Client_ / Server_)  
- `PlayerPlatformID` and `PlayerPlatform` on PlayerState **replicate** (for client display)  
- `SelectedCharacterSlot` is **server-only**  
- `SteamAuthTicket` is **server-only** and never replicated  
- Auth tickets are **never stored** beyond the validation flow  

---

## **Edge Cases**

- **First login (no account)** — auto-create account with an empty slots array (client pads to 3)  
- **First login (account exists, no characters)** — all 3 slots empty, force creation  
- **All 3 slots full** — delete an occupied slot (slot delete button → `Server_DeleteCharacter`) before creating another  
- **Save fails (DB error)** — client receives `Client_SaveComplete(false)`, retry on next trigger  
- **Disconnect during save** — saves are synchronous per-call; a failed write logs and returns false rather than partially committing
- **DS crash** — last auto-save checkpoint survives; at most 5 minutes of progress lost  
- **Identity preservation** — all partial saves (disconnect, auto-save, travel, auto-combat timeout) route through `SaveCharacterPreservingIdentity` so name/level/class are never clobbered by a partial save (see Post 19)
- **Slot deletion** — confirm dialog before deleting a character  

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

- **Character rename** — in-game or on character select screen
- **Last-selected slot auto-pick** — skip character select if only one character
- **Read-only spectator** — login without selecting a character (watch others play)
- **Cross-platform merge** — link multiple platform IDs to one account
