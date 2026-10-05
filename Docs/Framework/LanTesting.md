# Windows LAN test setup

The second machine needs Windows 10 or Windows 11 and Steam signed into a **different account** from
the first client. It does not need Unreal Engine, Visual Studio, Git, Docker or PostgreSQL.
This development package uses Steam test App ID 480. No Steam password or publisher key
is needed by the test launchers.

Copy the entire verified `WindowsClient` directory using a USB drive or your existing
LAN file share. Keep its Engine and Onset subdirectories together. If prerequisites are
missing, run `Engine/Extras/Redist/en-us/vc_redist.x64.exe` from that directory. Start
Steam normally before launching the game. Do not copy the repository or server configs.

From PowerShell in the copied directory, first check the local Steam installation:

```powershell
& .\Start-LanClient.ps1 -PackageRoot . -SteamSmoke
```

The launcher stores results under `MHTestResults`. A pass confirms ticket acquisition and
release for that Steam client. The known Unreal `Unexpected TicketForWebResponse` warning
can appear; no server authentication is performed by this smoke test.

Once the test server is ready, connect using its private LAN IPv4 address:

```powershell
& .\Start-LanClient.ps1 -PackageRoot . -ServerAddress 192.168.0.84 -Port 17877
```

The address above was detected on the development computer during preparation; confirm
it has not changed. Both clients can connect to this same server from separate Steam
accounts. Keep the server on the LAN; no router port forwarding is required. If Windows
Firewall blocks the server, allow its executable on the Private network profile only.
The scripts do not modify firewall rules or create a network share.

## Server on the development computer

Use the matching freshly built server archive, then run. Replace the placeholder with
the real directory path; do not include the angle brackets.

To build that archive from source, run `Scripts/Framework/Validate-Server.ps1
-EngineRoot 'Z:\UnrealEngine'`. Its output directory contains `archive/WindowsServer`
with both LoginServer and DemoLevel cooked. The script also runs the existing local
server smoke check and stops that check's process before returning.

```powershell
& .\Scripts\Framework\Start-LanServer.ps1 -PackageRoot '<WindowsServer folder>' `
  -BindAddress 192.168.0.84 -Port 17877
```

The launcher starts a DirectAuth login server on 17877 and a TokenAuth game server on
17878, with separate Steam query ports 17879 and 17880. It prints both process IDs, the
log directory and an exact stop command. Each run gets one fresh SQLite database shared
by the two servers. It overrides datastore selection, the shared development secret,
AuthMode and the game-server LAN address; existing databases are not used.

Steam's packet handler is enabled in the packaged client and server. It starts Steam's
dedicated-server ticket validation during the network handshake, before gameplay RPCs
are available. A remote two-machine acceptance test passed on 2026-10-04, including
DirectAuth login and TokenAuth character handoff. Rejection and logout/revocation cases
remain required checks before production rollout.

## Build and package on the development computer

```powershell
& .\Scripts\Framework\Package-LanClient.ps1 -EngineRoot 'Z:\UnrealEngine' `
  -OutputDirectory '<new absolute output directory>'
```

The packaging script builds OnsetEditor before cooking so the cooker's reflected C++
classes match the client. `-SkipBuild` skips only the client build; the editor build still
runs. After changing C++ source, use the full build command above.

The script also launches the cooked menu with rendering and Steam disabled to catch
asset serialization failures. Its results are written to `startup-check/result.json`
under the output directory; this check does not validate Steam authentication or the
rendered UI.

Validated 2026-10-05: the startup check reproduced the previous cooked-menu bad-export
failure, then passed after building matching editor/client modules and removing the
native property that shadowed the Blueprint's ConnectButton variable. The original
MyOnsetPlayerController Blueprint remains the runtime controller. The Connect-button
authentication gate still requires a live LAN UI check.

The OnsetClient target excludes legacy database backends. `CustomConfig=MHLanTest` enables
local test settings and a packaging denylist for credential keys. Before distributing a
build, inspect the staged/package INI files to confirm filtering, and run the packaged
Steam smoke test. A successful editor test does not validate the packaged client.

Keep logs local until reviewed: legacy Onset can log account identifiers and token-bearing
travel URLs. Share the test result status and relevant error lines with credentials removed.
