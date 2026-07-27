# 🏃 Sprint A5c: Auth Subsystem Extraction & Login Server

**Goal:** Extract Steam auth logic from `AOnsetGameModeBase` into a dedicated `UOnsetAuthSubsystem`, introduce a session token system, and build a minimal Login Server target. The existing `DirectAuth` flow remains untouched; `TokenAuth` mode is the new path for production deployments with a separate login server.

**Target:** Game server no longer calls Steam APIs directly. Login Server validates tickets and issues signed tokens. Game servers validate tokens and load account data. Architecture is clean, testable, and documented for tutorial episodes.

**Dependencies:** A5.4 (Steam Auth — ticket validation), A5b.1 (PlayerDataSubsystem foundation), A5b.4 (Login RPC flow)

---

## 📊 Sprint Overview

| Section | Tasks | Est. Days |
|---------|-------|-----------|
| Wave 1 — Auth Subsystem Extraction | 8 | 1 |
| Wave 2 — Session Token System | 6 | 0.5 |
| Wave 3 — Login Server Target | 7 | 1 |
| Wave 4 — Client & Game Server Token Flow | 8 | 1 |
| Wave 5 — Cleanup, Documentation, Hardening | 5 | 0.5 |
| **Sprint Total** | **34** | **~4 days** |

---

## 📋 Sprint Waves

### Wave 1 — Auth Subsystem Extraction (Day 1, ~1d)
**Extract all Steam auth logic from `AOnsetGameModeBase` into a focused world subsystem so the game mode no longer cares how auth works.**

- [x] Create `UOnsetAuthSubsystem` (world subsystem, DS only, with `ShouldCreateSubsystem`)
- [x] Define subsystem interface:
  - `HandlePostLogin(APlayerController*)` — validates ticket, resolves platform ID, triggers account load
  - `HandleLogout(AController*)` — cleanup on disconnect
  - `GetAuthMode()` returns current mode enum (`Direct`, `Token`)
- [x] Move `AOnsetGameModeBase::ValidateAuthTicket` implementation into subsystem:
  - Empty ticket → kick player
  - Clear auth timeout + send Client_ClearAuthTimeout
  - Store ticket on `AOnsetPlayerState` (`SteamAuthTicket`)
- [x] Move `Server_SendAuthTicket` routing — PC delegates to `Auth->ValidateAuthTicket()` instead of `GM->ValidateAuthTicket()`
- [x] Add config key `[Onset.Auth] AuthMode=Direct` (default, preserves existing behavior)
- [x] Slim down `AOnsetGameModeBase::PostLogin`:
  - `Auth->HandlePostLogin(NewPlayer)`
  - Add `Logout()` override calling `Auth->HandleLogout()`

### Wave 2 — Session Token System (Day 1-2, ~0.5d)
**Design and implement a self-contained session token format. Tokens are HMAC-signed, time-limited, and opaque to clients.**

- [x] Design `FOnsetSessionToken` struct (in `OnsetAuthSubsystem.h`):
  ```cpp
  struct FOnsetSessionToken {
      FString PlatformID;
      FString Platform;
      int64   ExpiryUnix = 0;
      FString Signature;
  };
  ```
- [x] Implement `GenerateToken(const FString& Platform, const FString& PlatformID)`:
  - Payload: `PlatformID|Platform|ExpiryUnix`
  - HMAC-SHA256 with configured secret key (`AuthTokenSecret`)
  - Expiry from config (`AuthTokenLifetimeSeconds`, default 300s / 5 min)
  - Output: `Base64(payload).Base64(signature)`
  - HMAC implemented manually using `FGenericPlatformMisc::GetSHA256Signature`
- [x] Implement `ValidateToken(const FString& TokenStr, FString& OutPlatform, FString& OutPlatformID)`:
  - Split on `.`
  - Decode payload and verify HMAC signature matches
  - Check expiry against current UTC time
- [x] Add config keys under `[Onset.Auth]`:
  - `AuthTokenSecret` (FString) — shared secret between Login Server and Game Servers
  - `AuthTokenLifetimeSeconds` (int32, default=300)
