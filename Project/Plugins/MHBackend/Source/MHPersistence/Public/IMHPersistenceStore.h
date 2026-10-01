#pragma once

#include "MHRequest.h"

/** Exact, case-sensitive opaque identifiers. Game-specific schema belongs in an adapter. */
struct FMHRecordKey
{
    FString Namespace;
    FString Id;
    bool operator==(const FMHRecordKey& Other) const
    {
        return Namespace.Equals(Other.Namespace, ESearchCase::CaseSensitive) && Id.Equals(Other.Id, ESearchCase::CaseSensitive);
    }
    friend uint32 GetTypeHash(const FMHRecordKey& Key)
    {
        return HashCombine(GetTypeHash(Key.Namespace), GetTypeHash(Key.Id));
    }
};

/** Provider contract: call on the game thread; all completions are deferred onto that thread. */
class IMHPersistenceStore
{
public:
    virtual ~IMHPersistenceStore() = default;
    /** Missing records fail with NotFound; a stored zero-length payload is a success. */
    virtual FMHRequestHandle Load(FMHRecordKey Key,
        TUniqueFunction<void(TMHResult<TArray<uint8>>)>&& Completion) = 0;
    /** Atomic per-record upsert. Opaque bytes; no implicit serialization or schema migration. */
    virtual FMHRequestHandle Put(FMHRecordKey Key, TArray<uint8> Payload,
        TUniqueFunction<void(TMHResult<FMHUnit>)>&& Completion) = 0;
    /** Idempotent: deleting an absent record succeeds. */
    virtual FMHRequestHandle Delete(FMHRecordKey Key,
        TUniqueFunction<void(TMHResult<FMHUnit>)>&& Completion) = 0;
    /** Cancel pending work and reject new operations; call before engine ticker shutdown. */
    virtual void Shutdown() = 0;
};

/** Shared input rules for providers. Does not constrain game schema or payload bytes. */
MHPERSISTENCE_API bool MHIsValidRecordKey(const FMHRecordKey& Key);
