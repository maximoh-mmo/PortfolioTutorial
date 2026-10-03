# Local identity and authentication tickets

MHOnlineCore exposes `IMHIdentityProvider` without Steam, Onset or backend dependencies.
`FMHAccountId` combines a provider namespace and a case-sensitive opaque subject. It is
local account information, not evidence that a remote caller owns that account.

`GetLocalAccount` queries an already signed-in local user. `AcquireAuthTicket` requests
a temporary credential for an explicit provider-specific audience, with a finite timeout.
Neither API opens a login UI. Providers must reject invalid user indices, unsupported
audiences and non-positive/non-finite timeouts before starting native work. Use
InvalidArgument for invalid input, Unauthorized for a signed-out account, NotConfigured
for absent SDK configuration, Unavailable for shutdown/service failure, and Timeout for
an expired request. Completion is deferred to the game thread and happens exactly once.

A successful result owns a shared `FMHAuthTicket` lease. The host retains it while the
verifier needs the credential, then calls `Release`. Release is idempotent, clears the
retained bytes and invokes native revocation once. Destruction of the last reference also
releases it. All lease access and final destruction belong on the game thread. Callers
must not log, persist or unnecessarily copy its bytes; clearing the lease cannot clear
caller copies or retract credentials already sent over the network.

Providers track outstanding leases weakly and release them on shutdown. Cancellation
and timeout must clean up native requests and discard/revoke late tickets. Shutdown is
idempotent and rejects further work. Shut providers down before their native SDK, while
the core ticker can still deliver cancellations. Native release callbacks must not capture
a destroyed provider. Server validation and backend session issuance remain separate host
responsibilities; no production Onset authentication has been migrated.

`MH.Online.IdentityContract` uses a controlled mock to cover identity namespace equality,
deferred callbacks, cancellation followed by late completion, lease destruction, shared
release, shutdown revocation and rejection after shutdown. It does not validate Steam.

## Steam integration constraint

In the inspected UE 5.8.3 source, `FOnlineIdentitySteam::GetLinkedAccountAuthToken` forwards
the Web API ticket string but discards `AuthTicketHandle` from its internal callback.
That public abstraction cannot implement individual native revocation. The adapter must
retain the handle through a supported native integration; do not access private engine
headers or treat an unrevocable string as an owned lease. Steam implementation and a real
logged-in client smoke test remain pending.
