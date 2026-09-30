# Incremental Onset migration

| Existing source | Planned destination | Boundary |
| --- | --- | --- |
| Onset/Public/Data/IPlatformAuth.h | MHOnlineCore identity contract | Remove APlayerController from generic API |
| Onset/Private/Data/SteamAuth.cpp | MHOnlineSteam | Map Steam identifiers to provider + opaque user ID |
| Onset/Private/Subsystem/OnsetAuthSubsystem.cpp | MHBackendCore session services | Keep world/player orchestration in Onset |
| OnsetDataStore/Private/FHttpStore.cpp | MHBackendHttp + HTTP persistence provider | Introduce async transport alongside old path |
| OnsetDataStore/Public/IPlayerDataStore.h | MHPersistence + Onset adapter | Keep account/character schemas in Onset |
| OnsetDataStore/Private/FSQLiteStore.cpp, FPgSQLStore.cpp | Future provider modules | Generic records; Onset owns schema mapping |

Paths above are relative to Project/Source. Existing auth extraction planning remains
historical context; this roadmap governs the new reusable framework.

For each extraction: add the API, test a provider, add an Onset adapter, migrate one
consumer, run regression checks, then remove obsolete code. Keep the legacy route until
replacement behavior passes. Preserve DirectAuth and TokenAuth, token format/expiry,
account identity, character slot behavior and stored data compatibility.

Never silently reuse the old synchronous persistence interface as the new public API.
Do not change token wire formats or persistence schemas as incidental refactoring.
