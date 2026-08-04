# 🎬 Post 11 — Server's Word Is Law: HasAuthority Guards, Replication, and the Steam-Authenticated Dedicated Server

*16 commits · Jun 27–29, 2026 · covering Sprint A5: Multiplayer & Steam*

---

## Introduction

The single-player demo works. The AI fights. The menus navigate. But the moment two players connect, everything that was built on the assumption of a single authoritative server gets exposed. Who owns the damage? Who decides who dies? Do NPCs replicate? Does the client see what the server sees?

Sprint A5 answers all of these in 2.5 days of intense work. The arc opens with a pre-sprint audit that reveals exactly what's missing — a checklist of 35 items across four waves — and closes with a fully working two-server auth flow and a comprehensive test script. The through-line is **authority**: every system that mutates state gets a `HasAuthority()` guard, every replicated property gets a `DOREPLIFETIME` entry, and the game server learns to trust nothing from the client it doesn't explicitly validated.

---

## Technical Exposition

### The brute-force authority pass

The sprint doc is explicit about Wave 1:

> "Every StateTree task gets `if (!HasAuthority()) return Failed;` at the top of EnterState/Tick. Brute-force ensures no AI logic leaks to clients."

21 files get guards. All 11 StateTree tasks return `Failed` on the client. Non-actor subsystems (`UWorldSubsystem`) can't call `HasAuthority()`, so they use `GetWorld()->GetNetMode() == NM_Client` — a deliberate two-pattern approach documented in the commit messages.

### Why `OnRep_VisualProfile` instead of replicating mesh/anim directly

Skeletal mesh, animation blueprint, and material assignment are heavy, non-serializable operations. The server replicates only the lightweight `UVisualProfile` DataAsset reference; the client reapplies it locally via `ApplyProfile()` on receipt. This is why the `HasAuthority()` guard was **removed** from `ApplyProfile` — OnRep runs on clients.

The cube fallback (runtime `NewObject<UStaticMeshComponent>`) was deleted because runtime component creation can't replicate. And `IsDataValid()` blocks authoring a profile with no mesh so there's nothing left to fall back to.

### Why the DS uses the Game target + `-server` flag

The `OnsetServer.Target.cs` (Type=Server) was created but the engine distribution doesn't support the Server target type. The DS is just the normal `Onset.exe` launched with `-server`. This single constraint drove the whole Wave 3 approach and `RunDS.ps1`.

---

## Implementation Details

### 1. Replication foundation

`AOnsetBaseCharacter` gets `bReplicates = true`, `GetLifetimeReplicatedProps` with `DOREPLIFETIME_CONDITION(bIsAlive, COND_None)`, and an `OnRep_bIsAlive()` that hides/disables the pawn client-side when dead:

```cpp
void AOnsetBaseCharacter::OnRep_bIsAlive()
{
    SetActorHiddenInGame(!bIsAlive);
    SetActorEnableCollision(bIsAlive);
}
```

`AOnsetEnemy` adds `SetReplicateMovement(true)` and `VisualProfile` as a `ReplicatedUsing=OnRep_VisualProfile` property. AI controllers are explicitly *not* replicated — they're server-only by design.

### 2. The GAS replication bug

The movement speed modifier used `NewObject<UGameplayEffect>` to create infinite GEs dynamically. This produced a client error: `'Received ReplicatedGameplayEffect with no UGameplayEffect def'` — because the replication system can't find the CDO of a transient object. The fix is a real `UOnsetMovementSpeedModifierEffect` class with `SetByCaller` magnitude:

```cpp
FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
    UOnsetMovementSpeedModifierEffect::StaticClass(), 1.0f, ASC->MakeEffectContext());
FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
Spec->SetSetByCallerMagnitude(FName("MoveSpeedMod"), Magnitude);
return ASC->ApplyGameplayEffectSpecToSelf(*Spec);
```

This is the same pattern that survives into the current codebase — a proper GE class with SetByCaller for the runtime value, replicated by class reference, not by object pointer.

### 3. Steam auth flow

The client requests a ticket, sends it to the server via reliable RPC, the server validates it with Steam's `BeginAuthSession`, and the client gets a signed session token back:

```cpp
const FString AuthTicket = Identity->GetAuthToken(0);
Server_SendAuthTicket(AuthTicket);
GetWorldTimerManager().SetTimer(AuthTimeoutTimerHandle, this, &AOnsetPlayerController::OnAuthTimeout, 10.0f, false);
```

The server validates and calls `Client_ClearAuthTimeout()` to close the client-side timer — without this, the client would log "auth timed out" even on successful validation.

### 4. The dedicated server build

The DS is launched as a standalone process with the `-server` flag and the login map:

```powershell
$dsArgs = "`"$ProjectPath`" $MapPath -server -log"
Start-Process -FilePath $DSBinary -ArgumentList $dsArgs -WindowStyle Normal
```

The DS has no viewport, no local player, no HUD. It runs headless, authenticating via Steam tickets and managing the game state for connected clients.

### 5. The two-server split

The architecture separates auth/character-select (login server, port 7777, Direct mode) from gameplay (game server, port 7778, Token mode). The token carries the `SlotIndex` so the game server knows which character to spawn:

```cpp
FString URL = FString::Printf(TEXT("%s:%s/Game/Maps/DemoLevel?Token=%s"),
    *ServerIP, *ServerPort, *Token);
