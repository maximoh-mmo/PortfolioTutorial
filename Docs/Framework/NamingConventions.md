# Naming conventions

Use MH-prefixed Unreal types: FMHError, TMHResult<T>, FMHRequestHandle,
FMHUserIdentity and IMHIdentityProvider are planned names, not implemented APIs.
Use MODULE_API export macros for non-inline symbols crossing module boundaries.
Public headers belong in Public; implementations and provider SDK includes in Private.

Planned log categories: LogMHCore, LogMHBackend, LogMHBackendHttp,
LogMHPersistence, LogMHOnline and LogMHSteam. Never log tickets, secrets or tokens.
Planned config sections: [MH.Backend], [MH.Auth], [MH.Persistence], [MH.Online].
Keep existing Onset keys working until each migration documents precedence and rollback.
Do not add empty config helpers or settings until a concrete consumer needs them.
