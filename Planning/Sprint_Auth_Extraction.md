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

- [ ] Create `UOnsetAuthSubsystem` (world subsystem, DS only, created in `AOnsetGameModeBase::StartPlay` or `Init()`)
- [ ] Define subsystem interface:
  - `HandlePostLogin(APlayerController*, const FString& AuthTicket)` — validates ticket, resolves platform ID, triggers account load
  - `HandleLogout(APlayerController*)` — cleanup on disconnect
  - `GetAuthMode()` returns current mode enum (`Direct`, `Token`)
- [ ] Move `AOnsetGameModeBase::ValidateAuthTicket` implementation into subsystem:
  - `SteamGameServer()->BeginAuthSession()` call
  - SteamID → FString conversion
  - Storage on `AOnsetPlayerState` (`PlayerPlatformID`, `PlayerPlatform`)
- [ ] Move `Server_SendAuthTicket` handling from `AOnsetGameModeBase` to subsystem:
  - Store ticket bytes in subsystem for validation
  - Move timeout timer handling
- [ ] Move `Client_ClearAuthTimeout` RPC handler to subsystem
- [ ] Add config key `[Onset.Auth] AuthMode=Direct` (default, preserves existing behavior)
- [ ] Slim down `AOnsetGameModeBase::PostLogin`:
  - `GetAuthSubsystem()->HandlePostLogin(NewPlayer, AuthTicket)`
  - Remove all direct Steam API calls
  - Remove raw `Server_SendAuthTicket` handler
- [ ] Slim down `AOnsetGameModeBase::Logout`:
  - `GetAuthSubsystem()->HandleLogout(Player)`

### Wave 2 — Session Token System (Day 1-2, ~0.5d)
**Design and implement a self-contained session token format. Tokens are HMAC-signed, time-limited, and opaque to clients.**

- [ ] Design `FOnsetSessionToken` struct:
  ```cpp
  USTRUCT(BlueprintType)
  struct FOnsetSessionToken {
      FString PlayerPlatformID;    // e.g. "76561197960287930"
      FString PlayerPlatform;      // e.g. "Steam"
      int64   ExpiryUnix;          // UTC unix timestamp
      FString Signature;           // HMAC-SHA256 hex digest
  };
  ```
- [ ] Implement `GenerateToken(const FString& PlatformID, const FString& Platform)`:
  - Payload: `V1:{PlatformID}:{Platform}:{ExpiryUnix}`
  - HMAC-SHA256 with configured secret key (`AuthTokenSecret`)
  - Expiry from config (`AuthTokenLifetimeSeconds`, default 300s / 5 min)
  - Output: `Base64URL(payload).Base64URL(signature)`
  - Use UE's `FSecureHash::HMACSHA256` or `FBase64` for encoding
- [ ] Implement `ValidateToken(const FString& TokenString)`:
  - Split on `.`
  - Decode header and verify HMAC signature matches
  - Check expiry against current UTC time
  - Return `FOnsetSessionToken` with parsed fields (or empty on failure)
- [ ] Add config keys under `[Onset.Auth]`:
  - `AuthTokenSecret` (FString) — shared secret between Login Server and Game Servers
  - `AuthTokenLifetimeSeconds` (int32, default=300)
- [ ] Add `Client_SessionToken(const FString& Token)` RPC to `AOnsetPlayerController`:
  - Called by Login Server after successful auth
  - Client stores token for reconnection to Game Server
- [ ] Add `Client_SessionTokenFailed(const FString& Reason)` for error reporting

### Wave 3 — Login Server Target (Day 2-3, ~1d)
**Build a minimal dedicated server target whose only job is auth: receive ticket → validate → issue token → disconnect.**

- [ ] Create `Source/OnsetLoginServer.Target.cs`:
  - `TargetType = TargetRules.TargetType.Game`
  - `bUseLoggingInGameInstanceSpecific = true`
  - Minimal link dependencies (no editor, no client code)
- [ ] Create `Source/OnsetLoginServer/OnsetLoginServer.Build.cs`:
  - `PublicDependencyModuleNames: "Onset", "OnlineSubsystemSteam", "CommonUI"`
  - Mark as server-only target
- [ ] Create `Source/OnsetLoginServer/Private/LoginServerGameMode.h/.cpp`:
  - Inherits `AOnsetGameModeBase` (for base auth setup) or bare `AGameModeBase`
  - No NPC spawning, no combat, no world tick overhead
  - Override `PostLogin(APlayerController*)`:
    1. Wait for auth ticket from client (via `Server_SendAuthTicket`)
    2. Validate via `UOnsetAuthSubsystem` (Steam `BeginAuthSession`)
    3. If valid: call `GenerateToken()` → send `Client_SessionToken` → kick client
    4. If invalid: send `Client_SessionTokenFailed` → kick client
    5. Timeout (10s): kick with timeout error
  - Override `PreLogin(const FString& Options, ...)`:
    - Accept all connections (validation happens in PostLogin)
  - Disable player pawn spawning (no `DefaultPawnClass` needed)
