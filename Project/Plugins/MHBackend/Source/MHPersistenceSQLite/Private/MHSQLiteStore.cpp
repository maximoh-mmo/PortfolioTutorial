#include "MHSQLiteStore.h"

#include "Async/Async.h"
#include "Containers/Queue.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "SQLiteDatabase.h"
#include "SQLitePreparedStatement.h"

namespace
{
    bool Open(FSQLiteDatabase& Database, const FString& Filename)
    {
        return IFileManager::Get().DirectoryExists(*FPaths::GetPath(Filename)) &&
            Database.Open(*Filename) && Database.Execute(TEXT("PRAGMA busy_timeout=2000;")) &&
            Database.Execute(TEXT("CREATE TABLE IF NOT EXISTS mh_records_v1 ("
                "namespace TEXT COLLATE BINARY NOT NULL, record_key TEXT COLLATE BINARY NOT NULL, "
                "payload BLOB NOT NULL, PRIMARY KEY(namespace, record_key)) WITHOUT ROWID;"));
    }

    bool BindKey(FSQLitePreparedStatement& Statement, const FMHRecordKey& Key)
    {
        return Statement.IsValid() && Statement.SetBindingValueByIndex(1, Key.Namespace) &&
            Statement.SetBindingValueByIndex(2, Key.Id);
    }

    template <typename T> TMHResult<T> DatabaseError()
    {
        // Database error strings can contain data/paths. Do not forward them to generic callers.
        return TMHResult<T>::Failure({EMHErrorCode::Unavailable, TEXT("SQLite operation failed")});
    }
}

