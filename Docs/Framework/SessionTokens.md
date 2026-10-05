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
The context claim is opaque. The Onset adapter maps its selected-character slot to this
claim and validates its game-specific range. The service checks nonempty bounded claims,
forbids delimiters/control characters, verifies signatures with constant-time comparison,
rejects malformed payloads, and returns claims only on success. Expiry uses the existing
Onset boundary: valid when `now == expiry`, expired when `now > expiry`.

The caller supplies UTC Unix seconds for deterministic tests and explicit clock ownership.
Issue rejects overflow. Tokens are signed, not encrypted; do not put secrets in claims.
Replay prevention, audience binding, key rotation and revocation require a separate
versioned protocol decision; they are not introduced silently during compatibility work.

Signing/verification are synchronous in-process operations. Calls that leave the process
must use the MHCore async contract. Neither this service nor its tests contacts a server.

## Onset adapter

Onset now issues and validates its login-to-game-server tokens through
`FMHSessionTokenService`. The adapter preserves the deployed envelope exactly:
`Base64(UTF8(PlatformID|Platform|SlotIndex|Expiry)).Base64(HMAC-SHA256)`. It accepts only
the existing unselected-slot sentinel `-1` or slots `0` through `2`, so a signed but invalid
context claim cannot authorize TokenAuth. This keeps the generic service free of game schema.

The adapter is compiled for server and editor targets. Client builds retain opaque token
transport only and do not take a direct dependency on MHBackendCore.

`MH.Backend.SessionTokens` checks an independent .NET HMAC fixture matching the existing
Onset wire format, payload/signature tampering, expiry boundaries, malformed input,
configuration failure, Unicode and long signing keys. This does not replace a live
login-server/game-server integration test.

`MH.Backend.SessionConfig` loads an isolated INI fixture, checks secret/lifetime values,
and rejects missing secrets and nonpositive lifetimes. Paths are normalized before
accessing Unreal's config cache. The fixture is removed after the test.

`Onset.Auth.SessionTokens` adds the host-side compatibility gate. It matches an independent
legacy HMAC fixture, retains the expiry boundary and slot mapping, rejects malformed or
out-of-range slot contexts, and fails closed for tampering or missing signing configuration.
