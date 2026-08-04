# 🎬 Post 1 — Before a Single Line of Gameplay: Scaffolding a Production Repository

*25 commits · May 22, 2026 · covering repo setup, documentation system, series planning, and pre-production review*

---

## Introduction

Every project I've started in the past has followed the same doomed pattern: create a fresh Unreal project, start moving a character around, and then slowly realize I've built something that can't be shipped, taught, or even understood three months later. The systems grow organically, the naming is inconsistent, and the "how do I explain this to someone" question gets deferred forever.

This time I decided to invert that. Before writing a single gameplay class, I spent a day building the **container** the whole project would live in: a folder skeleton that encodes the roadmap, a documentation system with templates, a full episode outline for a tutorial series, and — crucially — a **two-repository strategy** that separates the messy reality of development from the clean, teachable snapshots viewers will see. This post walks through those first 25 commits and the decisions behind them.

---

## Technical Exposition

### The problem I was solving

I wanted to build a top-down ARPG combat prototype in Unreal Engine **5.8** and simultaneously produce a 36-episode tutorial series about it. Two audiences pull in opposite directions:

- **The demo itself** needs room to experiment, break things, and hold intermediate assets (Steam SDK, server configs, throwaway prototypes).
- **The tutorial** needs clean, minimal, *explainable* snapshots at every step — one feature per episode, no extraneous systems.

The natural answer is **two repos**: a private one as the "master source of truth" and a public one containing only polished episode snapshots. That decision shaped the entire repository layout, and it's visible in the very first commit's folder structure.

### Why the folder structure *is* the architecture

The first commit (`88359a6`) contains no code at all — it's a `.gitignore`, a `README`, and over 80 empty `.gitkeep` directories. But read the directory names as a list and you can see the entire production pipeline:

```
Assets/          Non-Unreal assets (diagrams, images, audio, video)
Design/          Game design documentation
Docs/            Technical system documentation (AI, GAS, Steam, ...)
Experiments/     Throwaway prototypes and tests
Planning/        Episode outlines, scripts, release plan
Project/         THE actual Unreal project (the real game)
Scripts/         Automation
Series/          Clean episode snapshots for the public repo (01-36)
Server/          Dedicated server configs
Steam/           Steam integration
Tools/           Episode exporter, build scripts, profiling
```

Two details stand out to me with hindsight:

1. **`Project/` vs `Series/`.** The Unreal project lives in a subfolder, not the repo root. That's unusual — most UE repos put `.uproject` at the root. But it enforces the mental model that the project is just *one component* of a larger production machine. `Series/Episode01` through `Episode36` are pre-created as placeholders, announcing "the tutorial is the deliverable" from day one.

2. **The security posture was there from the start.** The `.gitignore` already excluded the Steam SDK, server builds, and secret config files:

```
# Private / Sensitive
Steam/SDK/
Steam/*.sdk
Server/Builds/
Server/Config/*.secret.*
Tools/EpisodeExporter/config.local.*
```

No secrets ever needed to be scrubbed later, because they were structurally impossible to commit.

---

## Implementation Details

### 1. Define the "source of truth" README first

The initial README was written before any code existed, and it does two things at once: it describes the *final* demo (click-to-move, targeting, GAS abilities, NPC AI, multiplayer, Steam) and it documents the repository philosophy — *"the public repo will contain only the clean, step-by-step episode snapshots."*

Writing the README first forced me to commit to what the project *was*, so every later decision could be checked against it.

### 2. Bootstrap the Unreal project from the TopDown template

The second commit (`11b0994`) imports a stripped-down version of Unreal's TopDown template. I deliberately kept the core base — `Character`, `PlayerController`, `GameMode`, cursor FX, and the input actions — because it gave me a guaranteed-working starting point, then gutted the demo variants I didn't need.

The `Build.cs` shows the module dependencies I knew I'd need from the start:

```csharp
PublicDependencyModuleNames.AddRange(new string[] {
    "Core",
    "CoreUObject",
    "Engine",
    "InputCore",
    "EnhancedInput",
    "AIModule",
    "NavigationSystem",
    "Niagara",
    "UMG"
});
```

Note what's *not* there yet: `GameplayAbilities`. GAS is a known future requirement, but I kept the foundation minimal — add modules when the system that needs them lands, not before. The project was still named `MPTDARPG` (Multi-Player Top-Down ARPG) at this point.

### 3. Create templates before writing a single doc

