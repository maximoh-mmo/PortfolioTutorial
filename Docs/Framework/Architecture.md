# Architecture

MH provides mechanisms; Onset owns game meaning. Initial support is UE 5.8 only.
Use a few plugins with focused modules, async-first APIs, Steam first, and incremental
replacement. Lyra and GameFeature integration belongs in optional adapters above MH.

| Plugin | Initial modules | Responsibility when implemented |
| --- | --- | --- |
| MHCore | MHCore | Results, errors, request lifetime, common conventions |
| MHBackend | MHBackendCore | Backend configuration, sessions, auth handoff |
| MHBackend | MHBackendHttp | Async HTTP transport |
| MHBackend | MHPersistence | Generic record persistence contracts |
| MHBackend | MHPersistenceSQLite | Worker-based durable SQLite records |
| MHOnline | MHOnlineCore | Provider-neutral identity contracts |
| MHOnline | MHOnlineSteam | Steam implementation of identity contracts |

MHCore implements results, errors and cancellable requests. MHBackendCore implements
session-token signing, verification and configuration. MHBackendHttp provides async
transport. MHPersistence provides the record contract and a memory reference store;
MHPersistenceSQLite provides durable records. The two online modules remain skeletons.
Dependencies are private until required by a public API. Unreal HTTP and SQLite types
remain private to their providers. PostgreSQL and HTTP persistence providers are pending.

Onset retains account/character structures, slots, classes, inventory and quest JSON,
progression, actor ownership and gameplay policy. No MH module owns those schemas.
