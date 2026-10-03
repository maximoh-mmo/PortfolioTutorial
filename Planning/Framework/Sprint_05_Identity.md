# Identity contract validation — 2026-10-03

Implemented MHOnlineCore provider-neutral account identifiers, asynchronous identity and
ticket request interfaces, and a shared ticket lease with explicit and automatic native
release. Public dependencies expose Core and MHCore correctly to consumers. Added mock
automation and documented provider obligations and the boundary with server authentication.

Validation with source Unreal Engine 5.8.3:

- OnsetEditor Win64 Development: succeeded.
- OnsetServer Win64 Development: succeeded.
- MH.Online.IdentityContract: succeeded, with an unrelated EOS_NoConnection warning.
- PostgreSQL loopback port 55432: connection timed out; no database regression rerun.

Follow-up after the user restarted the fixture on 2026-10-03: host TCP/password and
SQL smoke passed against PostgreSQL 17.11. Both `MH.Persistence.PostgresContract` and
`MH.Persistence.PostgresTimeout` passed without warnings in source UE 5.8.3. The earlier
database availability blocker is cleared; this does not imply production migration.

The mock verifies deferred completion, exactly-once cancellation, disposal of late tickets,
idempotent shared release, automatic final-owner release, shutdown revocation and rejection
of new requests after shutdown. It is not a Steam integration test. Native Steam acquisition,
timeouts, logout handling and a real client smoke test are the next milestone. No Onset
production consumer was migrated. The public Unreal Steam token API drops the native ticket
handle, so the adapter must use an integration that retains native revocation ownership.
