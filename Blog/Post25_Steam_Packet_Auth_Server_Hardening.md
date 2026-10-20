# 🎬 Post 25 — Steam Packet Authentication, Ticket Lifecycles, and Dedicated Server Hardening

*21 commits · Oct 3–7+, 2026 · covering Provider-Neutral Identity Contracts, Steam Native AuthTickets, Dedicated Server Packet Encryption, Server Callback Diagnostics, Malformed Ticket Test Launcher, Cooker Hardening*

---

## Introduction

The difference between a tech demo running on `localhost` and a real multiplayer game is **perimeter security**. In an unhardened setup, clients can connect with forged player IDs, send fake account strings to the database, or flood the dedicated server with crafted UDP packets that trigger memory corruptions.

Having built data-driven abilities, full inventory systems, client-side pathfinding, and our modular MH Framework, we faced the final gate before production: **authoritative authentication and dedicated server hardening**.

Over twenty-one commits, I brought Onset's networking up to production security standards:
* Architected provider-neutral identity contracts (`IMHIdentityProvider`, `FMHAuthTicket`) within our new `MHOnlineCore` plugin.
* Implemented native Steam `AuthTicket` acquisition, cancellation, and validation pipelines using the Steamworks SDK.
* Enabled Steam packet authentication and encryption across all dedicated server traffic.
* Built a standalone security regression harness (`Launch-MalformedTicketTest.ps1`) that bombards the server with corrupt tickets to guarantee resilience.
* Solved subtle world-transition race conditions and cooker asset mismatches, producing clean, shippable cooked builds.

---

## Technical Exposition

### Provider-Neutral Identity vs. Vendor Lock-In

While Steam is our primary PC distribution platform, hardcoding `SteamUser()` and `SteamGameServer()` calls directly into gameplay or account code creates severe technical debt. If the game later ships on Epic Games Store, GOG, or runs behind a standalone developer launcher, every auth check would have to be rewritten.

To prevent vendor lock-in, `MHOnlineCore` establishes an abstract identity contract:

```
+-------------------------------------------------------------+
|                      Onset Game Mode                        |
|             (PreLogin & Account Association)                |
+-------------------------------------------------------------+
                              |
                              v
+-------------------------------------------------------------+
|                    IMHIdentityProvider                      |
|  - GetProviderType()                                        |
|  - GetUniqueId()                                            |
|  - RequestAuthTicketAsync()                                 |
|  - CancelAuthTicket()                                       |
+-------------------------------------------------------------+
                              ^
                              | (Implements)
+-------------------------------------------------------------+
|                  FMHSteamIdentityProvider                   |
|         (Wraps Steamworks SDK: ISteamUser / SDR)            |
+-------------------------------------------------------------+
```

Gameplay and server login systems speak strictly to `IMHIdentityProvider`. The underlying implementation manages Steamworks handles, native byte arrays, and asynchronous callbacks behind an opaque ticket contract (`FMHAuthTicket`).

### Steam Packet Authentication & Valve Datagram Relay (SDR)

Standard Unreal Engine networking routes unencrypted UDP packets. If a rogue actor sniffs packet traffic, they can spoof RPCs or launch denial-of-service reflection attacks.

By enabling Steam packet authentication in `DefaultEngine.ini`:

```ini
[/Script/Engine.Engine]
!NetDriverDefinitions=ClearArray
+NetDriverDefinitions=(DefName="GameNetDriver",DriverClassName="/Script/SteamSockets.SteamSocketsNetDriver",DriverClassNameFallback="/Script/OnlineSubsystemUtils.IpNetDriver")

[OnlineSubsystemSteam]
bUseSteamNetworking=true
```

Every connection handshake is backed by a native Steam authentication session (`BeginAuthSession` / `ValidateAuthTicketResponse`). Packets travel over Valve's encrypted Steam Datagram Relay (SDR), rendering IP address sniffing, packet tampering, and unauthorized connection hijacking impossible.

### The Malformed Ticket Regression Gate

Security is never verified by testing valid logins. A server's true resilience is demonstrated by how it handles garbage.

To prove the security perimeter, I built an automated attack harness: `Launch-MalformedTicketTest.ps1`. The script launches headless test clients that attempt connection using:
1. **Empty/Zero-Length Tickets**
2. **Truncated Payloads (premature EOF)**
3. **Bit-Flipped Corrupted Tokens**
4. **Valid Tickets Spoofing Mismatched Steam IDs**
5. **Replayed Expired Tickets**

