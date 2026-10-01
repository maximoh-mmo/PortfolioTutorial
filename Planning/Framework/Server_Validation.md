# Dedicated-server validation

OnsetServer Win64 Development built successfully on 2026-10-01 using the UE 5.8.3
source engine at `Z:\UnrealEngine`. The user ran the prepared build script outside
the coding sandbox; the saved build log and resulting executable were inspected.

- Result: Succeeded; reported process exit code 0.
- 1,026 build actions; 2,258.59 seconds total (about 37 minutes 39 seconds).
- Output: `Project/Binaries/Win64/OnsetServer.exe` (316,941,312 bytes).
- MH_POSTGRES_ROOT was supplied by the build script for the existing libpq SDK.
- The build emitted a first-header diagnostic for MHHttpClient.cpp (expects
  MHHttpClient.h before MHHttpClientState.h), but still compiled/linked successfully.
  The source now includes MHHttpClient.h first; the next build must verify the fix.
- The engine dependency scan needed `-maxCpuCount:1` in DotnetDepends.bat.
  This fixes the observed solution scan failure without limiting C++ build parallelism.
  The user applied that engine-local change; it is not a project source modification.

The dedicated-server compile/link gate now passes. Previous notes about the Launcher
engine blocking server compilation describe historical attempts and are superseded
by this result. Editor/game tests previously passed on 5.8.2; they have not yet been
repeated on 5.8.3. The source engine has not been set as the project's default association.

Remaining gates: cook/package server content, start the server in an isolated test
configuration, verify runtime libraries/module initialization, and run client login,
auth and persistence regressions. A successful compile does not establish these.
Do not redirect production Onset consumers until those runtime checks pass.

The next run is prepared in `Scripts/Framework/Validate-Server.ps1`; see
`Docs/Framework/ServerTesting.md`. It builds the matching editor, cooks/stages the
server and runs an isolated local startup check. Its syntax has been checked, but
execution still requires engine write access outside the current coding sandbox.
