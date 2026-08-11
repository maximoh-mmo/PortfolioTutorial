# ☁️ **Account API (Serverless)**

**File:** `Docs/Server/Account_Api.md`

---

## **Purpose**

Serverless REST backend for player accounts and characters. `FHttpStore` (`[Onset.DataStore] Type=HttpApi`) proxies all data-store calls to this API instead of a local SQLite/PostgreSQL database.

---

## **Architecture**

- **Source:** `Project/AccountApi/` — CDK app + Lambda source.
- **Lambda:** `express` app (`src/app.js`) served via `serverless-http` (`src/handler.js`).
- **Storage:** single DynamoDB table (`TABLE_NAME`, default `onset-accounts-dev`) with a single-table design:
  - `PK = "ACCOUNT#<platform>:<platformId>"`
  - `SK = "CHARACTER#<slotIndex>"` for each character row
- **Deploy:** `npx cdk deploy` (region `us-east-1`); secrets come from `.env` (not committed) — see `.env.example`.
- **Config:** table name + secrets read from `src/config.js` (env vars `TABLE_NAME`, `API_KEY`, `JWT_SECRET`, `AUTH_TOKEN_SECRET`).
- **Endpoint protection:** AWS WAFv2 WebACL (rate limit ~100 req/min per IP) + CloudWatch JSON logging + X-Ray tracing.

---

## **Endpoints**

| Method | Path | Purpose | Auth |
|--------|------|---------|------|
| GET | `/health` | Liveness check | none |
| POST | `/auth/validate-token` | Verify a JWT, returns `{platform, platformId}` | none |
| GET | `/account/:platform/:id` | Account + character slots (client pads to 3) | `X-API-Key` + `X-Store-Token` |
| POST | `/account/:platform/:id` | Create account (returns empty `slots: []`) | `X-API-Key` + `X-Store-Token` |
| GET | `/account/:platform/:id/character/:slot` | Load one character | `X-API-Key` + `X-Store-Token` |
| POST | `/account/:platform/:id/character/:slot` | Create character (409 if slot occupied) | `X-API-Key` + `X-Store-Token` |
| PUT | `/account/:platform/:id/character/:slot` | Save/update character | `X-API-Key` + `X-Store-Token` |
| DELETE | `/account/:platform/:id/character/:slot` | Delete character | `X-API-Key` + `X-Store-Token` |

---

## **Authorization (two layers)**

Every `/account/*` route (`src/middleware/auth.js`) enforces both:

1. **`X-API-Key`** — static shared key (`APIKey` in config). Coarse gate; identifies that the caller is a game server.
2. **`X-Store-Token`** — short-lived HMAC-SHA256 token minted per request by the game server (`FHttpStore::BuildSignedToken`). Payload: `base64("PlatformID|Platform|SlotIndex|ExpiryUnix").base64(HMAC)` signed with `AUTH_TOKEN_SECRET`. The middleware verifies the signature + expiry, then **binds** the request: `token.platform`/`token.platformId` must match the URL path params (403 otherwise). Slot is **not** bound (platform-only authorization).

`AUTH_TOKEN_SECRET` **must equal** the game server's `[Onset.Auth] AuthTokenSecret` (or `-OnsetAuthTokenSecret=` override) or signatures will not validate. This secret is server-only — it never ships to game clients.

The static key alone can no longer read arbitrary accounts; a token valid for account A is rejected for account B.

---

## **Consistency**

Character listing (`GET /account/:platform/:id`) reads DynamoDB with **`ConsistentRead: true`** so the character-select screen never shows stale slot state immediately after a create or delete.

---

## **Interactions With Other Systems**

- **[Persistence Data Store](../Server/Persistence_Data_Store.md)** — `FHttpStore` consumes this API when `Type=HttpApi`
- **[Account System](../Player/Account_System.md)** — drives login/create/select/delete flow on the servers
