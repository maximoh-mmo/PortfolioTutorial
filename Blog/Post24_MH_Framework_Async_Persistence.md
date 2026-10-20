# 🎬 Post 24 — The MH Framework: Modular Async Architecture and Multi-Backend Persistence

*14 commits · Sep 30 – Oct 2, 2026 · covering MHCore, MHPersistence, Cancellable Async Requests, Result Ownership, Multi-Backend Storage (SQLite, PostgreSQL, HTTP), Live Docker Test Fixtures*

---

## Introduction

As a multiplayer game matures, architectural debt accumulates in the foundation. In the early days of Onset, database operations, HTTP requests, and engine glue were poured directly into the primary `Onset` game module. If the game needed to save a character, `AOnsetPlayerController` made direct calls into SQLite or PostgreSQL stores.

This created tight coupling:
* The game module couldn't build without database third-party libraries (`libpq`, SQLite).
* Running integration tests required spinning up the entire game engine and loading game maps.
* Switching from a local SQLite database to a dedicated server's PostgreSQL or a web microservice required touching gameplay code.
* In-flight async queries had no clean cancellation mechanism, risking crashes when players disconnected before a database read completed.

Between September 30 and October 2, I executed a major architectural refactor: I scaffolded the **MH Framework**—a family of independent, modular Unreal Engine plugins designed for clean separation of concerns, cancellable async operations, and swappable persistence backends.

---

## Technical Exposition

### Plugin Taxonomy and Dependency Hygiene

The core rule of the MH Framework is **strict unidirectional dependency flow**:

```
+-------------------------------------------------------------+
|                        Onset Game                           |
+-------------------------------------------------------------+
          |                                       |
          v                                       v
+-----------------------+               +---------------------+
|      MHPersistence    |               |    MHOnlineCore     |
+-----------------------+               +---------------------+
     |        |        |                           |
     v        v        v                           v
 [SQLite] [Postgres] [HTTP]                [MHOnlineSteam]
     |        |        |                           |
     +--------+--------+                           |
              |                                    |
              +-----------------+------------------+
                                |
                                v
                    +-----------------------+
                    |        MHCore         |
                    | (Results, Cancel, IO) |
                    +-----------------------+
```

1. **`MHCore`:** Has zero dependencies beyond Unreal's foundational `Core`. It defines move-only results (`FMHResult<T>`), cancellable async contracts, and session token primitives.
2. **`MHPersistence`:** Defines the abstract `IMHPersistenceStore` interface without knowing how data is physically stored.
3. **Provider Modules:** `MHPersistencePostgres`, `MHPersistenceSqlite`, and `MHPersistenceHttp` implement the storage contract independently. Gameplay code never references `libpq` or SQL headers; it references only `IMHPersistenceStore`.

### The Cancellable Async Pattern & Move Ownership

In multiplayer games, database queries take tens of milliseconds. In that time window, a player might disconnect, their Pawn might be destroyed, or the level might unload. Invoking a callback on a garbage-collected object is a guaranteed crash.

To solve this, `MHCore` introduces `FMHCancellationToken` and move-enabled `FMHResult<T>`:

```
[ Game Thread: Actor ] ---- Issue Request ----> [ Background Worker Thread ]
          |                                                |
     (Actor Dies)                                     (Query Done)
          |                                                |
    Cancel Token                                     Check Token
          |                                                |
          x <------------------ Discard Payload <----------+
```

When an actor initiates an async store operation, it passes an `FMHCancellationToken`. If the actor begins destruction, it cancels the token. When the background thread finishes its SQL query, it checks `Token.IsCancelled()`. If cancelled, the result payload is dropped safely without ever attempting to invoke the dead actor's delegate.

Furthermore, `FMHResult<T>` supports C++ move semantics (`TakePayload()`), allowing large binary blobs or parsed JSON trees to be moved into place with zero heap copying.

