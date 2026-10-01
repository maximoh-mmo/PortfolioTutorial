#pragma once

#include "IMHPersistenceStore.h"

namespace MH::Private { struct FPostgresStoreState; }

struct FMHPostgresConfig
{
    /** Explicit libpq connection string. Never logged or sent back in diagnostics. */
    FString ConnectionString;
    int32 ConnectTimeoutSeconds = 5;
    int32 StatementTimeoutMilliseconds = 5000;
};

/** Serial worker I/O with private libpq. Cancellation suppresses results, not committed writes. */
class MHPERSISTENCEPOSTGRES_API FMHPostgresStore final : public IMHPersistenceStore
{
    TSharedRef<MH::Private::FPostgresStoreState, ESPMode::ThreadSafe> State;
public:
    explicit FMHPostgresStore(FMHPostgresConfig Config);
    ~FMHPostgresStore() override;
    FMHPostgresStore(const FMHPostgresStore&) = delete;
    FMHPostgresStore& operator=(const FMHPostgresStore&) = delete;
    FMHRequestHandle Load(FMHRecordKey Key, TUniqueFunction<void(TMHResult<TArray<uint8>>)>&& Completion) override;
    FMHRequestHandle Put(FMHRecordKey Key, TArray<uint8> Payload, TUniqueFunction<void(TMHResult<FMHUnit>)>&& Completion) override;
    FMHRequestHandle Delete(FMHRecordKey Key, TUniqueFunction<void(TMHResult<FMHUnit>)>&& Completion) override;
    void Shutdown() override;
    /** After Shutdown, wait for false on the game thread before module unload. */
    bool HasPendingIO() const;
};
