# Dependency rules

- MHCore depends only on Unreal Core initially.
- MHBackendCore and MHPersistence may depend on MHCore.
- MHBackendHttp depends on MHCore; Unreal HTTP stays implementation-private.
- MHPersistenceSQLite depends on MHPersistence/MHCore; SQLiteCore stays implementation-private.
- MHPersistencePostgres depends on MHPersistence/MHCore; libpq stays private and its SDK path is supplied by the host.
- MHPersistenceHttp depends on MHPersistence/MHCore; MHBackendHttp is implementation-private.
- MHOnlineCore may depend on MHCore, never on Steam.
- MHOnlineSteam may depend on MHOnlineCore; provider SDK details stay private.
- MHInteraction may depend on MHCore and Engine; game-specific interaction rules stay in Onset.
- MHBackend and MHOnline do not depend on each other. Onset coordinates them.
- MH modules never depend on Onset, Lyra or GameFeatures, including header includes.
- Adapters and Onset may consume MH. Do not invert this relationship for convenience.

Declare cross-plugin dependencies in .uplugin and module dependencies in .Build.cs.
Expose public dependencies only when required by a public header. Check both declarations
and includes during review, and build without unity when adding public APIs.
