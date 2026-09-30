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
| MHOnline | MHOnlineCore | Provider-neutral identity contracts |
| MHOnline | MHOnlineSteam | Steam implementation of identity contracts |

All six modules currently register only FDefaultModuleImpl. Dependencies are private
until a public API requires them. HTTP, Steam SDK and database dependencies will be
introduced with their implementations, not as unused dependencies in skeletons.
SQLite, PostgreSQL and HTTP persistence provider modules follow later inside MHBackend.

Onset retains account/character structures, slots, classes, inventory and quest JSON,
progression, actor ownership and gameplay policy. No MH module owns those schemas.
