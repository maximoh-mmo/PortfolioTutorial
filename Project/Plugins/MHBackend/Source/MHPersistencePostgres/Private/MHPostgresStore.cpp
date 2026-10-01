#include "MHPostgresStore.h"
#include "Async/Async.h"
#include "Containers/Queue.h"
#include "Misc/ScopeExit.h"
#if WITH_MH_POSTGRES
#include "libpq-fe.h"
#endif

namespace
{
    bool IsConfigured(const FMHPostgresConfig& Config)
    {
        return WITH_MH_POSTGRES && !Config.ConnectionString.IsEmpty() &&
            Config.ConnectTimeoutSeconds >= 1 && Config.ConnectTimeoutSeconds <= 60 &&
            Config.StatementTimeoutMilliseconds >= 1 && Config.StatementTimeoutMilliseconds <= 60000;
    }

    template <typename T> TMHResult<T> DatabaseError(EMHErrorCode Code = EMHErrorCode::Unavailable)
    {
        return TMHResult<T>::Failure({Code, TEXT("PostgreSQL operation failed")});
    }

#if WITH_MH_POSTGRES
    struct FConnection
    {
        PGconn* Handle = nullptr;
        ~FConnection() { if (Handle) { PQfinish(Handle); } }
    };
    struct FResult
    {
        PGresult* Handle;
        explicit FResult(PGresult* InHandle) : Handle(InHandle) {}
        ~FResult() { if (Handle) { PQclear(Handle); } }
        FResult(const FResult&) = delete;
        FResult& operator=(const FResult&) = delete;
        bool HasStatus(ExecStatusType Status) const { return Handle && PQresultStatus(Handle) == Status; }
        EMHErrorCode Error() const
        {
            const char* Code = Handle ? PQresultErrorField(Handle, PG_DIAG_SQLSTATE) : nullptr;
            if (Code && (FCStringAnsi::Strcmp(Code, "57014") == 0 || FCStringAnsi::Strcmp(Code, "55P03") == 0))
                return EMHErrorCode::Timeout;
            if (Code && FCStringAnsi::Strncmp(Code, "28", 2) == 0) return EMHErrorCode::Unauthorized;
            if (Code && FCStringAnsi::Strcmp(Code, "23505") == 0) return EMHErrorCode::Conflict;
            return EMHErrorCode::Unavailable;
        }
    };

    EMHErrorCode Open(FConnection& Connection, const FMHPostgresConfig& Config, bool& bSuccess)
    {
        const FTCHARToUTF8 ConnInfo(*Config.ConnectionString);
        const FTCHARToUTF8 Timeout(*LexToString(Config.ConnectTimeoutSeconds));
        const FString Options = FString::Printf(TEXT("-c statement_timeout=%d -c lock_timeout=%d -c search_path=pg_catalog"),
            Config.StatementTimeoutMilliseconds, Config.StatementTimeoutMilliseconds);
        const FTCHARToUTF8 OptionsUtf8(*Options);
        const char* Keywords[] = {"dbname", "connect_timeout", "options", "client_encoding", nullptr};
        const char* Values[] = {ConnInfo.Get(), Timeout.Get(), OptionsUtf8.Get(), "UTF8", nullptr};
        // Expand the explicit conninfo, then override timeout/encoding/search-path options.
        Connection.Handle = PQconnectdbParams(Keywords, Values, 1);
        if (!Connection.Handle || PQstatus(Connection.Handle) != CONNECTION_OK) return EMHErrorCode::Unavailable;
        PQsetNoticeProcessor(Connection.Handle, [](void*, const char*) {}, nullptr);
        // Serialize initial schema creation across provider instances. Transaction-scoped lock
        // is released on COMMIT or connection close. No Onset tables or migration state touched.
        FResult Schema(PQexec(Connection.Handle,
            "BEGIN; SELECT pg_catalog.pg_advisory_xact_lock(1296584786,1); "
            "CREATE TABLE IF NOT EXISTS public.mh_records_v1 ("
            "namespace TEXT COLLATE \"C\" NOT NULL, record_key TEXT COLLATE \"C\" NOT NULL, "
            "payload BYTEA NOT NULL, PRIMARY KEY(namespace,record_key)); COMMIT;"));
        bSuccess = Schema.HasStatus(PGRES_COMMAND_OK);
        return Schema.Error();
    }

