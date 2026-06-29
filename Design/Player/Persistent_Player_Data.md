# 🧩 **PERSISTENT PLAYER DATA — DESIGN DOC**

**File:** `Design/Player/Persistent_Player_Data.md`

---

## **Overview**

This document captures the design decisions, schema ERD, and data flow for the player persistence system. It covers the account/character slot model, the cross-platform auth abstraction, and the save/load lifecycle.

---

## **Schema ERD**

```
┌─────────────────────────────┐
│         accounts            │
├─────────────────────────────┤
│ PK (platform, platform_id)  │──┐
│    platform       TEXT      │  │
│    platform_id    TEXT      │  │
│    created_at     TEXT      │  │
│    last_login     TEXT      │  │
└─────────────────────────────┘  │
                                 │ 1
                                 │
                                 │ N
┌─────────────────────────────┐  │
│         characters          │  │
├─────────────────────────────┤  │
│ PK id                INT   │  │
│ FK (platform, platform_id)  │<-┘
│    slot_index       INT    │  (0-2)
│    name             TEXT   │
│    level            INT    │
│    experience       REAL   │
│    max_health       REAL   │
│    pos_x            REAL   │
│    pos_y            REAL   │
│    pos_z            REAL   │
│    rot_yaw          REAL   │
│    inventory_json   TEXT   │
│    equipment_json   TEXT   │
│    quests_json      TEXT   │
│    play_time_sec    INT    │
│    created_at       TEXT   │
│    saved_at         TEXT   │
└─────────────────────────────┘
```

---

## **Platform Auth Abstraction**

Every platform follows the same token-exchange pattern. Only the SDK calls differ:

```
Client                          Server
──────────────────────────────────────────────────
1. Request identity token       │
   (Steam: GetAuthTicket)       │
   (Xbox: GetTokenAndSignature) │
   (PSN: NP GetAuthorizationCode)
                                │
2. Send token to server ──────► │
   Server_Validate(Token)       │
                                │
                                │ 3. Validate token
                                │    (Steam: BeginAuthSession)
                                │    (Xbox: VerifyToken)
                                │    (PSN: VerifyCode)
                                │
                                │ 4. Extract platform user ID
                                │    (all return string IDs)
                                │
                                │ 5. Load account by (platform, platform_id)
                                │    or auto-create if first login
```

The interface is minimal:

```cpp
struct FPlatformAuthResult {
    bool bSuccess;
    FString PlatformID;
    FString ErrorMessage;
};

class IPlatformAuth {
public:
    virtual FString GetPlatformName() const = 0;
    virtual FPlatformAuthResult ValidateTicket(const FString& AuthTicket) = 0;
};
```

Current implementation: **Steam** (via `SteamGameServer()->BeginAuthSession()`).  
Future implementations: **Xbox Live**, **PSN**, **Nintendo Account** — each adds one `.cpp` file.

---

## **Data Flow Diagram**

```
Client                     DS (GameMode)              UOnsetPlayerDataSubsystem        IPlayerDataStore
──────                     ──────────────             ─────────────────────────        ───────────────
  │                            │                              │                            │
  │── Connect ───────────────► │                              │                            │
  │                            │                              │                            │
  │── Auth Ticket ───────────► │                              │                            │
  │                            │── BeginAuthSession ────────► │                            │
  │                            │  ← SteamID (uint64) ─────── │                            │
  │                            │                              │                            │
  │                            │── LoadAccount(Steam, ID) ──►│                            │
  │                            │                              ├── SELECT * FROM accounts ──►│
  │                            │                              │  ← row or empty ───────────│
  │                            │                              │                            │
  │                            │  [if no account:             │                            │
  │                            │   CreateAccount(Steam, ID)] ─┤── INSERT INTO accounts ──►│
  │                            │                              │                            │
  │◄── Client_AccountData ────│                              │                            │
  │                            │                              │                            │
  │ [Character Select]         │                              │                            │
  │                            │                              │                            │
  │── Server_SelectSlot(1) ──►│                              │                            │
  │                            │── LoadCharacter(Steam,ID,1)►│                            │
  │                            │                              ├── SELECT * FROM chars ────►│
  │                            │                              │  ← full row ──────────────│
  │                            │                              │                            │
  │                            │  [Spawn pawn at saved pos]   │                            │
  │                            │  [Apply saved attributes]    │                            │
  │◄── Client_CharacterData ──│                              │                            │
  │                            │                              │                            │
  │ [Enter World]              │                              │                            │
  │                            │                              │                            │
  │   ... gameplay ...         │                              │                            │
  │                            │                              │                            │
  │── Server_SaveCharacter ──►│                              │                            │
  │                            │── SaveCharacter ───────────►│                            │
  │                            │                              ├── UPDATE characters SET ──►│
  │                            │                              │  ← done ─────────────────│
  │◄── Client_SaveComplete ───│                              │                            │
```

---

## **Versioning Strategy**

The `inventory_json`, `equipment_json`, and `quests_json` blobs each include a `version` field:

```json
{
    "version": 1,
    "items": [...]
}
```

When a system reads a blob and finds an older version, it runs an in-memory upgrade before populating the runtime data structures. The DB schema never needs to change for these systems.

The `_schema_version` table handles structural DB changes (adding columns, new tables) via sequential integer migrations.

---

## **Thread Safety**

SQLite is used in **single-connection mode** on the DS game thread. All access is serialized by the `UOnsetPlayerDataSubsystem` — no concurrent read/write from separate threads in v1.

For future scalability with PgSQL, the `IPlayerDataStore` interface can be extended with async delegates, but the synchronous API keeps the initial implementation simple and correct.

---

## **Config File Format**

```ini
[Onset.DataStore]
; Available stores: SQLite, Postgres
DataStore=SQLite

; SQLite: relative or absolute path to the DB file
SQLitePath=../../OnsetDB/playerdata.db

; PostgreSQL: libpq connection string (ignored if DataStore=SQLite)
; PgSQLConnectionString=host=localhost dbname=onset user=onset password=onset
```

---

## **Key Decisions Summary**

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Store interface | Pure virtual `IPlayerDataStore` | Swap backends without touching gameplay code |
| Embedded DB | SQLite + WAL | Zero dependencies, ACID, portable |
| Production DB | PostgreSQL | Industry standard, libpq available |
| Platform identity | (platform, platform_id) composite PK | Cross-platform without collisions |
| Character slots | 3 per account | User requirement |
| Future data | JSON blobs with version field | Schema stable while systems evolve |
| Save timing | On-trigger + 5-min timer + disconnect | No per-tick writes, max 5m loss on crash |
| Auth abstraction | `IPlatformAuth` interface | One `.cpp` per platform |
