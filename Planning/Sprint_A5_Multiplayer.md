# 🏃 Sprint A5: Multiplayer & Steam

**Goal:** Make the entire demo server-authoritative and multiplayer-safe, supporting dedicated servers and Steam-authenticated sessions.
**Target:** 2-client + dedicated server session with Steam auth, full combat loop replicating correctly.
**Estimate:** ~10 working days (4 waves)
**Dependencies:** All A1-A3 systems, A4 (GAS basics — Basic Attack, Hit Reaction, Death/Corpse)

---

## 📊 Current State

| Section | Tasks | Done | % | Remaining |
|---------|-------|------|---|-----------|
| A5.1 Server/Client Authority | 6 | 5 | 83% | 1 (PIE verify) |
| A5.2 Replication Pass | 12 | 10 | 83% | 2 (targeting validation, 2-client test) |
| A5.3 Dedicated Server Build | 7 | 0 | 0% | 7 |
| A5.4 Steam Auth Integration | 10 | 6 | 60% | 4 |
| **Sprint Total** | **35** | **21** | **60%** | **14** |

### Pre-Sprint Audit: What Already Exists
- GAS attributes replicate: Health, MaxHealth, MovementSpeed via DOREPLIFETIME
- bIsPvPEnabled replicates via AOnsetPlayerState
- Server_SetPvPEnabled RPC exists on AOnsetPlayerController
- AOnsetAIController has HasAuthority() guards in BeginPlay and OnPerceptionUpdated
- AOnsetCorpse sets bReplicates = true
- OnlineSubsystem commented out in Build.cs
- No Steam plugin in .uproject
- No bReplicates on AOnsetBaseCharacter, AOnsetPlayerCharacter, or AOnsetEnemy
- No SetReplicateMovement on any pawn
- No GetLifetimeReplicatedProps on base character or enemy

---

## 📋 Sprint Waves

### Wave 1 — Server/Client Authority (Days 1-2, ~2d)
**Add HasAuthority() guards to every server-only system.**

- [ ] Audit all systems for server-only requirements
- [ ] Add HasAuthority() guards to all StateTree tasks (EnterState/Tick)
- [ ] Add HasAuthority() guards to Spawner spawn/destroy logic
- [ ] Add HasAuthority() guards to PoolSubsystem (ReturnToPool, GetPooledEnemy)
- [ ] Add HasAuthority() guards to CorpseSubsystem
- [ ] Add HasAuthority() guards to ThreatSubsystem public API
- [ ] Add HasAuthority() guard to InteractionComponent
- [ ] Add HasAuthority() guard to PlayerController ability activation
- [ ] Configure GameMode/GameState for multiplayer
- [ ] Add bReplicates = true to AOnsetBaseCharacter constructor
- [ ] Add bReplicates = true to AOnsetAIController constructor
- [ ] PIE test: launch listen server + client, verify connection
- [ ] PIE test: verify no authority errors in log

### Wave 2 — Replication Pass (Days 3-5, ~3d)
**Replicate NPCs, player state, abilities, and targeting.**

- [ ] Add GetLifetimeReplicatedProps to AOnsetBaseCharacter/AOnsetEnemy
- [ ] Set SetReplicateMovement(true) on NPCs
- [ ] Verify NPC movement is smooth on client (PIE 2-window)
- [ ] Verify GAS health replicates correctly to client
- [ ] Verify PvP flag OnRep fires correctly
- [ ] Verify GAS abilities + cooldowns replicate (native GAS replication)
- [ ] Add HasAuthority() to GrantDefaultAbilities (guard already exists, verify)
- [ ] Audit all StateTree tasks for client-side early-out
- [ ] PIE test: 1 server + 2 clients, full combat loop
- [ ] Verify death/corpse/pool replication
- [ ] Verify no client-side exceptions

### Wave 3 — Dedicated Server (Days 6-7, ~2d)
**Build, launch, and verify dedicated server.**

- [ ] Add OnlineSubsystem module to Build.cs
- [ ] Add OnlineSubsystemSteam plugin to .uproject
- [ ] Create DS build configuration
- [ ] Build DS target from command line
- [ ] Fix any platform-specific compile errors
- [ ] Create launch script (RunDS.ps1) for DS + client
- [ ] Test DS + 1 client connection
- [ ] Test DS + 2+ client connection
- [ ] Verify AI behaviour is identical on DS vs PIE
- [ ] Security audit: Server_/Client_ RPC permissions, no client-side authority exploits

### Wave 4 — Steam Auth Integration (Days 8-10, ~3d)
**Implement Steam authentication flow.**

- [x] Add OnlineSubsystemSteam to .uproject plugins with "Enabled": true
- [x] Configure DefaultEngine.ini — OnlineSubsystemSteam with AppId=480
- [x] Implement Steam subsystem init + runtime detection
- [x] Client: RequestAuthTicket() via IOnlineSubsystem::Get()->GetIdentityInterface()
- [x] PlayerController: Server_SendAuthTicket(const FString&) reliable RPC
- [x] GameMode: ValidateAuthTicket() — server-side ticket validation
- [x] Handle "Steam not running" — graceful fallback to LAN mode
- [ ] Handle invalid/expired ticket — reject client with message
- [ ] Handle ticket validation timeout (10s timer, retry/disconnect)
- [ ] Verify auth flow with AppID 480
- [ ] Verify invalid tickets rejected (garbage data test)
- [ ] Verify clients can join Steam-authenticated session
- [ ] End-to-end: DS + Steam auth + 2 auth'd clients -> full combat loop
- [ ] Update Private_Demo_Checklist.md progress

---

## ⚠ Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| StateTree task authority guards missed during audit | Client runs AI logic -> desync | Medium | Systematic audit: grep all FOnsetStateTreeTask derived classes, add guard template |
| Client authority exploit via RPC spoofing | Cheating | Medium | Validate all Server_ RPC inputs server-side; never trust client data |
| DS build fails on UE 5.8 platform-specific issues | Blocked | Medium | Test compile early (Day 6), iterate on errors; fallback to listen-server |
| GAS replication prediction conflicts | Visual desync | Low | Rely on UE GAS replication defaults — test with net emulation in A7 |
| Steam auth flow changes between UE 5.8 and docs | Integration fails | Low | Use stable IOnlineSubsystem interface; LAN fallback always works |
| NPC movement jitter on client | Poor feel | Medium | Test SetReplicateMovement + CharacterMovementComponent defaults |

---

## 📐 Design Decisions for This Sprint

**Authority pattern:** Every StateTree task gets `if (!HasAuthority()) return Failed;` at the top of EnterState/Tick. Brute-force ensures no AI logic leaks to clients.

**Targeting replication:** Clients run their own traces for targeting visuals (current behaviour). Server validates the target when an ability is activated. No need to replicate CurrentTarget.

**GAS replication:** UE's GAS handles ability activation, GE application, and cooldown tag replication natively. No custom replication needed for the ability system itself.

**Steam auth wrapper:** No custom USteamSubsystem class — use IOnlineSubsystem interface directly from PlayerController/GameMode. Keeps the footprint minimal.

**PvP flag:** Already replicates via bIsPvPEnabled on AOnsetPlayerState. Verify OnRep fires on clients. No additional work needed.

**DS build:** Use UnrealBuildTool command line. No CI/CD pipeline — manual build for now.
