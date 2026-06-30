#include "DataStoreFactory.h"
#include "OnsetDataStoreModule.h"

#ifndef ONSETDATASTORE_CLIENT_ONLY
#include "FSQLiteStore.h"
#include "FPgSQLStore.h"
#endif

TUniquePtr<IPlayerDataStore> CreateDataStore(const FString& Type, const FString& ConnectionString, bool& bOutSuccess)
{
	bOutSuccess = false;

#ifndef ONSETDATASTORE_CLIENT_ONLY
	if (Type.Equals(TEXT("SQLite"), ESearchCase::IgnoreCase))
	{
		TUniquePtr<FSQLiteStore> Store = MakeUnique<FSQLiteStore>();
		bOutSuccess = Store->Initialize(ConnectionString);
		if (bOutSuccess)
			return Store;
	}
	else if (Type.Equals(TEXT("Postgres"), ESearchCase::IgnoreCase) || Type.Equals(TEXT("PgSQL"), ESearchCase::IgnoreCase))
	{
		TUniquePtr<FPgSQLStore> Store = MakeUnique<FPgSQLStore>();
		bOutSuccess = Store->Initialize(ConnectionString);
		if (bOutSuccess)
			return Store;
	}
	else
	{
		UE_LOG(LogOnsetDataStore, Error, TEXT("CreateDataStore: unknown store type '%s'"), *Type);
	}
#else
	UE_LOG(LogOnsetDataStore, Warning, TEXT("CreateDataStore: store backends not available on client builds"));
#endif

	return nullptr;
}
