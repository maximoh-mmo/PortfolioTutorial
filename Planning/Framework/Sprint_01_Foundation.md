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
