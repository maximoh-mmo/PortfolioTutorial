# 🎬 Post 14 — The Cloud Spine: Auth Extraction, Lambda Accounts, and the Two-Server Token Flow

*14 commits · Jul 27–28, 2026 · covering Sprint A5c Auth Extraction & Login Server*

---

## Introduction

Post 11 proved the game can run multiplayer with Steam tickets. Post 12 proved it can persist accounts. Post 13 proved it can render menus in packaged builds. But there's a gap: the auth logic is still baked into the game server, and the game server is still the only server. What if we separate auth from gameplay entirely?

Sprint A5c does exactly that. It extracts the Steam auth flow into a standalone login server, adds a PostgreSQL-backed account API (Lambda + DynamoDB), and implements the two-server token architecture that makes the game ready for production. Fourteen commits, two days, and a server infrastructure that would be recognizable to any live-service team.

---

## Technical Exposition

### Why extract auth?

The Steam auth flow in Post 11 was server-side — the game server validated tickets, issued tokens, and managed sessions. This works for a demo, but it couples auth to the game server in ways that don't scale. A production game needs auth to be independent: the login server handles tickets and issues tokens; the game server validates tokens and loads account data. If the game server goes down, players are still logged in. If the login server goes down, players can still play offline.

### The Lambda + DynamoDB account API

The account API is a serverless function (Lambda) backed by DynamoDB. It handles account creation, login, and character data retrieval. The Lambda is invoked via HTTP from the login server, which caches results in PostgreSQL for the game server to query.

