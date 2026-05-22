# 🌿 **Branching Strategy**

See the [Repository README](../../README.md) for the full project overview and [Episode List](../Outlines/Episode_List.md) for episode contents.

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

See the full [Episode Export Workflow](EPISODE_EXPORT_WORKFLOW.md) for the complete step-by-step process.

1. Work is done on `dev` (or a `feature/*` branch merged into `dev`)
2. Create `episode/XX` from `dev` at the episode snapshot point (see the [Episode List](../Outlines/Episode_List.md) for the full breakdown)
3. Strip advanced features not yet introduced in the episode
4. Copy cleaned snapshot to `Series/EpisodeXX/`
5. Add episode README and verify the snapshot
6. Push `episode/XX` to remote

## **Rules**

- No direct commits to `main` or `dev` — use pull requests / merge requests
- `feature/*` branches branch from `dev`, merge back to `dev`
- `episode/*` branches branch from `dev`, never merge back
- Delete feature/fix branches after merging
- Rebase feature branches onto `dev` before merging to keep history clean
- Never force-push to shared branches (`main`, `dev`)