namespace MH::Private
{
    struct FSQLiteStoreState : TSharedFromThis<FSQLiteStoreState, ESPMode::ThreadSafe>
    {
        FString Filename;
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
    FMHRequestHandle QueueSQLite(const TSharedRef<FSQLiteStoreState, ESPMode::ThreadSafe>& State,
        FMHRecordKey Key, TUniqueFunction<void(TMHResult<T>)>&& Completion, TWork Work)
    {
        check(IsInGameThread());
        TMHRequest<T> Source(MoveTemp(Completion));
        if (State->bShutdown || State->Filename.IsEmpty() || !MHIsValidRecordKey(Key))
        {
            const auto Code = State->bShutdown ? EMHErrorCode::Unavailable :
                (State->Filename.IsEmpty() ? EMHErrorCode::NotConfigured : EMHErrorCode::InvalidArgument);
            Source.Complete(TMHResult<T>::Failure({Code, TEXT("SQLite store or record key is not available")}));
            return Source.GetHandle();
        }
        const FGuid Id = FGuid::NewGuid();
        State->Pending.Add(Id, Source.GetHandle());
        const TWeakPtr<FSQLiteStoreState, ESPMode::ThreadSafe> WeakState = State;
        State->Jobs.Enqueue([WeakState, Source, Key = MoveTemp(Key), Id, Work = MoveTemp(Work)]() mutable
        {
            const auto Self = WeakState.Pin();
            if (!Self) { return; }
            Async(EAsyncExecution::ThreadPool, [Self, Source, Key = MoveTemp(Key), Id, Work = MoveTemp(Work)]() mutable
            {
                auto Result = Source.GetHandle().IsPending() ? Work(Self->Filename, Key) :
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

FMHSQLiteStore::FMHSQLiteStore(FString DatabaseFilename)
    : State(MakeShared<MH::Private::FSQLiteStoreState, ESPMode::ThreadSafe>())
{
    check(IsInGameThread());
    if (!DatabaseFilename.IsEmpty() && DatabaseFilename != TEXT(":memory:"))
    {
        State->Filename = FPaths::ConvertRelativePathToFull(DatabaseFilename);
    }
}
FMHSQLiteStore::~FMHSQLiteStore() { Shutdown(); }

FMHRequestHandle FMHSQLiteStore::Load(FMHRecordKey Key, TUniqueFunction<void(TMHResult<TArray<uint8>>)>&& Completion)
{
    return MH::Private::QueueSQLite<TArray<uint8>>(State, MoveTemp(Key), MoveTemp(Completion),
        [](const FString& Filename, const FMHRecordKey& RecordKey)
        {
            FSQLiteDatabase Database;
            ON_SCOPE_EXIT { if (Database.IsValid()) { Database.Close(); } };
            if (!Open(Database, Filename)) { return DatabaseError<TArray<uint8>>(); }
            auto Statement = Database.PrepareStatement(TEXT("SELECT payload FROM mh_records_v1 WHERE namespace=?1 AND record_key=?2;"));
            if (!BindKey(Statement, RecordKey)) { return DatabaseError<TArray<uint8>>(); }
            const auto Step = Statement.Step();
            if (Step == ESQLitePreparedStatementStepResult::Done)
            {
                return TMHResult<TArray<uint8>>::Failure({EMHErrorCode::NotFound, TEXT("Record not found")});
            }
            TArray<uint8> Payload;
            if (Step != ESQLitePreparedStatementStepResult::Row || !Statement.GetColumnValueByIndex(0, Payload))
            {
                return DatabaseError<TArray<uint8>>();
            }
            return TMHResult<TArray<uint8>>::Success(MoveTemp(Payload));
        });
}

FMHRequestHandle FMHSQLiteStore::Put(FMHRecordKey Key, TArray<uint8> Payload, TUniqueFunction<void(TMHResult<FMHUnit>)>&& Completion)
{
    return MH::Private::QueueSQLite<FMHUnit>(State, MoveTemp(Key), MoveTemp(Completion),
        [Payload = MoveTemp(Payload)](const FString& Filename, const FMHRecordKey& RecordKey)
        {
            FSQLiteDatabase Database;
            ON_SCOPE_EXIT { if (Database.IsValid()) { Database.Close(); } };
            if (!Open(Database, Filename)) { return DatabaseError<FMHUnit>(); }
            auto Statement = Database.PrepareStatement(TEXT("INSERT INTO mh_records_v1(namespace,record_key,payload) VALUES(?1,?2,?3) "
                "ON CONFLICT(namespace,record_key) DO UPDATE SET payload=excluded.payload;"));
            const uint8 EmptyBlob = 0;
            const void* Bytes = Payload.IsEmpty() ? &EmptyBlob : Payload.GetData();
            if (!BindKey(Statement, RecordKey) || !Statement.SetBindingValueByIndex(3, Bytes, Payload.Num()) ||
                Statement.Step() != ESQLitePreparedStatementStepResult::Done) { return DatabaseError<FMHUnit>(); }
            return TMHResult<FMHUnit>::Success({});
        });
}

FMHRequestHandle FMHSQLiteStore::Delete(FMHRecordKey Key, TUniqueFunction<void(TMHResult<FMHUnit>)>&& Completion)
{
    return MH::Private::QueueSQLite<FMHUnit>(State, MoveTemp(Key), MoveTemp(Completion),
        [](const FString& Filename, const FMHRecordKey& RecordKey)
        {
            FSQLiteDatabase Database;
            ON_SCOPE_EXIT { if (Database.IsValid()) { Database.Close(); } };
            if (!Open(Database, Filename)) { return DatabaseError<FMHUnit>(); }
            auto Statement = Database.PrepareStatement(TEXT("DELETE FROM mh_records_v1 WHERE namespace=?1 AND record_key=?2;"));
            if (!BindKey(Statement, RecordKey) || Statement.Step() != ESQLitePreparedStatementStepResult::Done)
            {
                return DatabaseError<FMHUnit>();
            }
            return TMHResult<FMHUnit>::Success({});
        });
}

void FMHSQLiteStore::Shutdown()
{
    check(IsInGameThread());
    if (State->bShutdown) { return; }
    State->bShutdown = true;
    for (const auto& Request : State->Pending) { Request.Value.Cancel(); }
    State->Pending.Empty();
    State->Jobs.Empty();
}

bool FMHSQLiteStore::HasPendingIO() const
{
    check(IsInGameThread());
    return State->bBusy || !State->Jobs.IsEmpty();
}
