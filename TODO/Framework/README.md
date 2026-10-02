# Active framework work

- [x] Recover agreed architecture and record roadmap.
- [x] Create feature/mh-framework branch.
- [x] Scaffold three plugins and six runtime modules.
- [x] Enable plugins in Onset without migrating existing behavior.
- [x] Document dependency, naming, async and migration rules.
- [ ] Pass UE 5.8 editor, game and server builds and plugin-load smoke check.
- [x] Compile/link OnsetServer Win64 Development using source-built UE 5.8.3.
- [x] Validate cooked dedicated-server startup with isolated SQLite and Token auth initialization.
- [ ] Validate client login and runtime auth/persistence flows.
- [x] Implement MHCore result/error and request lifetime with automation tests.
- [x] Implement backend session signing/verification and configuration.
- [x] Implement async HTTP transport and loopback integration tests.
- [x] Implement persistence contract and memory reference tests.
- [x] Implement SQLite provider with worker I/O and durability tests.
- [x] Verify local PostgreSQL fixture access with host libpq smoke test.
- [x] Implement PostgreSQL provider and Unreal integration tests.
- [ ] Implement generic HTTP persistence protocol/provider and server fixture.
- [ ] Implement identity contracts and Steam provider.
- [ ] Migrate Onset auth and persistence with regression gates.

See [roadmap](../../Planning/Framework/Roadmap.md) and [backlog](Backlog.md).

Validation: editor/game builds without unity and eleven MH automation tests pass (2026-10-01).
PostgreSQL contract/timeout tests pass against the local container. One full-suite test collected
an unrelated background Unreal HTTP connectivity warning from the sandbox proxy. The PostgreSQL
module also compiles with no SDK configured. OnsetServer compilation/linking now passes
with source-built UE 5.8.3. Server runtime checks and editor/game revalidation on 5.8.3
remain pending; see [server validation](../../Planning/Framework/Server_Validation.md).