### Live PostgreSQL Testing Inside Unreal Engine

Testing database code with mocks is fine for basic logic, but it misses syntax errors, connection pool exhaustion, and real database constraints.

I set up a **live Docker test fixture**:
* A local Docker Compose container runs PostgreSQL 16 on an isolated test port.
* Unreal automation specs (`MHPostgresTests.cpp`) connect directly to the container, create a dynamic test schema, execute concurrent CRUD queries, and tear down the schema.
* Before running tests on Windows, a host `libpq` smoke test validates socket connectivity and library loading.

---

## Implementation Details

### 1. The Generic Persistence Interface (`IMHPersistenceStore.h`)

All storage engines adhere to an asynchronous, key-value document contract:

```cpp
class MHPERSISTENCE_API IMHPersistenceStore
{
public:
    virtual ~IMHPersistenceStore() = default;

    virtual void InitializeAsync(
        const FMHStoreConfig& Config,
        FMHStoreInitDelegate OnComplete,
        FMHCancellationToken Token = FMHCancellationToken::None
    ) = 0;

    virtual void SaveRecordAsync(
        const FString& PartitionKey,
        const FString& DocumentKey,
        const FString& JsonPayload,
        FMHStoreOperationDelegate OnComplete,
        FMHCancellationToken Token = FMHCancellationToken::None
    ) = 0;

    virtual void LoadRecordAsync(
        const FString& PartitionKey,
        const FString& DocumentKey,
        FMHStoreLoadDelegate OnComplete,
        FMHCancellationToken Token = FMHCancellationToken::None
    ) = 0;
};
```

### 2. Move Semantics in `FMHResult<T>` (`MHCore/Public/MHResult.h`)

The result container enforces safe, single-ownership extraction:

```cpp
template <typename TPayload>
class FMHResult
{
public:
    static FMHResult Success(TPayload&& InPayload)
    {
        return FMHResult(true, MoveTemp(InPayload), TEXT(""));
    }

    static FMHResult Failure(const FString& InError)
    {
        return FMHResult(false, TPayload(), InError);
    }

    bool IsSuccess() const { return bSuccess; }
    const FString& GetError() const { return ErrorMessage; }

    // Move-only payload extraction prevents redundant cloning of large payloads
    TPayload TakePayload()
    {
        checkf(bSuccess, TEXT("Cannot take payload from a failed result!"));
        return MoveTemp(Payload);
    }

private:
    bool bSuccess;
    TPayload Payload;
    FString ErrorMessage;
};
```

### 3. Background Execution in `FMHPostgresStore.cpp`

PostgreSQL queries are offloaded to Unreal's TaskGraph to ensure the Game Thread never hitches:

```cpp
void FMHPostgresStore::SaveRecordAsync(
    const FString& PartitionKey,
    const FString& DocumentKey,
    const FString& JsonPayload,
    FMHStoreOperationDelegate OnComplete,
    FMHCancellationToken Token)
{
    // Capture state for background execution
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [this, PartitionKey, DocumentKey, JsonPayload, OnComplete, Token]()
        {
            if (Token.IsCancelled()) return;

            // Execute SQL upsert via libpq
            const char* Query = 
                "INSERT INTO mh_records (partition_key, document_key, data, updated_at) "
                "VALUES ($1, $2, $3, NOW()) "
                "ON CONFLICT (partition_key, document_key) DO UPDATE SET data = $3, updated_at = NOW();";

            const char* Values[3] = { TCHAR_TO_UTF8(*PartitionKey), TCHAR_TO_UTF8(*DocumentKey), TCHAR_TO_UTF8(*JsonPayload) };
            PGresult* Res = PQexecParams(Connection, Query, 3, nullptr, Values, nullptr, nullptr, 0);

            const bool bSuccess = (PQresultStatus(Res) == PGRES_COMMAND_OK);
            PQclear(Res);

            // Return to Game Thread for delegate invocation
            AsyncTask(ENamedThreads::GameThread, [OnComplete, Token, bSuccess]()
            {
                if (!Token.IsCancelled())
                {
                    OnComplete.ExecuteIfBound(bSuccess ? FMHResult<void>::Success() : FMHResult<void>::Failure(TEXT("Upsert Failed")));
                }
            });
        },
        TStatId(),
        nullptr,
        ENamedThreads::AnyBackgroundThreadNormalTask
    );
}
```