- [ ] Create `Source/OnsetLoginServer/Private/LoginServerGameState.h/.cpp`:
  - Minimal stub (empty subclass of `AGameStateBase`)
- [ ] Create `/Game/Maps/LoginServer`:
  - Empty persistent level
  - Default game mode override = `ALoginServerGameMode`
- [ ] Create `Scripts/RunLoginServer.ps1`:
  - Launches LoginServer.exe with `-log -server -Map=LoginServer`
  - Optional: `-AuthTokenSecret=<secret>` override
- [ ] Add `[OnsetLoginServer]` config section to `DefaultEngine.ini`:
  - Login server port (default: 7777, same as game server — separate instance)
  - Log level for auth tracing

### Wave 4 — Client & Game Server Token Flow (Day 3-4, ~1d)
**Update the client to connect to Login Server first, obtain a token, then reconnect to the Game Server. Game Server validates the token instead of calling Steam.**

- [ ] **Client token flow update** in `AOnsetPlayerController` or `UOnsetUISubsystem`:
  - Add `LoginServerIP` (FString) and `LoginServerPort` (int32) config keys under `[Onset.Auth]`
  - Client flow:
    1. Attempt connection to Login Server at `LoginServerIP:LoginServerPort`
    2. Send auth ticket via existing `Server_SendAuthTicket`
    3. On `Client_SessionToken`: store token in `CachedSessionToken` member
    4. Disconnect from Login Server (`FWorldContext::TravelURL = ""` / force disconnect)
    5. Connect to Game Server at configured game server address
    6. Pass token via URL: `game.exe/Game/Maps/DemoLevel?Token=<token>`
  - On `Client_SessionTokenFailed`: show error, retry or fall back to DirectAuth
  - Timeout (15s): fall back to DirectAuth with warning
- [ ] **Game Server token validation** in `AOnsetGameModeBase::PostLogin` (TokenAuth mode):
  - Extract `?Token` from `Options` string passed to `PostLogin`
  - Call `UOnsetAuthSubsystem::ValidateToken(Token)`
  - If valid:
    - Extract `PlayerPlatformID`, `PlayerPlatform` from token
    - Set on `AOnsetPlayerState`
    - Proceed with `UOnsetPlayerDataSubsystem::LoadAccount()` (existing flow)
    - Send `Client_AccountData` (existing flow)
  - If invalid/expired:
    - Log warning, reject player with reason
    - Send `Client_AccountData` with empty/error state
- [ ] **URL token parsing** in `AOnsetGameModeBase::PreLogin` or `PostLogin`:
  - `UGameplayStatics::HasOption(Options, TEXT("Token"))`
  - `UGameplayStatics::ParseOption(Options, TEXT("Token"))`
- [ ] **DirectAuth backward compatibility**:
  - `AuthMode=Direct`: existing flow unchanged (no token needed)
  - `AuthMode=Token`: requires valid token, rejects without one
  - Config mode checked at runtime in `HandlePostLogin`
- [ ] **Fallback chain**:
  - If Login Server unreachable → client logs warning → retry with DirectAuth mode
  - If Game Server receives connection without token → reject or fall back depending on config
  - Document fallback behavior for production demo
- [ ] **Security note**: Token replay prevention — token is single-use or tied to client IP
  - Document as future enhancement (not implemented in stub)

### Wave 5 — Cleanup, Documentation, Hardening (Day 4, ~0.5d)
**Remove stale auth code from GameMode, update architecture docs, and write the sprint postmortem.**

- [ ] Remove stale Steam auth references from `AOnsetGameModeBase` (comments, includes, deprecated RPCs)
- [ ] Update `AOnsetPlayerController` — remove auth-specific RPCs handled by subsystem (if any are duplicated)
- [ ] Update `DefaultEngine.ini` — add `[Onset.Auth]` section with default values:
  ```ini
  [/Script/Onset.OnsetAuthSubsystem]
  AuthMode=Direct
  AuthTokenSecret=ChangeMeInProduction
  AuthTokenLifetimeSeconds=300
  ```
- [ ] Create `TODO/DONE/07-??-26.md` — sprint completion record (dated at completion)
- [ ] Update `TODO/Private_Demo_Checklist.md` — mark A5c all done
- [ ] Add `LoginServerIP`, `LoginServerPort` config to client-side config section
- [ ] **Architecture diagram** — add note to sprint doc: auth flow diagram (Login Server → Token → Game Server)

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