Rather than write documentation ad hoc, I created two templates first (`7af828d`):

- `SYSTEM_DOCUMENTATION_TEMPLATE.md` — a structured header: Purpose, Responsibilities, *Non-Responsibilities*, Key Classes, Data Flow, Replication Rules, Edge Cases, Testing Checklist.
- `EPISODE_SCRIPT_TEMPLATE.md` — Goal, Context & Dependencies, High-Level Summary, Technical Breakdown, Code Snippets, Common Pitfalls, Checklist.

The **Non-Responsibilities** section is the unsung hero here. It forces the author to say what a system *doesn't* do, which is how you catch scope creep before it happens.

### 4. Write the system documentation batch

With templates locked, I wrote the first ten system docs in one sweep (`31c241b`): NPC AI, Group System, Player AI, Pooling, Spawner, GAS, Ability Targeting, UI, Multiplayer, Steam. These weren't written from code — they were written from *intent*, describing what each system would do. That's a spec document, and it became the blueprint all implementation work would follow.

A second batch (`c7b78be`) added the Player System, Targeting System, and PvP System docs, and retrofitted the Architecture Overview to include PvP. Notice the pattern: **the docs were the first draft of the design**, and code would later be written to match them — the exact opposite of the usual "code first, document later" instinct.

### 5. Choose the documentation tooling

I briefly tried a Docsify site (auto-generated sidebar, GitHub Pages hosting) in `460b0a8`. One day later I deleted it (`75c1058`) in favor of Obsidian for the private docs, and added Obsidian's folder to `.gitignore`:

```
# Obsidian
.obsidian/
.obsidian/workspace
.obsidian/workspace.json
```

The reasoning: this repo is **private**, so it doesn't need web hosting. Obsidian gives me wiki-style cross-linking (a giant benefit for a 30-doc network), graph view, and free-form exploration — all local. Docsify would matter only if these docs were public-facing, which the architecture deliberately prevents. Tooling should match the *deployment reality* of the content, not the trend of the week.

### 6. Plan the series: outlines, scripts, workflow

The planning commits built the full series production pipeline:

- **Episode List** — 36 episodes grouped into phases, each with a one-sentence goal.
- **Scope Overview** — explicit project boundaries and success criteria.
- **Series Overview** — audience, prerequisites, learning outcomes.
- **Episode scripts 1–5** — full drafts (Project Setup, Top-Down Camera, Click-to-Move, Enemy Spawner, Click-to-Target).
- **Branching Strategy** — `main`/`dev`/`feature/*`/`episode/*` branches, with a rule that *no commits go directly to `main` or `dev`*.
- **Episode Export Workflow** — the process of stripping advanced features from a snapshot before publishing.

Two planning decisions are worth highlighting:

**The branch model encodes the two-repo strategy.** `episode/*` branches branch from `dev` and *never merge back* — they're staging areas for public snapshots. The workflow doc even names the steps: build in `/Project`, create `episode/XX`, strip advanced features, copy to `Series/EpisodeXX/`, push to public. The entire series production is automated as a repeatable process.

**The episode order wasn't sacred.** In `852748a` I swapped episodes 4 and 5 — Enemy Spawner now comes *before* Click-to-Target — because the spawner is an NPC-lifecycle system and targeting is a player-combat system, and they belonged in different phases. This was a pre-production review correction, caught precisely because the docs existed to review.

### 7. Add the diagrams

The `Mermaid` commits embedded diagrams inline across all ten system docs. Mermaid (rather than hand-drawn images) means the diagrams live in the same files as the prose, stay versioned in git, and are trivial to update when the design changes. The architecture doc gained a layered diagram from Player → Player AI → Ability System → NPC AI → Spawner+Pooling → Multiplayer → Steam, which became the visual anchor for the whole project.

### 8. Close pre-production with the risk review

The final commits of the day are pure production management, and they're my favorite part of the arc:

- **Risk Identification** — 40 risks across 5 categories.
- **Risk Mitigation Plan** — a mitigation strategy per risk with `prototype / simplify / delay` guidance.
- **Production Timeline** — ~12.5 weeks for the private demo, ~8 weeks for episode production, a 12-month calendar with a weekly release cadence.
- **Private Demo Checklist** — full A1–A7 task tracking.
- **The rename** — `MPTDARPG` → **`Onset`**, 38 references across 6 files. The project finally had a real name.

