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
- **Deploy:** `npx cdk deploy` (region `us-east-1`); secrets come from `.env` (not committed).
- **Config:** table name + secrets read from `src/config.js`.

---

## **Endpoints**

| Method | Path | Purpose | Auth |
|--------|------|---------|------|
| GET | `/health` | Liveness check | none |
| POST | `/auth/validate-token` | Verify a JWT, returns `{platform, platformId}` | none |
| GET | `/account/:platform/:id` | Account + all 3 character slots | `X-API-Key` |
| POST | `/account/:platform/:id` | Create account (3 empty slots) | `X-API-Key` |
| GET | `/account/:platform/:id/character/:slot` | Load one character | `X-API-Key` |
| POST | `/account/:platform/:id/character/:slot` | Create character (409 if slot occupied) | `X-API-Key` |
| PUT | `/account/:platform/:id/character/:slot` | Save/update character | `X-API-Key` |
| DELETE | `/account/:platform/:id/character/:slot` | Delete character | `X-API-Key` |

All `/account/*` routes are guarded by the `X-API-Key` middleware (`src/middleware/auth.js`) matching `APIKey` in config.

---

## **Consistency**

Character listing (`GET /account/:platform/:id`) reads DynamoDB with **`ConsistentRead: true`** so the character-select screen never shows stale slot state immediately after a create or delete.

---

## **Interactions With Other Systems**

- **[Persistence Data Store](../Server/Persistence_Data_Store.md)** — `FHttpStore` consumes this API when `Type=HttpApi`
- **[Account System](../Player/Account_System.md)** — drives login/create/select/delete flow on the servers
