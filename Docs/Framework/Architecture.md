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
| MHBackend | MHPersistencePostgres | Worker-based durable PostgreSQL records with optional libpq SDK |
| MHBackend | MHPersistenceHttp | Generic v1 binary record API client |
| MHOnline | MHOnlineCore | Provider-neutral identity contracts |
| MHOnline | MHOnlineSteam | Steam implementation of identity contracts |

MHCore implements results, errors and cancellable requests. MHBackendCore implements
session-token signing, verification and configuration. MHBackendHttp provides async
transport. MHPersistence provides the record contract and a memory reference store;
MHPersistenceSQLite and MHPersistencePostgres provide durable records. The two online modules remain skeletons.
Dependencies are private until required by a public API. Unreal HTTP and SQLite types
remain private to their providers, as does libpq. MHPersistenceHttp uses MHBackendHttp
privately for the generic record protocol; Onset's account API stays game-specific.

Onset retains account/character structures, slots, classes, inventory and quest JSON,
progression, actor ownership and gameplay policy. No MH module owns those schemas.