    PGresult* Execute(PGconn* Connection, const char* Sql, const FMHRecordKey& Key, const TArray<uint8>* Payload = nullptr)
    {
        const FTCHARToUTF8 Namespace(*Key.Namespace), Id(*Key.Id);
        const char Empty = 0;
        const char* Values[] = {Namespace.Get(), Id.Get(), Payload && !Payload->IsEmpty() ?
            reinterpret_cast<const char*>(Payload->GetData()) : &Empty};
        const int Lengths[] = {Namespace.Length(), Id.Length(), Payload ? Payload->Num() : 0};
        const int Formats[] = {0, 0, 1};
        const Oid Types[] = {25, 25, 17}; // text, text, bytea; binary payload must have explicit type.
        return PQexecParams(Connection, Sql, Payload ? 3 : 2, Types, Values, Lengths, Formats, 1);
    }
#endif
}

namespace MH::Private
{
    struct FPostgresStoreState : TSharedFromThis<FPostgresStoreState, ESPMode::ThreadSafe>
    {
        FMHPostgresConfig Config;
        TQueue<TUniqueFunction<void()>> Jobs;
        TMap<FGuid, FMHRequestHandle> Pending;
        bool bBusy = false;
        bool bShutdown = false;

        void StartNext()
        {
            check(IsInGameThread());
            TUniqueFunction<void()> Job;
            if (!bBusy && !bShutdown && Jobs.Dequeue(Job))
            {
                bBusy = true;
                Job();
            }
        }
    };

    template <typename T, typename TWork>
    FMHRequestHandle QueuePostgres(const TSharedRef<FPostgresStoreState, ESPMode::ThreadSafe>& State,
        FMHRecordKey Key, TUniqueFunction<void(TMHResult<T>)>&& Completion, TWork Work)
    {
        check(IsInGameThread());
        TMHRequest<T> Source(MoveTemp(Completion));
        if (State->bShutdown || !IsConfigured(State->Config) || !MHIsValidRecordKey(Key))
        {
            const auto Code = State->bShutdown ? EMHErrorCode::Unavailable :
                (!IsConfigured(State->Config) ? EMHErrorCode::NotConfigured : EMHErrorCode::InvalidArgument);
            Source.Complete(TMHResult<T>::Failure({Code, TEXT("Postgres store or record key is not available")}));
            return Source.GetHandle();
        }
        const FGuid Id = FGuid::NewGuid();
        State->Pending.Add(Id, Source.GetHandle());
        const TWeakPtr<FPostgresStoreState, ESPMode::ThreadSafe> WeakState = State;
        State->Jobs.Enqueue([WeakState, Source, Key = MoveTemp(Key), Id, Work = MoveTemp(Work)]() mutable
        {
            const auto Self = WeakState.Pin();
            if (!Self) { return; }
            Async(EAsyncExecution::ThreadPool, [Self, Source, Key = MoveTemp(Key), Id, Work = MoveTemp(Work)]() mutable
            {
                auto Result = Source.GetHandle().IsPending() ? Work(Self->Config, Key) :
                    TMHResult<T>::Failure({EMHErrorCode::Cancelled, TEXT("Request cancelled before I/O")});
                DispatchCompletion([Self, Source, Id, Result = MoveTemp(Result)]() mutable
                {
                    Self->Pending.Remove(Id);
                    Self->bBusy = false;
                    Source.Complete(MoveTemp(Result));
                    Self->StartNext();
                });
            });
        });
        // Defer launching as well as completion so same-frame cancellation can skip I/O.
        DispatchCompletion([WeakState]() { if (const auto Self = WeakState.Pin()) { Self->StartNext(); } });
        return Source.GetHandle();
    }
}


FMHPostgresStore::FMHPostgresStore(FMHPostgresConfig Config)
    : State(MakeShared<MH::Private::FPostgresStoreState, ESPMode::ThreadSafe>())
{
    check(IsInGameThread());
    State->Config = MoveTemp(Config);
}
FMHPostgresStore::~FMHPostgresStore() { Shutdown(); }