---

## Results & Validation

1. **Automation Test Suite Passing:**
   Ran `Automation RunTests MH.` inside the Unreal Editor console:
   ```
   [PASSED] MHCore.Result.LifecycleAndOwnership
   [PASSED] MHCore.Async.CancellationHandles
   [PASSED] MHPersistence.MemoryStore.BasicCRUD
   [PASSED] MHPersistence.SQLiteStore.SchemaMigrationAndReadWrite
   [PASSED] MHPersistence.PostgresStore.LiveDockerCRUD
   [PASSED] MHPersistence.HttpRecordStore.LoopbackMock
   ```
   All 14 tests completed with 0 errors.
2. **Dedicated Server Cook Verification:**
   Built a full cooked dedicated server (`UE 5.8.3 Dedicated Server`). The server compiled and linked cleanly with no missing symbol errors or runtime DLL search path failures.
3. **Loopback HTTP Record Persistence:**
   Created a standalone Python test server (`server.py`) and verified that `MHPersistenceHttp` handles authentication headers, chunked transfer encoding, and proper JSON payload persistence over HTTP/1.1.

---

## Challenges & Solutions

### Challenge 1: Windows `libpq.lib` Import Library Mismatches

**Problem:** Building `MHPersistencePostgres` on Windows failed with unresolved external symbol errors (`__imp_PQexecParams`, `__imp_PQconnectdb`). The standard PostgreSQL installer for Windows includes binaries compiled with different MSVC runtime flags than those used by Unreal Engine 5.8.

**Solution:** Compiled an import library stub targeting the exact MSVC CRT profile used by Unreal Build Tool (UBT), placed it in the plugin's `ThirdParty/PostgreSQL/lib/Win64` directory, and configured `MHPersistencePostgres.Build.cs` to delay-load `libpq.dll`.

### Challenge 2: HTTP Header Order Sensitivity on Reverse Proxies

**Problem:** The HTTP persistence store worked locally, but failed when routed through certain reverse proxies because the Authorization header was appended after custom record headers.

**Solution:** Standardized header construction in `FMHHttpRecordStore` to inject standard headers (`Content-Type`, `Accept`, `Authorization`) first before adding custom metadata headers.

---

## Reflection & Lessons Learned

Separating infrastructure into dedicated plugins is one of the highest-leverage investments you can make in a long-term codebase. The moment `MHPersistence` stopped knowing what an `AOnsetPlayerCharacter` was, the entire database layer became 10x more testable, reusable, and robust.

Cancellable async operations should be mandatory for any game that touches the network. Threading bugs are the hardest to debug; making safety the default path at the API boundary pays off every single day.

---

## Forward-Looking Content

| Phase | Status |
|---|---|
| Quests, Click-to-Move & Autoplay Polish | ✅ Committed (Post 23) |
| **The MH Framework Architecture** | ✅ **Committed** |
| Steam Packet Authentication & Dedicated Server Hardening | ⏳ Next Up (Post 25) |

With modular persistence and async architecture solid, the final frontier is authentication security: integrating Steam native AuthTickets, securing dedicated server packet communication, diagnosing callback lifecycles, and hardening the game against malformed login attempts.

---

*Arc commits: `30445cc`, `a33aebb`, `144060b`, `53d9d49`. Related docs: `Docs/Framework/Architecture.md`, `Docs/Framework/Persistence.md`, `Docs/Framework/PostgreSQL.md`.*
