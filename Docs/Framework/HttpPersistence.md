# HTTP record persistence (v1)

`FMHHttpRecordStore` in MHPersistenceHttp implements IMHPersistenceStore using the
private MHBackendHttp transport. The service contract is independent of Onset's
account/character endpoints. No production consumer is migrated by this module.

## Wire protocol

For a configured service root, the resource path is:

`/v1/records/{base64url(UTF8(namespace))}/{base64url(UTF8(id))}`

Encode each identifier separately using unpadded, canonical Base64 URL encoding.
This preserves case, Unicode, slashes and dot-like identifiers without introducing
path separators or traversal segments. Keys follow the generic persistence bounds.

| Operation | Method/body | Successful response |
| --- | --- | --- |
| Load | GET, empty body | 200 with raw bytes; 404 means NotFound |
| Put | PUT, application/octet-stream bytes | 204, atomic upsert |
| Delete | DELETE, empty body | 204; 404 also counts as successful deletion |

Empty bodies are valid record values. Put replaces the entire record. Successful
writes must be visible to subsequent reads at the service. Unexpected success codes
(e.g. 202 Accepted) fail with Internal rather than claiming a completed write. Other
errors use the HTTP status mapping (including Unauthorized, Conflict, Timeout and
Unavailable); response bodies and credentials are not included in generic diagnostics.
There are no retries, versions, transactions across records or conflict resolution.

## Client configuration and lifetime

Set FMHHttpRecordConfig.BaseUrl, Headers and TimeoutSeconds explicitly. Production
service roots require HTTPS. HTTP is allowed only for localhost or 127.0.0.1 fixtures.
Roots cannot contain credentials, queries, fragments or whitespace. Trailing slashes
are normalized. Headers can carry authorization; none is supplied by default. The
provider supplies Accept and Content-Type as application/octet-stream. Callers should
not override those headers with differently cased duplicate names.

Call store operations and Shutdown on the game thread. Jobs execute serially per
store, including after reopening a service connection; other store instances may race.
Completions are deferred through MHCore. Same-frame cancellation can skip queued I/O.
In-flight cancellation cancels the client request and suppresses its result, but the
server may already have committed a mutation. It does not guarantee rollback.
Shutdown cancels pending operations and rejects new work. HasPendingIO reports queued
or active work while the facade is retained. Keep the engine ticker running until work
and callbacks drain before module unload, following the common request-lifetime rules.

## Local fixture and tests

Run the plugin-contained, Python-standard-library fixture:

```powershell
python Project/Plugins/MHBackend/Tests/HttpPersistence/server.py --port-file record-port.txt
```

It binds an ephemeral loopback port, writes the port number to the supplied file and
expects `Authorization: Bearer mh-local-test-only`. Data is in memory and disappears
when the service stops. This is a test fixture, not a production backend: it uses one
shared test identity, no TLS, no persistence and a 1 MiB request-body limit. It offers
`/unexpected`, `/unavailable` and `/slow` root prefixes solely for fault injection.

Run Unreal automation `MH.Persistence.HttpRecords` with
`-MHRecordTestBaseUrl=http://127.0.0.1:<port>`. Without that argument the integration
tests report that they were not run; such a run is not integration evidence.
The contract test covers binary/empty payloads, Unicode/SQL-shaped keys, case and
namespace isolation, upserts, idempotent delete, missing records, pre-I/O cancellation,
shutdown/destruction, configuration/auth errors and records surviving a new client.
The failure test covers unexpected success codes, 503, a real HTTP timeout,
in-flight cancellation and rejected nonlocal plaintext configuration.

Verified 2026-10-02 with source UE 5.8.3: OnsetEditor and OnsetServer Win64 Development
builds pass. Both HTTP record integration tests pass without warnings against the live
fixture. A direct Python HTTP smoke check also passes. The full 13-test run has 11
successes (one with an unrelated EOS connectivity warning) and two PostgreSQL failures:
the local database is not listening on port 55432. PostgreSQL host smoke also failed
to connect, so that external fixture must be restarted before claiming a clean full run.
The newly linked server has not been recooked/repackaged for this provider revision.
