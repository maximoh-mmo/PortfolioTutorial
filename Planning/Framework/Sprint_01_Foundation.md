# Sprint 01: foundation

Deliver conventions and enable MHCore, MHBackend and MHOnline in Onset. Register six
runtime modules without changing gameplay. Defer persistence providers and service APIs.

Acceptance: parse every descriptor, verify module registration/dependency graph, then
build OnsetEditor, Onset and OnsetServer with UE 5.8. Check plugin load in the editor.
No successful compile claim may be based solely on static descriptor checks.

Next sprint: implement MHCore results, error model and request state machine with lifecycle
tests. Only then extract backend configuration/session logic, followed by async HTTP,
persistence, identity/Steam and Onset adapters in roadmap order.

## Initial validation

Static checks passed for descriptor parsing, project enablement, all six module
registrations and absence of game/adapter dependencies. Whitespace checks passed.
UE 5.8.2 is installed, but UnrealBuildTool stopped before compilation with
System.UnauthorizedAccessException while rotating its LocalAppData log. Requested
build-directory access was not granted. Editor/game/server builds and runtime plugin
loading remain unverified. Existing Onset code has not been migrated.

## Build retry (2026-09-30)

Build-directory permissions were granted on retry, including the Unreal Build Accelerator
cache. UE 5.8.2 successfully compiled and linked all six MH runtime modules for
OnsetEditor Win64 Development. The full editor build encountered LNK1181 in the existing
OnsetDataStore module: Project/Source/Onset/ThirdParty/PostgreSQL/lib/libpq.lib is missing
and is not tracked in this repository. Supply the matching PostgreSQL import library
before repeating the full build. Game/server builds and runtime loading remain pending.

## MHCore validation (2026-09-30)

- OnsetEditor Win64 Development: passed with UE 5.8.2.
- Onset Win64 Development: passed with UE 5.8.2.
- Headless Onset editor: loaded MH modules; MH.Core.Result and
  MH.Core.RequestLifecycle both passed (zero test warnings/errors).
- OnsetServer Win64 Development: blocked before compilation because the installed
  engine distribution does not support server targets. A server-capable UE 5.8 build
  is needed before the migration's server regression gate can pass.

Resolved baseline prerequisites: tracked PostgreSQL import library and changed the
hit-reaction cooldown call to pass the gameplay effect default object, as required
by the UE 5.8 API. Existing GAS deprecation warnings remain outside this milestone.

MHCore now provides typed results/errors and shared request completion/cancellation.
See Docs/Framework/Core.md. Backend/online/persistence modules remain skeletons;
Onset production consumers have not been migrated.
