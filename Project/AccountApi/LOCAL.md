# Local development workflow

Run the account server against a local DynamoDB so testing never touches AWS (zero cost, zero free-tier meter burn) while still seeing per-request read/write capacity usage.

## 1. Start DynamoDB (pick one)

**Docker (recommended):**
```bash
docker run -d --name onset-ddb -p 8000:8000 amazon/dynamodb-local:latest
```

**Or the downloadable JAR (needs JRE 17+):**
```bash
# one-time: download + extract DynamoDB Local into .local/dynamodb-local
# note: -dbPath must point at a directory (this build rejects relative paths)
java "-Djava.library.path=<abs>\DynamoDBLocal_lib" -jar <abs>\DynamoDBLocal.jar \
  -dbPath <abs-db-dir> -sharedDb -disableTelemetry -port 8000
```

Verify: `aws dynamodb list-tables --endpoint-url http://localhost:8000`

## 2. Create the table (once per fresh DB)

```bash
npm run db:local:create
```
Creates `onset-accounts-dev` with PK/SK, provisioned 25/25 to mirror prod so local RCU/WCU numbers map 1:1 to the free-tier meter units.

## 3. Start the account API

```bash
npm start
```
Serves on http://localhost:3000. With `ENABLE_METRICS=1` in `.env` it logs a `[metrics]` line per request (`rcu=` / `wcu=` in capacity units) and exposes session totals at http://localhost:3000/local/metrics.

## 4. Point the game server at it

Launch the dedicated server with command-line flags (prod ini untouched):
```
-OnsetDataStoreType=HttpApi -OnsetDataStoreURL=http://localhost:3000
-OnsetDataStoreAPIKey=dev-api-key-change-me-in-production
-OnsetAuthTokenSecret=local-dev-shared-secret
```
The `AUTH_TOKEN_SECRET` value must match the one in `AccountApi/.env`.

## 5. Monitor usage/throughput

- Per-request: console log lines like `[metrics] POST /account/steam/765... 201 4.2ms rcu=0 wcu=1`
- Session totals: `curl http://localhost:3000/local/metrics` → requests, rcu, wcu, req/s, peak req/s, per-path breakdown

## Env vars that matter locally

| Var | Purpose |
| --- | --- |
| `DYNAMODB_ENDPOINT` | when set, the API targets the local DB and uses dummy creds (never real AWS) |
| `ENABLE_METRICS` | `1` turns on metrics logging + `/local/metrics` (never enabled in prod Lambda) |
| `AUTH_TOKEN_SECRET` | must match the server's `-OnsetAuthTokenSecret` or every request is 403 |

Unset `DYNAMODB_ENDPOINT`/`ENABLE_METRICS` to restore prod behavior (real AWS DynamoDB).