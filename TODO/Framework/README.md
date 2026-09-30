# Active framework work

- [x] Recover agreed architecture and record roadmap.
- [x] Create feature/mh-framework branch.
- [x] Scaffold three plugins and six runtime modules.
- [x] Enable plugins in Onset without migrating existing behavior.
- [x] Document dependency, naming, async and migration rules.
- [ ] Pass UE 5.8 editor, game and server builds and plugin-load smoke check.
- [x] Implement MHCore result/error and request lifetime with automation tests.
- [x] Implement backend session signing/verification and configuration.
- [ ] Implement async HTTP transport.
- [ ] Implement persistence contracts and providers.
- [ ] Implement identity contracts and Steam provider.
- [ ] Migrate Onset auth and persistence with regression gates.

See [roadmap](../../Planning/Framework/Roadmap.md) and [backlog](Backlog.md).

Validation: editor/game builds and MH.Core automation pass. Dedicated-server validation
is blocked by the installed engine distribution; requires a server-capable UE 5.8 build.