- [x] Add `Client_SessionToken(FString Token)` RPC to `AOnsetPlayerController`
- [x] Add `Client_SessionTokenFailed(FString Reason)` for error reporting

### Wave 3 — Login Server Target (Day 2-3, ~1d)
**Build a minimal dedicated server target whose only job is auth: receive ticket → validate → issue token → disconnect.**

- [x] ~~Create `Source/OnsetLoginServer.Target.cs`~~ — **Removed.** UE distribution doesn't support Server targets. Uses existing `Onset.exe` with LoginServer map override.
- [x] Create `AOnsetLoginServerGameMode` (in Onset module):
  - Inherits `AOnsetGameModeBase`
  - Override `PostLogin(APlayerController*)`:
    1. Super::PostLogin (auth subsystem handles ticket + token generation)
    2. Send `Client_SessionToken` via auth subsystem
    3. Kick player after 2s delay (timer-based)
- [x] Create `Scripts/RunLoginServer.ps1`:
  - Launches `Onset.exe /Game/Maps/LoginServer -server -log`
  - Falls back to UnrealEditor.exe if no standalone binary
- [x] Add `[OnsetLoginServer]` and `[Onset.GameServer]` config sections to `DefaultEngine.ini`

### Wave 4 — Client & Game Server Token Flow (Day 3-4, ~1d)
**Update the client to connect to Login Server first, obtain a token, then reconnect to the Game Server. Game Server validates the token instead of calling Steam.**

- [x] **Game Server token validation** in `AOnsetGameModeBase::PreLogin` (TokenAuth mode):
  - Extract `?Token` from `Options` via `UGameplayStatics::ParseOption`
  - Call `UOnsetAuthSubsystem::PreLoginTokenAuth()` — validates + caches in `PendingTokenAuthMap`
  - If invalid/expired: set `ErrorMessage`, reject connection
- [x] **Game Server token usage** in `UOnsetAuthSubsystem::HandlePostLogin` (TokenAuth mode):
  - Look up `PendingTokenAuthMap` by player network address
  - Extract `PlayerPlatformID`, `PlayerPlatform` from cached entry
  - Set on `AOnsetPlayerState`
  - Proceed with `UOnsetPlayerDataSubsystem::LoadAccount()` (existing flow)
- [x] **DirectAuth backward compatibility**:
  - `AuthMode=Direct`: existing flow unchanged (no token needed)
  - `AuthMode=Token`: requires valid token, rejects without one
  - Config mode checked at runtime in `HandlePostLogin` / `PreLoginTokenAuth`
- [ ] **Client token flow update** — **Not implemented (stub).** Client-side reconnect flow deferred to Blueprints/UI implementation:
  - Need `LoginServerIP`, `LoginServerPort` config keys
  - Client flow: connect to Login Server → auth → receive token → disconnect → reconnect to Game Server with `?Token=<token>`
  - `Client_SessionToken` RPC exists and logs receipt
  - Token stored on `AOnsetPlayerController` for future use
- [ ] **Security note**: Token replay prevention — token is single-use or tied to client IP
  - Document as future enhancement (not implemented in stub)

### Wave 5 — Cleanup, Documentation, Hardening (Day 4, ~0.5d)
**Remove stale auth code from GameMode, update architecture docs, and write the sprint postmortem.**

- [x] Remove `ValidateAuthTicket()` from `AOnsetGameModeBase` (moved to subsystem)
- [x] Add `Logout()` override to `AOnsetGameModeBase` delegating to subsystem
- [x] Update `AOnsetPlayerController::Server_SendAuthTicket` — route to subsystem, not GameMode
- [x] Add `[Onset.Auth]` section to `DefaultEngine.ini` with defaults
- [x] Add `[OnsetLoginServer]` and `[Onset.GameServer]` config sections
- [x] Update `DefaultEngine.ini` — `AuthTokenSecret`, `AuthTokenLifetimeSeconds`
- [ ] Create `TODO/DONE/07-27-26.md` — sprint completion record (this file)
- [x] Update `TODO/Private_Demo_Checklist.md` — mark A5c all done (**remaining below**)
- [ ] Client token reconnect flow — deferred to Blueprints/UI
- [ ] **Architecture diagram** — add auth flow diagram to sprint doc

