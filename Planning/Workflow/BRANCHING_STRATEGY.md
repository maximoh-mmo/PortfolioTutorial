# 🌿 **Branching Strategy**

See the [Repository README](../../README.md) for the full project overview and [Episode List](../Outlines/Episode_List.md) for episode contents.  
This document is part of the [Phase 4 — Repo & Workflow Setup](../../TODO/Checklist.md) deliverables.

## **Branches**

| Branch | Purpose | Base | Lifetime |
|---|---|---|---|
| `main` | Stable full demo, release-ready | — | Permanent |
| `dev` | Active development, integration branch | `main` | Permanent |
| `feature/*` | New systems or significant changes | `dev` | Short-lived |
| `fix/*` | Bug fixes on dev or main | `dev` or `main` | Short-lived |
| `episode/*` | Staging branch for a public episode snapshot | `dev` | Temporary |
| `steam/*` | Steam integration work | `dev` | Short-lived |
| `server/*` | Dedicated server work | `dev` | Short-lived |

## **Merge Flow**

```
main
  ▲
  │ (merge via PR, after QA)
dev
  ▲
  │ (merge via PR, after review)
feature/xxx   fix/xxx   steam/xxx   server/xxx
```

## **Episode Export Flow**

1. Work is done on `dev` (or a `feature/*` branch merged into `dev`)
2. Create `episode/XX` from `dev` at the episode snapshot point (see the [Episode List](../Outlines/Episode_List.md) for the full breakdown):
   ```
   git checkout dev
   git checkout -b episode/XX
   ```
3. Strip advanced features not yet introduced in the episode (refer to the [Architecture Overview](../../Docs/Architecture/Architecture%20Overview.md) to understand which systems belong where)
4. Copy cleaned snapshot to `Series/EpisodeXX/` (see the [Episode README Template](../Templates/EPISODE_README_TEMPLATE.md) for required files)
5. Push `episode/XX` to remote
6. Restore branch protections (if any)

## **Rules**

- No direct commits to `main` or `dev` — use pull requests / merge requests
- `feature/*` branches branch from `dev`, merge back to `dev`
- `episode/*` branches branch from `dev`, never merge back
- Delete feature/fix branches after merging
- Rebase feature branches onto `dev` before merging to keep history clean
- Never force-push to shared branches (`main`, `dev`)
