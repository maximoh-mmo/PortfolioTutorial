#pragma once

#include "IMHPersistenceStore.h"

struct FMHHttpRecordConfig
{
    /** Service root without query/fragment. HTTPS, or HTTP on localhost/127.0.0.1 for tests. */
    FString BaseUrl;
    TMap<FString, FString> Headers;
    float TimeoutSeconds = 10.0f;
};

namespace MH::Private { struct FHttpRecordState; }

/** v1 opaque record protocol; serial operations per instance, no automatic retries. */
class MHPERSISTENCEHTTP_API FMHHttpRecordStore final : public IMHPersistenceStore
{
    TSharedRef<MH::Private::FHttpRecordState, ESPMode::ThreadSafe> State;
public:
    explicit FMHHttpRecordStore(FMHHttpRecordConfig Config);
    ~FMHHttpRecordStore() override;
    FMHHttpRecordStore(const FMHHttpRecordStore&) = delete;
    FMHHttpRecordStore& operator=(const FMHHttpRecordStore&) = delete;
    FMHRequestHandle Load(FMHRecordKey Key, TUniqueFunction<void(TMHResult<TArray<uint8>>)>&& Completion) override;
    FMHRequestHandle Put(FMHRecordKey Key, TArray<uint8> Payload, TUniqueFunction<void(TMHResult<FMHUnit>)>&& Completion) override;
    FMHRequestHandle Delete(FMHRecordKey Key, TUniqueFunction<void(TMHResult<FMHUnit>)>&& Completion) override;
    void Shutdown() override;
    /** Game-thread query for queued/active operations; cancellation is not server-side rollback. */
    bool HasPendingIO() const;
};
