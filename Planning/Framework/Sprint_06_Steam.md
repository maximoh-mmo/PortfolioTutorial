# Steam adapter — 2026-10-03

Implemented FMHSteamIdentityProvider with private Steamworks SDK dependencies and a
testable native client boundary. It uses SteamShared for native lifetime and OSS for the
callback pump. Steam callbacks enqueue results; public completions run on the game thread.
Ticket ownership transfers to MHOnlineCore leases. Cancellation, timeout, failure,
account change, shutdown and destruction revoke owned handles. Public headers expose no
Steam SDK types. Unsupported native targets and dedicated servers fail NotConfigured.

Validation on source Unreal Engine 5.8.3:

- OnsetEditor Win64 Development: passed.
- OnsetServer Win64 Development: passed.
- MH.Online.SteamLifecycle: passed without warnings, 11 request outcomes and seven
  exactly-once native revocations.
- MH.Online.SteamLive: passed for both Web API and targeted session tickets with App ID
  480 and the user's running, signed-in Steam client. Both leases released successfully.
- Live report retains OSS's `Unexpected TicketForWebResponse` warning for the handle
  owned by MH. Neither ticket bytes nor account IDs are emitted by MH tests.
- Plugin test launcher PowerShell syntax: passed. The equivalent underlying editor
  commands were executed; the wrapper itself has not been run end-to-end.

Final server link encountered LNK1158 launching mt.exe through the build executor.
The exact generated link response succeeded when invoked directly from Engine/Source
with the Windows SDK 10.0.22621.0 x64 and MSVC 14.50.35717 Hostx64/x64 directories
prepended to the process PATH. After renewing engine log write access, UnrealBuildTool
completed target metadata with Result: Succeeded. No engine files or global PATH were
changed. A future clean link may need the same tool-environment investigation.

The initial live run triggered an unrelated Onset controller auth timeout. The successful
rerun used `/Engine/Maps/Entry?game=/Script/Engine.GameModeBase` to isolate the adapter;
the supplied script uses that same map override. No production authentication behavior
was changed and no remote verifier acceptance/revocation is claimed.

Next gate: Onset's existing ticket handlers only reject empty strings. Establish actual
server verification and exercise accept/reject, logout and handoff with distinct Steam
clients before migrating consumers. The user confirmed availability of a second client;
its network/platform/project setup is pending. See Docs/Framework/Steam.md.
