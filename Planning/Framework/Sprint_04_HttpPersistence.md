# HTTP persistence foundation, 2026-10-02

Implemented MHPersistenceHttp using MHBackendHttp privately, with no Onset dependency.
The v1 service contract uses canonical base64url UTF-8 identifiers and binary payloads.
Per-store queues preserve normal operation order, skip cancelled queued jobs, cancel
active transport requests and reject new work after shutdown. Status handling is strict
about completed writes (204); unexpected 2xx responses do not count as success.

A plugin-contained loopback Python fixture implements atomic in-memory upserts and
deterministic fault routes. Unreal tests cover shared persistence semantics, auth errors,
strict statuses, 503, real timeout and cancellation. Both new tests pass without warnings.
Editor and dedicated-server builds pass against source UE 5.8.3. Provider translation
units compiled separately through adaptive non-unity compilation.

The full suite reported 10 clean successes, one success with an unrelated EOS connection
warning and two PostgreSQL failures. TCP and libpq checks confirm that the test database
is not accessible on 127.0.0.1:55432. Docker engine access remains denied in this session;
restart the fixture externally and rerun its tests before a full-suite green claim.

No production Onset services were redirected. The HTTP fixture is not a durable or
production-authorized backend. Subsequent work is the identity contract/Steam provider
and the already documented client, authentication and persistence migration gates.
