#include "MHMemoryStore.h"

bool MHIsValidRecordKey(const FMHRecordKey& Key)
{
    auto IsValidPart = [](const FString& Part, int32 MaxLength)
    {
        if (Part.IsEmpty() || Part.Len() > MaxLength) { return false; }
        for (TCHAR C : Part) { if (C < 32 || C == 127) { return false; } }
        return true;
    };
    return IsValidPart(Key.Namespace, 128) && IsValidPart(Key.Id, 512);
}

namespace MH::Private
{
    struct FMemoryStoreState
    {
        TMap<FMHRecordKey, TArray<uint8>> Records;
        TMap<FGuid, FMHRequestHandle> Pending;
        bool bShutdown = false;
    };

    template <typename T, typename TWork>
    FMHRequestHandle Queue(const TSharedRef<FMemoryStoreState, ESPMode::ThreadSafe>& State,
        FMHRecordKey Key, TUniqueFunction<void(TMHResult<T>)>&& Completion, TWork Work)
    {
        check(IsInGameThread());
        TMHRequest<T> Source(MoveTemp(Completion));
        if (State->bShutdown || !MHIsValidRecordKey(Key))
        {
            Source.Complete(TMHResult<T>::Failure({State->bShutdown ? EMHErrorCode::Unavailable : EMHErrorCode::InvalidArgument,
                State->bShutdown ? TEXT("Persistence store is shut down") : TEXT("Invalid record key")}));
            return Source.GetHandle();
        }
        const FGuid Id = FGuid::NewGuid();
        State->Pending.Add(Id, Source.GetHandle());
        DispatchCompletion([State, Source, Id, Key = MoveTemp(Key), Work = MoveTemp(Work)]() mutable
        {
            State->Pending.Remove(Id);
            if (Source.GetHandle().IsPending()) { Source.Complete(Work(*State, Key)); }
        });
        return Source.GetHandle();
    }
}

FMHMemoryStore::FMHMemoryStore() : State(MakeShared<MH::Private::FMemoryStoreState, ESPMode::ThreadSafe>())
{
    check(IsInGameThread());
}
FMHMemoryStore::~FMHMemoryStore() { Shutdown(); }

FMHRequestHandle FMHMemoryStore::Load(FMHRecordKey Key, TUniqueFunction<void(TMHResult<TArray<uint8>>)>&& Completion)
{
    return MH::Private::Queue<TArray<uint8>>(State, MoveTemp(Key), MoveTemp(Completion),
        [](MH::Private::FMemoryStoreState& Data, const FMHRecordKey& RecordKey)
        {
            if (const auto* Payload = Data.Records.Find(RecordKey)) { return TMHResult<TArray<uint8>>::Success(*Payload); }
            return TMHResult<TArray<uint8>>::Failure({EMHErrorCode::NotFound, TEXT("Record not found")});
        });
}

FMHRequestHandle FMHMemoryStore::Put(FMHRecordKey Key, TArray<uint8> Payload, TUniqueFunction<void(TMHResult<FMHUnit>)>&& Completion)
{
    return MH::Private::Queue<FMHUnit>(State, MoveTemp(Key), MoveTemp(Completion),
        [Payload = MoveTemp(Payload)](MH::Private::FMemoryStoreState& Data, const FMHRecordKey& RecordKey) mutable
        {
            Data.Records.Add(RecordKey, MoveTemp(Payload));
            return TMHResult<FMHUnit>::Success({});
        });
}

FMHRequestHandle FMHMemoryStore::Delete(FMHRecordKey Key, TUniqueFunction<void(TMHResult<FMHUnit>)>&& Completion)
{
    return MH::Private::Queue<FMHUnit>(State, MoveTemp(Key), MoveTemp(Completion),
        [](MH::Private::FMemoryStoreState& Data, const FMHRecordKey& RecordKey)
        {
            Data.Records.Remove(RecordKey);
            return TMHResult<FMHUnit>::Success({});
        });
}

void FMHMemoryStore::Shutdown()
{
    check(IsInGameThread());
    if (State->bShutdown) { return; }
    State->bShutdown = true;
    for (const auto& Pending : State->Pending) { Pending.Value.Cancel(); }
    State->Pending.Empty();
}
