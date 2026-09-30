# Async conventions (implementation contract)

Every new operation that can leave the process completes asynchronously. Do not move
FHttpStore's blocking API unchanged into MH. Retain it in Onset during migration.

Before service extraction, implement and test these rules in MHCore:

- A result contains either a value or a structured error, never an ambiguous bool.
  Errors carry a stable code and a safe diagnostic; callers do not parse message text.
- Operations return a request handle with explicit cancellation and shared lifetime.
- Completion is delivered once on the game thread, never inline from the start call,
  including validation failure. Race cancellation against completion through one state gate.
- Cancellation completes with a cancellation error if it wins that gate; late provider
  responses are ignored. Cancelling a completed request does nothing.
- Owners explicitly cancel outstanding work during logout, travel and teardown.
  Callbacks bind weak UObject references; never capture an unguarded actor pointer.
- Shutdown prevents new requests and drains or cancels existing work while dispatch is
  available. Document process-exit limits; do not promise callbacks after dispatcher shutdown.
- Document timeouts and retry policy per service. Retry writes only with an explicit
  idempotency strategy. Never treat cancellation as proof that a remote write was undone.

Tests must cover success, error, invalid input, cancellation before/after completion,
late responses, duplicate responses and owner teardown before production consumers migrate.
