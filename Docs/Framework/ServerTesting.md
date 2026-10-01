# Cooked dedicated-server startup check

Run `Scripts/Framework/Validate-Server.ps1 -EngineRoot Z:\UnrealEngine` from PowerShell
with write access to the source engine. The script resolves the project from its own
location, so it works from any current directory. `PostgresRoot` optionally supplies a
different libpq SDK; the default is this checkout's existing third-party SDK.

The script builds OnsetEditor with the chosen source engine, then uses AutomationTool
BuildCookRun to build OnsetServer, cook DemoLevel, stage and archive WindowsServer.
It writes uniquely named results under `Project/Saved/MHServerValidation`. It does not
change the project's EngineAssociation. Builds reuse normal Unreal intermediates;
avoid concurrent editor/build sessions for this project.

The startup check uses the archived server executable, a fresh SQLite database in the
run directory, Token auth with a public test-only secret, Null online subsystem and
loopback port 17877. It checks the port is free first; `-Port` selects another port.
It disables Steam for this test and explicitly overrides the datastore to SQLite.
The running server exits normally after 20 seconds of engine tick time. A 180-second
outer timeout covers slow startup; on timeout the script stops only its own process.

A pass requires a zero exit code, successful SQLite initialization, Token auth
initialization, an IP driver listener, world startup, creation of the scratch database,
no datastore fallback and no fatal/assertion/exception log markers. The resulting
`result.json` records each check. Logs and databases are retained for diagnosis.
Missing packaged libpq.dll fails before startup. These checks exercise the existing
Onset persistence path; they do not claim MH runtime CRUD, Steam authentication,
client login, player data compatibility, graceful player-save shutdown or migration
regressions. Those are separate gates.

To repeat startup without rebuilding/cooking, supply `-PackageRoot` with the existing
archived `WindowsServer` directory. This still creates a new log/database directory.

Validated here: PowerShell syntax and option/log-marker names against UE 5.8.3 and
Onset source. Execution remains pending: engine write-permission requests returned
no granted paths in the coding sandbox. A passing build from the earlier stage is
not evidence that this cook/startup script has passed.