The naming of the mitigation guidance is worth stealing: forcing every risk into *prototype it, simplify it, or delay it* prevents the classic failure of writing risks with no decision attached.

---

## Results & Validation

- Repository skeleton with 80+ structured directories and a security-aware `.gitignore` — committed *before* any code.
- 10 system docs + player/targeting/PvP docs, all written against templates, all cross-linked.
- Full series planning: 36-episode outline, 5 drafted scripts, branching strategy, export workflow.
- 40 risks identified with concrete mitigations and a 12-month production timeline.
- Project formally named **Onset**, a clean TopDown-based foundation imported, and a daily TODO cadence (May 27 – Jun 4) queued for the first development phase.

**What was *not* done:** no gameplay code beyond the template base, no GAS, no AI systems. That was the point — the foundation phase was deliberately code-light so that design decisions could be made cheaply, on paper, before they cost hours of refactoring.

---

## Challenges & Solutions

### The Docsify experiment

**Problem:** I wanted a navigable documentation site and reached for Docsify on autopilot — it's the tool everyone shows off.

**Solution:** It took about ten minutes of real use to realize the docs are private and local-first; web hosting added friction without adding value. Switched to Obsidian, which matches the private wiki use-case, and documented the decision in the `.gitignore` rather than leaving a confusing dead-end. **Lesson:** pick tooling based on where the content actually lives and who reads it, not what looks impressive in a demo.

### Scope discipline under pressure

**Problem:** When writing the system docs, it was tempting to document every feature I'd ever wanted (the "everything is core" trap).

**Solution:** The `Non-Responsibilities` template field forced explicit exclusion statements, and the Scope Overview established hard project boundaries. The episode swap (E4/E5) showed the review loop working: because docs existed as a coherent whole, a misplaced episode was *visible* and cheap to fix.

### The "write specs before code" anxiety

**Problem:** There's real cognitive resistance to writing hundreds of lines of documentation for systems that don't exist yet — it feels like wasted work.

**Solution:** I treated the docs as a *specification*, not documentation. They resolved the architecture questions (what replicates, what's server-authoritative, how PvP filtering works) before they cost anything to change. The payoff is that later phases could implement against a known design instead of rediscovering it.

---

## Reflection & Lessons Learned

The single biggest lesson from this phase: **the repository layout is a product decision, not a hygiene chore.** Pre-creating `Series/Episode01-36` before writing a single feature committed me to the tutorial-first philosophy. Pre-writing the security rules in `.gitignore` meant I never had to un-commit a secret. And writing the README *first* gave every subsequent decision an anchor.

I also learned to respect the cost of choice. Each template, doc structure, and branch convention was a *constraint* I chose deliberately — and constraints were what made the fast iteration of later phases possible.

What I'd do differently: I'd probably have skipped the Docsify attempt entirely (it cost two commits for zero lasting value), and I might have drafted the Risk/Mitigation docs slightly earlier since they correctly predicted several real obstacles that showed up months later (input-method conflicts, dedicated-server networking surprises).

The pattern that proved most valuable — and that I'd carry into any project — is **document-first development**: write the specification before the implementation, so the code's job is to make the docs true, not the other way around.

---

## Forward-Looking Content

| Phase | Status |
|---|---|
| Repository scaffold, docs system, series planning | ✅ Complete |
| Pre-production review, risk/timeline, rename to Onset | ✅ Complete |
| A1 — Player core & input (EnhancedInput, camera, movement, targeting) | ⏭️ Next |
| A2 — NPC lifecycle (spawner, pooling, groups) | Planned |
| A3 — AI (StateTrees, perception) | Planned |
| A4 — Combat (GAS, death, corpses) | Planned |
| A5 — Multiplayer & Steam | Planned |

**What's next:** the docs gave me a plan, but the real test starts now. The next phase takes the TopDown base and builds the actual Onset player core — the input-agnostic movement system that supports mouse, touch, keyboard, and gamepad at once. The daily TODOs for May 27 are already queued.

> **Next time in Post 2:** Project bootstrap in earnest — stripping the TopDown template, reinitializing the blank Onset project, and laying down the A1 player systems (input pipeline, camera, movement) that everything else will build on.

---

*Series index and templates live in [`Planning/Outlines/`](../Planning/Outlines/) and [`Planning/Templates/`](../Planning/Templates/). Source of truth: this arc is commits `88359a6` through `2c82a9a`.*