---

## ⚠ Risks & Mitigations

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| Auth subsystem accidentally breaks DirectAuth | Players cannot connect at all | Low | Keep DirectAuth as default mode. Test regression before merging. Both modes coexist. |
| Login Server and Game Server config drift | Token signed with different secret → all tokens rejected | Medium | Shared config file or env variable for `AuthTokenSecret`. CI checks both targets use same secret. |
| Client flow change breaks existing client-server connection | Players stuck at "Connecting" | Medium | DirectAuth mode untouched. TokenAuth mode gated behind config. Client only uses token flow when `LoginServerIP` is configured. |
| Login Server target fails to compile | Blocked sprint | Low | Small target. Can debug quickly. Fallback: test subsystem + token logic in PIE using DirectAuth mode. |
| HMAC library not available in UE5.8 | Token system needs alternative signing | Low | `FSecureHash::HMACSHA256` exists in UE5. Alternative: simple XOR + Base64 for stub, upgrade to AES later. |
| Client forced disconnect from Login Server causes engine issues | Softlock or crash | Medium | Use non-seamless travel with clean disconnect. Test disconnect flow in PIE before Login Server launch. |

---

## 📐 Design Decisions

**World subsystem vs. game mode subclass:** A world subsystem (`UOnsetAuthSubsystem`) is the right home for auth logic. It's accessible from anywhere in the world, survives level transitions, and doesn't require subclassing `AGameModeBase`. The game mode delegates to it via a single `HandlePostLogin` call, keeping the game mode focused on gameplay orchestration.

**HMAC-signed token, not JWT:** JWT libraries add external dependencies. UE has built-in HMAC support (`FSecureHash`). The token format (`Base64(payload).Base64(signature)`) is JWT-like but implemented in ~50 lines of UE C++. Upgrade path: replace with JWTs when an OAuth/OpenID layer is needed.

**Login Server as a separate UE target, not a standalone app:** Using a UE target (`OnsetLoginServer.Target.cs`) reuses the entire Steam OSS integration, the auth subsystem, config parsing, and logging infrastructure. A standalone app would duplicate all of that. The cost is a larger binary; the benefit is zero maintenance overhead for the auth code.

**Token in URL option, not a separate RPC:** UE's `FURL` already supports key-value options. `?Token=<token>` is natural, works with `ServerTravel`, and is parsed by `UGameplayStatics::ParseOption`. No new connection protocol needed.

**Client disconnect-reconnect pattern, not seamless travel:** The Login Server and Game Server are separate processes (potentially separate machines). UE doesn't support seamless travel between processes. The client cleanly disconnects from Login Server and connects to Game Server as a fresh connection. The token bridges the identity gap.

**Config-driven auth mode, not compile-time switch:** `AuthMode=Direct|Token` in `[Onset.Auth]` means the same binary can be either a Login Server or a Game Server depending on its config. Simplifies deployment and testing. During development, use `Direct` mode for the main DS and `Token` mode only for the integrated test.

**Auth mode enums:**
```cpp
UENUM()
enum class EOnsetAuthMode : uint8 {
    Direct,   // GameMode handles Steam auth directly (existing flow)
    Token     // Client presents token, GameServer validates
};
```

---

## 🔗 Dependencies

| System | Dependency Type |
|--------|-----------------|
| A5.4 Steam Auth | Required — ticket validation must work before subsystem extraction |
| A5b.1 PlayerDataSubsystem | Required — account loading is the final step after token validation |
| A5b.4 Login RPCs (Client_AccountData, Server_SelectCharacter) | Required — game server uses existing RPCs for account flow |
| Steamworks SDK headers | Required — `ISteamGameServer::BeginAuthSession()` used by subsystem |
| UE5 `FSecureHash` | Required — HMAC-SHA256 for token signing (built-in, no external dep) |
| None beyond A5b | No new third-party dependencies |
