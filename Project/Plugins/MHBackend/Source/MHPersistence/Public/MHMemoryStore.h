#pragma once

#include "IMHPersistenceStore.h"

namespace MH::Private { struct FMemoryStoreState; }

/** Non-durable reference provider for contract tests and offline prototypes. */
class MHPERSISTENCE_API FMHMemoryStore final : public IMHPersistenceStore
{
    TSharedRef<MH::Private::FMemoryStoreState, ESPMode::ThreadSafe> State;

public:
    FMHMemoryStore();
    ~FMHMemoryStore() override;
    FMHMemoryStore(const FMHMemoryStore&) = delete;
    FMHMemoryStore& operator=(const FMHMemoryStore&) = delete;
    FMHRequestHandle Load(FMHRecordKey Key, TUniqueFunction<void(TMHResult<TArray<uint8>>)>&& Completion) override;
    FMHRequestHandle Put(FMHRecordKey Key, TArray<uint8> Payload, TUniqueFunction<void(TMHResult<FMHUnit>)>&& Completion) override;
    FMHRequestHandle Delete(FMHRecordKey Key, TUniqueFunction<void(TMHResult<FMHUnit>)>&& Completion) override;
    void Shutdown() override;
};