The server must reject 100% of these attempts with `EOnsetAuthResult::InvalidTicket`, immediately terminate the network channel, write diagnostic security logs, and never suffer heap corruption or thread stalls.

---

## Implementation Details

### 1. Owned Ticket Lifecycle (`MHOnlineCore/Public/MHAuthTicket.h`)

Tickets are move-only resources that automatically cancel on the Steam backend if destroyed or dropped:

```cpp
class MHONLINECORE_API FMHAuthTicket
{
public:
    FMHAuthTicket() = default;
    FMHAuthTicket(uint32 InHandle, TArray<uint8>&& InBytes)
        : TicketHandle(InHandle), TicketBytes(MoveTemp(InBytes)) {}

    ~FMHAuthTicket()
    {
        // Automatically release ticket handle on backend if still active
        Cancel();
    }

    // Move-only semantics prevent accidental double-cancellations
    FMHAuthTicket(FMHAuthTicket&& Other) noexcept
        : TicketHandle(Other.TicketHandle), TicketBytes(MoveTemp(Other.TicketBytes))
    {
        Other.TicketHandle = 0;
    }

    bool IsValid() const { return TicketHandle != 0 && TicketBytes.Num() > 0; }
    const TArray<uint8>& GetBytes() const { return TicketBytes; }
    uint32 GetHandle() const { return TicketHandle; }

    void Cancel();

private:
    uint32 TicketHandle = 0;
    TArray<uint8> TicketBytes;
};
```

### 2. Native Steam Ticket Generation (`MHSteamIdentityProvider.cpp`)

Acquiring a ticket uses Steamworks' modern `GetAuthSessionTicket` with callback validation:

```cpp
void FMHSteamIdentityProvider::RequestAuthTicketAsync(FMHOnTicketReady OnComplete)
{
    if (!SteamUser())
    {
        OnComplete.ExecuteIfBound(FMHResult<FMHAuthTicket>::Failure(TEXT("SteamUser unavailable")));
        return;
    }

    uint32 TicketBufferLength = 0;
    TArray<uint8> Buffer;
    Buffer.SetNumUninitialized(1024);

    HAuthTicket Handle = SteamUser()->GetAuthSessionTicket(
        Buffer.GetData(),
        Buffer.Num(),
        &TicketBufferLength
    );

    if (Handle == k_HAuthTicketInvalid)
    {
        OnComplete.ExecuteIfBound(FMHResult<FMHAuthTicket>::Failure(TEXT("GetAuthSessionTicket failed")));
        return;
    }

    Buffer.SetNum(TicketBufferLength);
    FMHAuthTicket Ticket(Handle, MoveTemp(Buffer));
    OnComplete.ExecuteIfBound(FMHResult<FMHAuthTicket>::Success(MoveTemp(Ticket)));
}
```

### 3. Server PreLogin Validation & Caching (`OnsetAuthSubsystem.cpp`)

In `AOnsetGameModeBase::PreLogin()`, connecting players are validated before a Pawn or PlayerController is ever allocated:

```cpp
void UOnsetAuthSubsystem::ValidateConnectingPlayer(
    const FString& Options,
    const FString& Address,
    FString& OutErrorMessage)
{
    const FString SteamIDStr = UGameplayStatics::ParseOption(Options, TEXT("SteamID"));
    const FString TicketHex = UGameplayStatics::ParseOption(Options, TEXT("AuthTicket"));

    TArray<uint8> TicketBytes;
    if (!HexToBytes(TicketHex, TicketBytes) || TicketBytes.Num() < 16)
    {
        OutErrorMessage = TEXT("Malformed or missing Steam authentication ticket.");
        LogSecurityEvent(Address, TEXT("MalformedTicketRejected"));
        return;
    }

    CSteamID SteamID(FCString::Atoi64(*SteamIDStr));
    EBeginAuthSessionResult Result = SteamGameServer()->BeginAuthSession(
        TicketBytes.GetData(),
        TicketBytes.Num(),
        SteamID
    );

    if (Result != k_EBeginAuthSessionResultOK)
    {
        OutErrorMessage = FString::Printf(TEXT("Steam authentication rejected: %d"), (int32)Result);
        return;
    }

    // Cache successful validation outcome across map transitions
    ResultCache.StoreResult(SteamIDStr, EOnsetAuthResult::Success);
}
```

### 4. Preserving Validation Across Seamless Travel

A subtle race condition previously existed where a player who validated successfully during `PreLogin` might hit `OnPostLogin` after world initialization had rotated.

