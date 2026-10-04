# LAN baseline preparation

Prepare a portable Windows client and two local servers before changing Onset auth.

- Client-only target builds with source UE 5.8.3; excludes native PostgreSQL runtime.
- Client-only worlds skip the legacy datastore subsystem, including standalone menus.
- Opt-in MHLanTest config strips credential keys during staging.
- Packager, packaged-config verifier, Steam smoke/client launcher and server-pair
  launcher are in Scripts/Framework. Second-machine instructions are in
  Docs/Framework/LanTesting.md.

## Validation (2026-10-03)

- OnsetClient, OnsetEditor and OnsetServer Development builds passed.
- Client cook passed: zero errors, one Windows certificate-store warning.
- Client staging/archive passed after allowing access to the local Zen service.
- Inspected 102 packaged INI files: credential keys absent; libpq.dll absent.
- Packaged MH.Online.SteamLive passed with App ID 480. Launcher rerun also passed
  after explicitly waiting for the GUI process and checking its exit code.
- The known OSS Unexpected TicketForWebResponse warning remains in the JSON report.
- Startup also logs existing unset StateTree asset errors; the test itself passes.
  The packaged build has no HTML report template; index.json is the test evidence.
- PowerShell syntax checks passed for the launch/packaging scripts.
- Matching server cook/stage/archive passed. Both launch roles initialized SQLite;
  DirectAuth listened on 17877 and TokenAuth on 17878 at 192.168.0.84.
- A packaged local client was welcomed by LoginServer and the server logged a
  successful join. The bounded headless probe was then stopped. This is connectivity
  evidence only; character selection and token handoff were not exercised.
- Server logs retain unset StateTree errors and a missing loading-screen class warning.

## Steam packet-auth migration (2026-10-04)

- Enabled `OnlineSubsystemSteam.SteamAuthComponentModuleInterface` in the project
  packet-handler configuration. It calls `SteamGameServer::BeginAuthSession` during
  connection establishment, before player-controller RPCs are available.
- Legacy gameplay ticket submission and validation are compatibility no-ops. They no
  longer accept, store, or log ticket material; the standalone legacy validator fails
  closed if called.
- OnsetClient and OnsetServer Development builds passed with the change.
- The new packaged client passed its Steam ticket smoke test and packaged-config
  inspection. The matching server package started with `Steam Auth Enabled` and the
  Steam dedicated-server API initialized.
- A distinct LAN Steam client passed packet-handler acceptance on LoginServer and
  DemoLevel, then completed DirectAuth character selection and TokenAuth handoff.

Ticket rejection and logout/revocation remain required checks before production rollout.
