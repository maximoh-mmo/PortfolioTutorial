# ✅ **Public Release Checklist**

Run through this checklist before pushing any episode snapshot to the public repo.  
See the [Episode Export Workflow](EPISODE_EXPORT_WORKFLOW.md) for the full export process.

---

## **Code**

- [ ] No references to future systems, episodes, or Spoiler content
- [ ] All C++ classes compile with zero warnings
- [ ] No commented-out code for features not yet introduced
- [ ] No debug logs, test functions, or cheat commands exposed
- [ ] Branch name matches `episode/XX` convention

## **Assets**

- [ ] All unused assets removed (starter content, test maps, placeholder meshes)
- [ ] No references to missing or deleted assets
- [ ] Maps clean — no test actors, debug volumes, or editor-only helpers

## **Configuration**

- [ ] `DefaultEngine.ini` — no Steam, server, or multiplayer settings if not yet introduced
- [ ] `DefaultGame.ini` — no references to unreleased systems
- [ ] `DefaultInput.ini` — only input mappings covered so far
- [ ] Project settings match the episode's scope

## **Documentation**

- [ ] `Series/EpisodeXX/README.md` is present and accurate
- [ ] README goal matches the episode script
- [ ] Testing instructions are correct
- [ ] Code snippets match the actual implementation

## **Diagrams**

- [ ] All diagrams referenced in the README are included
- [ ] Diagram names match the episode (no spoilers)

## **Project Health**

- [ ] Project opens without errors
- [ ] Play in Editor works as expected
- [ ] No missing blueprint classes or broken redirectors
- [ ] No stale `.gitkeep` files in empty directories

## **Export**

- [ ] Snapshot lives in `Series/EpisodeXX/`
- [ ] Git status is clean — no unintended files
- [ ] Public repo branch is up to date
