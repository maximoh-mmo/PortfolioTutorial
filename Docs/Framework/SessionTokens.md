# Backend session tokens

MHBackendCore provides `FMHSessionTokenService`, a local signing/verifying service.
Its public API depends on MHCore; OpenSSL is an implementation-only dependency.
No Onset, online provider, world or actor types are referenced.

Construct with `FMHSessionTokenConfig` or load `[MH.Auth] Secret` and `LifetimeSeconds`
from an explicit ini filename. There is no development-secret fallback. Supply a strong,
random shared secret through deployment configuration; never commit real secrets or send
them to clients. Lifetime must be positive and defaults to 300 seconds. This configuration
is opt-in: Onset continues using its existing `[Onset.Auth]` keys until migration.

The wire envelope remains `Base64(UTF8(subject|provider|context|expiry)).Base64(HMAC-SHA256)`.
The context claim is opaque. A future Onset adapter maps its character slot to this claim
and validates its game-specific range. The service checks nonempty bounded claims,
forbids delimiters/control characters, verifies signatures with constant-time comparison,
rejects malformed payloads, and returns claims only on success. Expiry uses the existing
Onset boundary: valid when `now == expiry`, expired when `now > expiry`.

The caller supplies UTC Unix seconds for deterministic tests and explicit clock ownership.
Issue rejects overflow. Tokens are signed, not encrypted; do not put secrets in claims.
Replay prevention, audience binding, key rotation and revocation require a separate
versioned protocol decision; they are not introduced silently during compatibility work.

Signing/verification are synchronous in-process operations. Calls that leave the process
must use the MHCore async contract. Neither this service nor its tests contacts a server.
Do not migrate Onset login/game-server handoff until dedicated-server regression can run.

`MH.Backend.SessionTokens` checks an independent .NET HMAC fixture matching the existing
Onset wire format, payload/signature tampering, expiry boundaries, malformed input,
configuration failure, Unicode and long signing keys. This does not replace a live
login-server/game-server integration test.

`MH.Backend.SessionConfig` loads an isolated INI fixture, checks secret/lifetime values,
and rejects missing secrets and nonpositive lifetimes. Paths are normalized before
accessing Unreal's config cache. The fixture is removed after the test.
