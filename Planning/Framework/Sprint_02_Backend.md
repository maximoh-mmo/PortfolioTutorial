# Sprint 02: backend services

Session-token signing/verification and explicit [MH.Auth] configuration are implemented
in MHBackendCore. See ../../Docs/Framework/SessionTokens.md. OpenSSL stays private;
MHCore is public because the API returns TMHResult. No Onset code is referenced.

The compatibility fixture preserves the old four-field wire format. Onset's slot value
maps to the generic opaque context claim. Existing auth configuration and runtime flow
remain unchanged. Dedicated-server integration is pending a server-capable UE 5.8 build.

Next implementation: MHBackendHttp transport with typed responses, timeout/error mapping,
request cancellation, shutdown cleanup and fake/local-server tests. Do not expose
Unreal HTTP interfaces through the generic backend public API. Do not migrate blocking
FHttpStore unchanged. Then implement generic persistence and its provider contract tests.

Validation on UE 5.8.2: OnsetEditor and Onset Win64 Development builds pass. Four
headless automation tests pass with zero test warnings/errors: MH.Core.Result,
MH.Core.RequestLifecycle, MH.Backend.SessionTokens and MH.Backend.SessionConfig.
OnsetServer cannot build with the installed distribution; this remains an explicit
integration gate, not a code-validation success. No production Onset auth migration
has been performed.

Update 2026-10-01: async HTTP and the persistence foundation are now implemented.
See Sprint_03_Persistence.md for current validation and remaining provider work.
