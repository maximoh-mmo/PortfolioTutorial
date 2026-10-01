#pragma once

#include "IMHPersistenceStore.h"

namespace MH::Private { struct FSQLiteStoreState; }

/** Serial per-instance worker I/O; no SQLite types or game schemas in the public API. */
class MHPERSISTENCESQLITE_API FMHSQLiteStore final : public IMHPersistenceStore
{
    TSharedRef<MH::Private::FSQLiteStoreState, ESPMode::ThreadSafe> State;

public:
    /** Use a dedicated database file in an existing directory. Empty/:memory: paths are rejected. */
    explicit FMHSQLiteStore(FString DatabaseFilename);
    ~FMHSQLiteStore() override;
    FMHSQLiteStore(const FMHSQLiteStore&) = delete;
    FMHSQLiteStore& operator=(const FMHSQLiteStore&) = delete;
    FMHRequestHandle Load(FMHRecordKey Key, TUniqueFunction<void(TMHResult<TArray<uint8>>)>&& Completion) override;
    FMHRequestHandle Put(FMHRecordKey Key, TArray<uint8> Payload, TUniqueFunction<void(TMHResult<FMHUnit>)>&& Completion) override;
    FMHRequestHandle Delete(FMHRecordKey Key, TUniqueFunction<void(TMHResult<FMHUnit>)>&& Completion) override;
    void Shutdown() override;
    /** Game-thread query: after Shutdown, wait for false before deleting the file or unloading the module. */
    bool HasPendingIO() const;
};