FMHRequestHandle FMHPostgresStore::Load(FMHRecordKey Key, TUniqueFunction<void(TMHResult<TArray<uint8>>)>&& Completion)
{
    return MH::Private::QueuePostgres<TArray<uint8>>(State, MoveTemp(Key), MoveTemp(Completion),
        [](const FMHPostgresConfig& Config, const FMHRecordKey& RecordKey)
        {
#if WITH_MH_POSTGRES
            FConnection Connection;
            bool bOpen = false;
            const auto Error = Open(Connection, Config, bOpen);
            if (!bOpen) return DatabaseError<TArray<uint8>>(Error);
            FResult Result(Execute(Connection.Handle,
                "SELECT payload FROM public.mh_records_v1 WHERE namespace=$1 AND record_key=$2", RecordKey));
            if (!Result.HasStatus(PGRES_TUPLES_OK)) return DatabaseError<TArray<uint8>>(Result.Error());
            if (PQntuples(Result.Handle) == 0)
                return TMHResult<TArray<uint8>>::Failure({EMHErrorCode::NotFound, TEXT("Record not found")});
            if (PQntuples(Result.Handle) != 1 || PQnfields(Result.Handle) != 1 ||
                PQgetisnull(Result.Handle, 0, 0) || PQftype(Result.Handle, 0) != 17 || PQfformat(Result.Handle, 0) != 1)
                return DatabaseError<TArray<uint8>>(EMHErrorCode::Internal);
            TArray<uint8> Payload;
            Payload.Append(reinterpret_cast<const uint8*>(PQgetvalue(Result.Handle, 0, 0)), PQgetlength(Result.Handle, 0, 0));
            return TMHResult<TArray<uint8>>::Success(MoveTemp(Payload));
#else
            return DatabaseError<TArray<uint8>>(EMHErrorCode::NotConfigured);
#endif
        });
}

FMHRequestHandle FMHPostgresStore::Put(FMHRecordKey Key, TArray<uint8> Payload, TUniqueFunction<void(TMHResult<FMHUnit>)>&& Completion)
{
    return MH::Private::QueuePostgres<FMHUnit>(State, MoveTemp(Key), MoveTemp(Completion),
        [Payload = MoveTemp(Payload)](const FMHPostgresConfig& Config, const FMHRecordKey& RecordKey)
        {
#if WITH_MH_POSTGRES
            FConnection Connection;
            bool bOpen = false;
            const auto Error = Open(Connection, Config, bOpen);
            if (!bOpen) return DatabaseError<FMHUnit>(Error);
            FResult Result(Execute(Connection.Handle,
                "INSERT INTO public.mh_records_v1(namespace,record_key,payload) VALUES($1,$2,$3) "
                "ON CONFLICT(namespace,record_key) DO UPDATE SET payload=excluded.payload", RecordKey, &Payload));
            if (!Result.HasStatus(PGRES_COMMAND_OK)) return DatabaseError<FMHUnit>(Result.Error());
            return TMHResult<FMHUnit>::Success({});
#else
            return DatabaseError<FMHUnit>(EMHErrorCode::NotConfigured);
#endif
        });
}

FMHRequestHandle FMHPostgresStore::Delete(FMHRecordKey Key, TUniqueFunction<void(TMHResult<FMHUnit>)>&& Completion)
{
    return MH::Private::QueuePostgres<FMHUnit>(State, MoveTemp(Key), MoveTemp(Completion),
        [](const FMHPostgresConfig& Config, const FMHRecordKey& RecordKey)
        {
#if WITH_MH_POSTGRES
            FConnection Connection;
            bool bOpen = false;
            const auto Error = Open(Connection, Config, bOpen);
            if (!bOpen) return DatabaseError<FMHUnit>(Error);
            FResult Result(Execute(Connection.Handle,
                "DELETE FROM public.mh_records_v1 WHERE namespace=$1 AND record_key=$2", RecordKey));
            if (!Result.HasStatus(PGRES_COMMAND_OK)) return DatabaseError<FMHUnit>(Result.Error());
            return TMHResult<FMHUnit>::Success({});
#else
            return DatabaseError<FMHUnit>(EMHErrorCode::NotConfigured);
#endif
        });
}

void FMHPostgresStore::Shutdown()
{
    check(IsInGameThread());
    if (State->bShutdown) { return; }
    State->bShutdown = true;
    for (const auto& Request : State->Pending) { Request.Value.Cancel(); }
    State->Pending.Empty();
    State->Jobs.Empty();
}

bool FMHPostgresStore::HasPendingIO() const
{
    check(IsInGameThread());
    return State->bBusy || !State->Jobs.IsEmpty();
}
