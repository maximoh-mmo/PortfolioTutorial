# 🧱 OPEN TASKS — Forward Plan

Consolidated open work after the Phase A core systems sweep (last updated 2026-08-11).
Master status lives in `TODO/Private_Demo_Checklist.md`; this file is the actionable forward list.

---

## Priority 1 — Demo finish line (A6.2, A6.3, A7)

The remaining checklist sections. Code for the ability set + HUD is committed, so these are the last gameplay items before the showcase.

### A6.2 — Final Demo Loop
- [ ] Wave spawning — multiple spawners, progressive difficulty
- [ ] Full combat flow: spawn → fight → die → respawn
- [ ] Integrate player abilities into demo loop
- [ ] Integrate multiplayer + Steam auth into final demo
- [ ] Verify loop plays start to finish
- [ ] Verify no crashes during extended session

### A6.3 — Performance Pass
- [ ] Profile NPC tick cost
- [ ] AI LOD — disable/rate-limit AI when far from player (see `Docs/Future_Ideas.md` AI LOD entry)
- [ ] Profile pooling vs direct spawn
- [ ] Profile network replication bandwidth
- [ ] Address bottlenecks found

### A7 — Integration & Hardening
- [ ] A7.1 cross-system bugfixing (9 items — autoplay vs NPC AI, PvP mid-combat, AoE filtering, 50+ NPC pool stress, respawn cascade, Steam/DS stress, network emulation, death mid-StateTree, PvP mid-projectile)
- [ ] A7.2 edge case hardening (8 items — spawner disabled mid-respawn, pool fallback, assist debounce, target auto-switch, PvP mid-ability, NPC loses sight in assist, GroupManager cleanup, client disconnect mid-combat)
- [ ] A7.3 final testing pass (6 items — system doc checklists, single-player/multiplayer/DS loops, export snapshot, 49 risks mitigated)

---

## Priority 2 — Ability icons (UI_ASSET_CHECKLIST Step 8) ✅

Icons assigned in-editor and committed (AoE/Cone previously, Shadowstep now). Source art under `Assets/Images/UI/` is gitignored; only imported Content/ textures are tracked.

- [x] Set `AbilityIcon` on `GA_AoE`, `GA_Cone`, `GA_Shadowstep` Blueprints
- [ ] Verify ability bar slots 1/2 + Shadowstep passive render icons (quick PIE check)
- [ ] After `EditorToolPlan.md` lands: icons become data-driven via `DT_Abilities.AbilityIcon` rows — tracked under Priority 3 Phase 3

---

## Priority 3 — Ability editor tool (`Docs/EditorToolPlan.md`)

Biggest code task, fully designed, unstarted. Data-driven `DT_Abilities` replaces hardcoded grants; introduces Slow vs Snare + damage execution pipeline. 5 phases:

- [x] **Phase 1 — Cooldown plumbing**: `UOnsetCombatAttributeSet` (`CooldownMultiplier`, replicated), `GE_GenericSlow`, `ApplyCooldown` SetDuration override. Verify slow extends enemy cooldowns.
- [x] **Phase 2 — Damage pipeline**: `UOnsetDamageExecution` + `GE_GenericDamage` + invulnerability gate (`TAG_State_Invulnerable` → 0). Verify damage numbers intact; invuln targets take 0.
- [x] **Phase 3 — Data-driven runtime**: `FOnsetAbilityDefinition`, `UOnsetGA_Generic`, `GE_GenericSnare`/`GE_GenericCooldown`, `UOnsetAbilityLibrary`, tag-based row resolution, data-driven `GrantDefaultAbilities`, row-based ability bar slots. Verify parity with today.
- [x] **Phase 4 — Editor tool (code)**: `OnsetEditor` module + `UOnsetAbilityEditorWidget` (list + `UDetailsView` form + Add/Delete/Save), Tools-menu entry hosting it in a nomad tab. **Extra (done)**: `bCreateScroll` — creating an ability also writes a `DT_Scrolls` row (with `GrantedAbility` pointing at the new row); `DeleteDefinition` removes now-dangling scroll rows. **Extra (done)**: per-ability `ThreatMultiplier` in the creation dialog (feeds `FOnsetAbilityDefinition::ThreatMultiplier`, see [Threat System](../Docs/AI/Threat_System.md)). **Remaining (in-editor)**: create `DT_Abilities` asset + populate demo loadout (AoE→1, Cone→2). Verify persists + shows in PIE.
- [ ] **Phase 5 — Extras (optional)**: player `AutoAttackInterval` driven by `CooldownMultiplier`; more effect types; assignment/unlock menu; armor/resist ExecCalc.

---

## Priority 3b — Items, loot & threat multipliers ✅ (code done)

Itemisation + loot pass (per-category item tables, stacked bag, loot tables, click-to-loot, scroll authoring, threat multipliers) is implemented and committed (`3bcf77d`, `6bae296`, `251a70e`). **Remaining (in-editor/content)**: populate `DT_Equipment`/`DT_QuestItems`/`DT_Junk`/`DT_Scrolls`/`DT_Loot` rows (partially done), set Tank `ThreatMultiplier = 1.5` in `DT_ClassInfo`, and author per-ability threat multipliers. Docs: [Inventory & Loot System](../Docs/Inventory/Inventory_System.md).

---

## Priority 4 — Tutorial series production (episodes 44–47, then 48–51)

Episode scripts 40–43 exist in `Planning/Scripts/`; 44–47 have outlines only. Content for 48–51 partially exists (Ep 48's HUD items are already built as A6.1).

- [ ] Script **Episode 44 — Auth Subsystem Extraction** (`UOnsetAuthSubsystem`, slim `PostLogin`, `DirectAuth` default)
- [ ] Script **Episode 45 — Session Token System** (`FOnsetSessionToken`, HMAC-SHA256, `Client_SessionToken` RPC, config-driven lifetime/secret)
- [ ] Script **Episode 46 — Login Server Target** (`OnsetLoginServer.Target.cs`... note: current impl reuses `Onset.exe`, document the deviation)
- [ ] Script **Episode 47 — Client & Game Server Token Flow** (`?Token=` URL, `AuthMode=Direct|Token`, reconnect flow)
- [ ] Map Ep 48 content: HUD items already committed (A6.1) — script can be written, no code pending unless new work is scoped
- [ ] Ep 49–51 depend on A6.2/A6.3 completion

---

## Priority 5 — Housekeeping & optional

- [ ] Delete stale backup artifact `Assets/Images/UI/7158.svg.2026_08_09_16_46_59.1.svg`
- [ ] Decide tracking policy for `Assets/Images/` (commit icon source vs gitignore)
- [ ] (Optional) AI-vs-AI autoplay harness — see `Docs/Future_Ideas.md`; supports demo recording / balance testing
