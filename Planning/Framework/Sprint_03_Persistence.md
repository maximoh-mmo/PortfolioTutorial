# Sprint 03: transport and persistence foundation

Delivered on 2026-10-01:

- MHCore cancellation cleanup hook, deferred to the game thread before cancelled completion.
- MHBackendHttp: typed requests/responses, status mapping, timeout, transport cancellation,
  input validation, idempotent shutdown and private Unreal HTTP implementation.
- MHPersistence: case-sensitive namespace/key record contract and a memory reference store.
- MHPersistenceSQLite: serial per-store worker I/O, parameter-bound atomic upserts/deletes,
  explicit connection cleanup, shutdown handling and persistence across store instances.

Validation: OnsetEditor and Onset Win64 Development builds pass with UE 5.8.2. Nine MH
headless automation tests pass with zero unexpected warnings/errors. HTTP loopback was
actually run against the provided Python fixture, including POST body/header forwarding,
404, timeout and disconnection. SQLite tests use scratch databases and verify reopening,
empty/binary payloads, cancellation, failure handling and case-sensitive key isolation.

The HTTP tests register only deliberate fixture warnings as expected. Do not interpret a
loopback test run without MHHttpTestBaseUrl as a successful integration check.

Remaining provider work:

- PostgreSQL: user started the supplied Docker fixture. Host libpq authentication and
  rollback-only SQL smoke tests pass against PostgreSQL 17.11. Provider implementation
  and Unreal integration tests remain pending. Docker engine pipe access is still denied,
  but database access is no longer blocked. No existing user database was used or modified.
- HTTP persistence: define a generic record protocol and matching server fixture on top
  of the tested transport; existing Onset endpoints remain game-specific.
- Online identity/Steam are still pending. Production Onset migration remains gated on
  a server-capable UE 5.8 build; the installed distribution cannot build OnsetServer.

No existing Onset persistence/auth consumer has been redirected to these providers.
Use Docs/Framework/Http.md and Persistence.md for API and lifetime contracts.

Source checkout inspection (2026-10-01): both server config layers select HttpApi,
pointing to an AWS Lambda endpoint. AccountApi uses DynamoDB; Test_All.ps1 selects
the HTTP path. PostgreSQL code and client libraries exist, but no active PostgreSQL
connection configuration, Windows service, or local port 5432 listener was found.
Configuration values and credentials were not copied into the framework.