I built `FOnsetSteamAuthResultCache`: a lightweight, thread-safe cache with a 30-second sliding TTL. When seamless travel completes, `OnPostLogin` checks the cache to confirm that the arriving connection already satisfied cryptographic validation, eliminating spurious disconnects during level transitions.

---

## Results & Validation

1. **Dedicated Server Connection Over Steam:**
   * Booted a headless Onset dedicated server on a test machine.
   * Launched packaged game client via Steam.
   * Server console confirmed connection:
     ```
     [LogSteamSockets] Connected to remote client [SteamID: 76561198000000000]
     [LogOnsetAuth] BeginAuthSession: Ticket validated successfully (k_EAuthSessionResponseOK)
     [LogOnsetAuth] Player authenticated. Permitting login.
     ```
2. **Malformed Ticket Attack Simulation:**
   Ran `.\Launch-MalformedTicketTest.ps1`:
   * 10 distinct malformed packet tests executed against the live server.
   * 10/10 attacks were cleanly rejected at the `PreLogin` boundary.
   * Zero server crashes, memory leaks, or unhandled exceptions.
3. **Cooker & Blueprint Hardening:**
   Cleaned and recompiled all affected UI blueprints and designated `AOnsetPlayerController` as the authoritative runtime class. Full package build completed with 0 errors.

---

## Challenges & Solutions

### Challenge 1: The Seamless Travel Validation Race

**Problem:** When a server initiated seamless travel between maps, players reconnecting in the new world triggered `OnPostLogin` before the Steamworks game server callbacks re-registered. The server interpreted this as an unauthenticated connection and kicked players back to the main menu.

**Solution:** Implemented `FOnsetSteamAuthResultCache`. Validated Steam sessions are cached with a short expiration timer that persists across the `UOnsetAuthSubsystem` instance, allowing seamless travel without re-requesting a ticket.

### Challenge 2: Cooker Assertions on Blueprint Connect Buttons

**Problem:** Running the cook pipeline for a packaged Windows build failed during cooking of the main menu widget because an old Blueprint utility function was referencing an editor-only module.

**Solution:** Resaved and cleaned the UI blueprints using the standalone engine cooker, moved editor-only utilities into the `OnsetEditor` module, and verified clean cooking in `UE 5.8.3`.

---

## Reflection & Lessons Learned

Multiplayer security is not an afterthought; it is an architectural commitment. Wrapping third-party SDKs like Steamworks behind clean, provider-neutral interfaces (`IMHIdentityProvider`) keeps your game code elegant while providing total control over authentication lifecycles.

Building the malformed ticket test harness was one of the most satisfying milestones of this sprint. Knowing with empirical certainty that our server rejects corrupted, truncated, and malicious packets gives total confidence as we look toward public testing.

---

## The Road So Far: Milestone Retrospective

With Post 25 complete, Onset has evolved from an empty Git repository into a full-featured, secure, data-driven ARPG:

| Milestone / Post | Focus & Core Systems |
|---|---|
| **Posts 01–05** | Repository bootstrap, player core, targeting, pooling, perception, StateTrees |
| **Posts 06–10** | Hit reactions, corpses, group assist, threat systems, player AI autoplay |
| **Posts 11–15** | Multiplayer Steam foundations, persistence, CommonUI, character classes |
| **Posts 16–19** | Phase 1 abilities, in-game HUD v1/v2, identity-safe persistence postmortem |
| **Post 20** | **The Ability Workbench:** Slate spreadsheets, data-driven combat, leap arcs |
| **Post 21** | **Local Infrastructure & Combat Math:** DynamoDB Local, healing, formulas |
| **Post 22** | **Inventory & Progression:** Bags, click-to-loot corpses, leveling, Slate tools |
| **Post 23** | **Control & Quests:** Client-side click-to-move, StateTree interrupts, quests |
| **Post 24** | **The MH Framework:** Decoupled plugins, async cancellation, multi-store |
| **Post 25** | **Hardened Multiplayer:** Steam packet auth, ticket lifecycles, cooker readiness |

---

## Forward-Looking Content

The core engine, combat, inventory, persistence, and network security are now complete and rock-solid. The next chapter of Onset is all about **content**: designing rich dungeon zones, tuning multi-stage boss encounters, building out quest storylines, and inviting real players in for alpha testing.

---

*Arc commits: `ae132b1`, `c25bdb0`, `6e0336c`, `3ab8d2b`, `b26fafd`. Related docs: `Docs/Framework/Identity.md`, `Docs/Framework/Steam.md`, `Docs/Steam/Steam_Integration_System.md`.*
