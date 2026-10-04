# MH framework roadmap

Locked decisions: MH prefix; PortfolioTutorial repository; few plugins/multiple modules;
UE 5.8; async-first; Steam first with extensible interfaces; incremental Onset migration;
broad reuse with optional Lyra/GameFeature adapters.

| Milestone | Scope | Exit gate |
| --- | --- | --- |
| 0 | Conventions and three plugin skeletons | Descriptor checks and editor/game/server compile |
| 1 | MHCore result/error types and cancellable request lifetime | Async lifecycle automation tests |
| 2 | MHBackendCore configuration and session-token services | Token compatibility, expiry and malformed-input tests |
| 3 | MHBackendHttp async transport | Timeout, cancellation, status/error mapping tests |
| 4 | MHPersistence generic async contract | In-memory contract tests without Onset types |
| 5 | SQLite/PostgreSQL/HTTP provider modules | Provider parity, failure and schema migration tests |
| 6 | MHOnlineCore identity contract | Provider-neutral identity and mock-provider tests |
| 7 | MHOnlineSteam | Ticket lifecycle and Steam integration smoke test |
| 8 | Onset auth migration | DirectAuth and TokenAuth login/logout/handoff regression |
| 9 | Onset persistence migration | Load/save/delete and existing-data compatibility |
| 10 | Cleanup and extraction hardening | Independent host, packaging, docs, legacy removal |
| 11+ | Online/gameplay/UI services and adapters | Prioritized by concrete consumers |

Current status: milestone 0 editor/game builds and plugin loading pass on 5.8.2;
OnsetServer Win64 Development compiles and links with source-built UE 5.8.3.
The 5.8.3 editor build and cooked server startup also pass. Client/auth/persistence
runtime regressions remain pending; see [server validation](Server_Validation.md).
Milestone 1 primitives and automation tests pass.
Milestone 2 session-token service and configuration are implemented alongside the legacy
Onset path. Milestones 3 (HTTP) and 4 (persistence contract) are implemented and tested.
Milestone 5 provider foundations are implemented: SQLite, PostgreSQL and HTTP records.
HTTP record tests pass on 5.8.3. After restarting the PostgreSQL fixture on 2026-10-03,
host SQL smoke, PostgresContract and PostgresTimeout tests pass on 5.8.3 without warnings.
The HTTP service fixture is
in-memory; a durable production service and migration compatibility remain host work.
Milestone 6 is implemented: provider-neutral account/ticket contracts, shared ticket
revocation and mock lifecycle automation. Editor and server builds and the identity
test pass on source UE 5.8.3 (2026-10-03); the test recorded an EOS connectivity warning.
Milestone 7 client adapter is implemented: mock lifecycle tests and live acquisition/release
of session and Web API tickets pass with Steam App ID 480 on 2026-10-03. The live run records
the OSS warning for an MH-owned Web API handle. Editor and server builds pass; the server
target intentionally cannot acquire local client tickets. See [Steam guide](../../Docs/Framework/Steam.md).
Milestone 8 has Steam packet-handler authentication enabled: it starts dedicated-server
ticket validation before gameplay RPCs. On 2026-10-04, the project-owned Steam server
bootstrap logged both dedicated servers into the Steam backend and received secure policy
responses. A LAN Steam client then completed callback-validated DirectAuth-to-TokenAuth
handoff. The malformed-ticket regression was refused during the packet handshake, without
server-side `PostLogin`, account work or token issuance. The valid-client disconnect also
ended its Steam authentication session. The client-only package, credential inspection and
packaged Steam acquisition/release smoke pass. See [LAN setup](../../Docs/Framework/LanTesting.md).
Milestone 9 is next.
See [identity contract](../../Docs/Framework/Identity.md).
Production migration must wait for server validation.
See [foundation sprint](Sprint_01_Foundation.md).