ClientTravel(URL, TRAVEL_Absolute);
```

---

## Results & Validation

- `HasAuthority()` guards on all 11 StateTree tasks + spawner + pool + threat + corpse subsystems
- `bReplicates` on `AOnsetBaseCharacter`, `DOREPLIFETIME_CONDITION(bIsAlive)`, `SetReplicateMovement(true)` on enemies
- `VisualProfile` replicated via `OnRep_VisualProfile`; cube fallback removed
- GAS speed modifier using proper `UOnsetMovementSpeedModifierEffect` class (no more replication error)
- Steam auth ticket flow: request → RPC → validate → token → kick timer
- DS build: Game target + `-server` flag, `RunDS.ps1` with editor + standalone paths
- Two-server architecture: lobby DS (Direct auth) + game DS (Token auth)
- `Test_All.ps1` comprehensive test suite (4 phases, 43 checkpoints)
- A5 = 42/42 (100%)

---

## Challenges & Solutions

### The runtime GE replication error

**Problem:** `NewObject<UGameplayEffect>` has no findable CDO on clients; replication fails silently.

**Solution:** Real `UOnsetMovementSpeedModifierEffect` class with `SetByCaller` magnitude. The value travels via the SetByCaller key, not the object pointer.

### The cube fallback can't replicate

**Problem:** Runtime `NewObject<UStaticMeshComponent>` creates components that don't replicate.

**Solution:** Delete the cube fallback entirely. `VisualProfile` replication handles all visual cases; profiles without a mesh are blocked by `IsDataValid()` at authoring time.

### The `GGameIni` vs `GEngineIni` auth config gap

**Problem:** Auth config lived in `DefaultEngine.ini` but was read through `GGameIni` — the game-specific INI that doesn't exist on the DS.

**Solution:** Read auth config through `GEngineIni` (always present), remove `if (GConfig->GetString(...))` wrappers that silently swallowed missing keys.

### The token address mismatch

**Problem:** `GetPlayerNetworkAddress()` returns `IP:port` but `PendingTokenAuthMap` was keyed by bare IP.

**Solution:** Strip the port before lookup. A one-character fix that prevented the entire token validation from working.

### The `steam://` URL incompatibility

**Problem:** `steam://` travel URLs only work when Steam is running; `-NOSTEAM`/Null-OSS dev fails.

**Solution:** Switch to direct `IP:Port/Path?Token=` URLs. The `-NOSTEAM` flag in `Test_All.ps1` ensures consistent Null-OSS testing.

### The `FGenericPlatformMisc::GetSHA256Signature` assertion

**Problem:** The platform SHA-256 API asserts on platforms without hardware/OS SHA-256 support.

**Solution:** Custom pure-software `FSHA256` implementation. The same code that powers the HMAC signing on both login and game servers — identical behavior guaranteed because both compute the same HMAC over the same payload with the same secret.

---

## Reflection & Lessons Learned

The arc's deepest lesson is that **authority is a property of the system, not a feature you bolt on.** The `HasAuthority()` guards had to be added systematically — a grep-based audit, not a per-feature check. One missed guard in a StateTree task means AI logic runs on clients, producing desync that's invisible in PIE but catastrophic in multiplayer.

The DS build constraint (Game target + `-server` flag, not a Server target) taught me to respect engine distribution limitations. The Server target type exists in the editor but not in shipping builds. Every architectural decision about the DS flows from this single constraint.

The two-server split (login + game) is the arc's most significant architectural decision. It separates concerns cleanly: the login server is a thin, disposable front door that validates tickets and issues tokens; the game server is platform-agnostic, validating tokens and loading account data. This mirrors live-service MMO architecture at a demo scale.

What I'd do differently: I'd have written the `Test_All.ps1` before the auth flow, not after. The test script is the safety net that proves the multi-server flow works; having it from day one of the arc would have caught the token address mismatch and the kick-timer bug earlier.

---

## Forward-Looking Content

| Phase | Status |
|---|---|
| A5 — Multiplayer + Steam | ✅ **100% (42/42)** |
| A5b — Persistence & Account System | ⏭️ Next |
| A5c — Auth Extraction & Login Server | Planned |

**What's next:** the auth spine is done, but a game needs a hero. Sprint A5b adds the persistence layer — SQLite store, SteamID resolution, character select, and the world-travel flow that ties the login server to the game server. The account API (Lambda + DynamoDB) is planned for A5c.

> **Next time in Post 12:** Remember You — SQLite persistence, SteamID resolution, and the three-slot character select that finally makes every death permanent-ish.

---

*Arc commits: `e854da8` → `91d709c`. Related docs: `Docs/Multiplayer/Multiplayer_System.md`, `Docs/Steam/Steam_Integration_System.md`, `Docs/Architecture/Architecture Overview.md`, `Planning/Sprint_A5_Multiplayer.md`.*
