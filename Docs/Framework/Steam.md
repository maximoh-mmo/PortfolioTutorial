# Steam identity provider

`FMHSteamIdentityProvider` implements `IMHIdentityProvider` using private Steamworks SDK
types. Native client support currently targets Win64; dedicated servers and other targets
return NotConfigured for acquisition. Core contracts and mock tests remain SDK-independent.
The host must enable/configure OnlineSubsystemSteam, initialize it, and keep its callback
pump running. MH retains a SteamShared client instance handle; it does not run a second
callback pump or independently shut down Steam. Construct/use/destroy providers on the
game thread, and shut them down before unloading the online subsystem.

Local user zero maps to provider `Steam` and the decimal SteamID64 subject. Other local
indices are rejected. Audience strings select the ticket kind:

- `webapi:<identity>` requests a Web API ticket. Identity is 1–128 ASCII letters, digits,
  underscores, hyphens or periods. The verifier must use the same identity.
- `session:<SteamID64>` requests a game-session ticket targeted at the specified Steam
  identity. The ID must be a nonzero canonical decimal uint64. The live acquisition test
  targets the local account; it does not validate a server session.

Requests require a positive finite timeout. Callbacks are marshalled through a queue to
the game thread. Cancellation, timeout, account change, failure and shutdown revoke owned
native handles. Successful requests transfer revocation into the shared ticket lease.
Logout/account change releases live leases on the next core tick. Unknown callbacks are
ignored: other Steam consumers own their own handles. No credential bytes are logged.

Steam tickets are raw bytes. Server authentication, ticket encoding for transport, Steam
ownership/bans policy, and backend session issuance remain host responsibilities. A local
identity lookup or successfully issued ticket does not authenticate a remote connection.
See Valve's [ISteamUser documentation](https://partner.steamgames.com/doc/api/ISteamUser)
for the distinct native-session and Web API verification protocols.

## Tests

Build the host editor with these plugin sources, then run the plugin's test script:

```powershell
& .\Project\Plugins\MHOnline\Tests\Run-SteamTests.ps1 `
  -EngineRoot 'Z:\UnrealEngine' -ProjectFile '.\Project\Onset.uproject'
```

Add `-Live` for the opt-in native acquisition test. Steam must be running and signed in,
and the host must configure its Steam App ID. This project currently uses test App ID 480.
The script runs an isolated base game mode, stores a unique report under Project/Saved,
and checks the JSON result because editor exit status alone does not prove tests passed.
Logs may include identifiers emitted by the host/online subsystem; review before sharing.

`MH.Online.SteamLifecycle` covers success, explicit cancellation followed by a late callback,
native failure, timeout, logout, shutdown, destruction, malformed audience, missing native
configuration and signed-out identity. It asserts exactly one revocation for every started
native request. `MH.Online.SteamLive` checks local identity and acquires/releases both ticket
kinds. Neither test claims successful remote authentication or server-side revocation.

UE 5.8.3 OnlineSubsystemSteam also receives MH's Web API callback and logs
`AUTH: Unexpected TicketForWebResponse` because its own request map does not own the handle.
MH receives and handles that callback independently. The warning is retained in reports;
it is not suppressed or treated as server validation.

## Migration gate

Onset now enables `OnlineSubsystemSteam.SteamAuthComponentModuleInterface` through
`PacketHandlerComponents`. On a dedicated server, that handler sends the connection ticket
to `SteamGameServer::BeginAuthSession` and processes Steam's asynchronous response. The
former gameplay RPC ticket route is retained only as a compatibility no-op, and
`FSteamAuth::ValidateTicket` fails closed if called.

The 2026-10-04 LAN test recorded callback-validated packet-handler authentication for a
Steam client on the DirectAuth login server, followed by TokenAuth game-server handoff.
The server bootstrap requested anonymous Game Server logon because Onset does not create an
advertised OnlineSubsystem Steam session; both servers then reported backend connection and
secure policy before accepting the client. A malformed-ticket test was refused during the
packet handshake without `PostLogin`, account work or token issuance. Closing the accepted
client ended its server-side Steam authentication session. These are development regressions;
repeat them before a production deployment with the production App ID and server settings.

The 2026-10-05 Connect-button regression exposed two callback lifecycle issues in the
host's world subsystem: an outgoing world could unbind the incoming world's Steam result
delegate, and validation arriving before `PostLogin` was discarded. Teardown now unbinds
only the delegate owned by that subsystem. Early results are retained per Steam user,
consumed once at `PostLogin`, and cleared at logout/world teardown. Account work still
requires a successful asynchronous Steam validation result.

`Onset.Auth.SteamEarlyResult` passed the early-success, rejection, latest-result,
user-isolation and cleanup checks. Editor/server builds, cooking and dedicated-server
startup checks also passed. A real Steam DirectAuth connection then produced the server's
validation-success message and delivered `Client_AccountData` to the client, which created
the main menu and exited cleanly. This headless check verifies the prerequisite for the
Connect gate; the rendered button still needs confirmation in the packaged client.