The Lambda uses `FHttpStore` (the HTTP backend from Post 12's `IPlayerDataStore` interface) to communicate with DynamoDB. The same interface that supports SQLite and PostgreSQL now supports HTTP — the abstraction works.

### The two-server token flow

The token is the linchpin. It carries the `SlotIndex` so the game server knows which character to spawn. The flow is:

1. Client authenticates with Steam → gets a ticket
2. Client sends ticket to login server → login server validates with Steam → issues a signed token
3. Client sends token to game server → game server validates token signature → loads account data from PostgreSQL → spawns the correct character

The token is signed with HMAC-SHA256 using a secret key shared between the login server and game server. This is the same HMAC implementation from Post 11's `FSHA256` — the custom pure-software SHA-256 that works on all platforms.

---

## Implementation Details

### 1. The login server

The login server is a standalone process that handles Steam auth and token issuance. It's the same architecture as the game server (UE executable with `-server` flag) but with a different GameMode and map:

```cpp
// OnsetLoginServer.Target.cs
using CSharpForUE.BuildSystem;
BuildTarget LoginServer = new BuildTarget("OnsetLoginServer", TargetType.Server);
```

The login server runs on port 7777 (Direct mode for lobby), validates Steam tickets, and issues tokens. The game server runs on port 7778 (Token mode), validates tokens, and loads account data.

### 2. The PostgreSQL account table

```sql
CREATE TABLE accounts (
    platform TEXT NOT NULL,
    platform_id TEXT NOT NULL,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW(),
    PRIMARY KEY (platform, platform_id)
);

CREATE TABLE characters (
    id SERIAL PRIMARY KEY,
    account_platform TEXT NOT NULL REFERENCES accounts(platform),
    account_platform_id TEXT NOT NULL REFERENCES accounts(platform_id),
    slot_index INTEGER NOT NULL,
    class_name TEXT NOT NULL DEFAULT 'Hero_N',
    display_name TEXT NOT NULL DEFAULT 'Hero',
    appearance JSONB DEFAULT '{}',
    created_at TIMESTAMPTZ DEFAULT NOW(),
    UNIQUE (account_platform, account_platform_id, slot_index)
);
```

The schema mirrors the SQLite schema from Post 12 but uses PostgreSQL's JSONB type for appearance data and serial IDs for characters. The `UNIQUE` constraint on `(account_platform, account_platform_id, slot_index)` prevents double-creation.

### 3. The Lambda function

The Lambda function is a Python script that handles account operations. It's invoked via HTTP from the login server:

```python
def lambda_handler(event, context):
    operation = event.get('operation')
    if operation == 'login':
        return handle_login(event)
    elif operation == 'create_account':
        return handle_create_account(event)
    elif operation == 'get_character':
        return handle_get_character(event)
    elif operation == 'save_character':
        return handle_save_character(event)
```

The Lambda connects to DynamoDB using the AWS SDK, performs the requested operation, and returns the result as JSON. The login server caches Lambda responses in PostgreSQL for the game server to query.

### 4. The token format

The token is a JSON Web Token (JWT) signed with HMAC-SHA256:

```cpp
FString Token = FString::Printf(TEXT("{\"platform\":\"%s\",\"platform_id\":\"%s\",\"slot_index\":%d,\"exp\":%lld}"),
    *Platform, *PlatformID, SlotIndex, ExpiryTimestamp);
FString Signature = ComputeHMACSHA256(Token, SecretKey);
return Token + TEXT(".") + Signature;
```

The game server validates the token by recomputing the HMAC and comparing signatures. If the signature matches and the token hasn't expired, the token is valid.

### 5. The two-server architecture

The architecture separates concerns cleanly:

| Server | Port | Mode | Purpose |
|---|---|---|---|
| Login Server | 7777 | Direct | Steam auth, token issuance |
| Game Server | 7778 | Token | Gameplay, token validation, account loading |

The login server is a thin, disposable front door. It validates tickets and issues tokens. The game server is platform-agnostic — it validates tokens and loads account data. This separation means the login server can be replaced or scaled independently of the game server.

### 6. API access control hardening

The Lambda function includes access control checks:

```python
def handle_login(event):
    platform = event.get('platform')
    platform_id = event.get('platform_id')
    if not platform or not platform_id:
        return {'statusCode': 400, 'body': 'Missing platform or platform_id'}
    if platform not in ALLOWED_PLATFORMS:
        return {'statusCode': 403, 'body': 'Platform not supported'}
    # ... proceed with login
```

The `ALLOWED_PLATFORMS` list restricts which platforms can authenticate. This prevents unauthorized platforms from creating accounts or accessing character data.

---

## Results & Validation

- Login server extracted from game server (standalone process, port 7777)
- Game server runs on port 7778 with Token mode
- PostgreSQL account table with characters, appearance JSONB, UNIQUE constraints
- Lambda + DynamoDB account API with 4 operations (login, create_account, get_character, save_character)
- JWT token format with HMAC-SHA256 signing
- Two-server token flow: Steam → Login Server → Token → Game Server → PostgreSQL
- API access control hardening in Lambda (platform allowlist, input validation)
- `FHttpStore` backend for Lambda + DynamoDB (completes `IPlayerDataStore` interface)
- A5c = 38/38 (100%)

---

## Challenges & Solutions

### The login server needs its own target type

**Problem:** The engine doesn't support a Server target type in shipping builds. The login server can't use the same target as the game server.

**Solution:** The login server uses the same Game target type with `-server` flag, just like the game server's DS. The difference is the GameMode, map, and port.

### DynamoDB eventual consistency

**Problem:** DynamoDB's default read consistency is eventual — a character save might not be immediately visible on a subsequent read.

**Solution:** The Lambda uses `ConsistentRead=true` for all read operations. This ensures that a save is immediately visible on the next read, at the cost of higher read capacity.

### The HMAC secret key management

**Problem:** The HMAC secret key needs to be shared between the login server and game server, but hardcoding it in the source is a security risk.

**Solution:** The secret key is read from an environment variable at runtime. The login server and game server both read `AUTH_SECRET_KEY` from the environment. In production, this is set via the deployment pipeline; in dev, it's a hardcoded fallback with a warning log.

### The Lambda cold start latency

**Problem:** Lambda cold starts add 1-3 seconds of latency to auth operations.

**Solution:** The login server caches Lambda responses in PostgreSQL. The first request hits Lambda (slow), but subsequent requests hit the cache (fast). The cache is invalidated on account changes.

### The `ALLOWED_PLATFORMS` list hardcoded in Lambda

**Problem:** Adding a new platform requires a Lambda code change and redeployment.

**Solution:** The platform list is stored in DynamoDB as a config table. The Lambda reads it at runtime. Adding a new platform means adding a row to the config table — no code change needed.

---

## Reflection & Lessons Learned

The two-server architecture is the arc's most significant structural decision. It separates auth from gameplay in a way that's invisible to the player but transformative for the engineering. The login server is a thin, disposable front door; the game server is platform-agnostic. This separation means the game can scale to multiple game servers behind a single login server — the architecture of a live-service game, built at demo scale.

The `IPlayerDataStore` interface proves its worth again. The Lambda + DynamoDB backend (`FHttpStore`) was added with zero changes to the GameMode or PlayerController — just a new backend class and a config key. The abstraction works exactly as designed.

The JWT token format is simple but effective. HMAC-SHA256 signing with a shared secret is the minimum viable token format for a demo. It's not JWE (encrypted tokens) or JWS (asymmetric signing) — it's symmetric HMAC, which is fast, simple, and secure enough for a demo. The custom `FSHA256` implementation from Post 11 powers the HMAC computation on both servers.

---

## Forward-Looking Content

| Phase | Status |
|---|---|
| A5 — Multiplayer + Steam | ✅ **100% (42/42)** |
| A5b — Persistence & Account System | ✅ **100% (30/30)** |
| A5c — Auth Extraction & Login Server | ✅ **100% (38/38)** |
| CommonUI UI Migration | ✅ **100% (4/4)** |
| Character Classes + Final Polish | ⏭️ Next |

**What's next:** the cloud spine is complete, but the game still needs its character classes and final polish. Post 15 delivers the last major feature arc: character classes (Warrior, Mage, Rogue), loading screens, and API access control hardening. Then the series wraps up with a retrospective and roadmap.

> **Next time in Post 15:** The Final Form — character classes, loading screens, and the polish that turns a demo into a game.

---

*Arc commits: `7ce133a → 717bb90`. Related docs: `Docs/Server/Auth_System.md`, `Docs/Server/Account_API.md`, `Docs/Architecture/Architecture Overview.md`, `Planning/Sprint_A5c_Auth_Extraction.md`.*
