#include "FPgSQLStore.h"
#include "OnsetDataStoreModule.h"

#ifndef ONSETDATASTORE_CLIENT_ONLY

bool FPgSQLStore::Initialize(const FString& ConnectionString)
{
	UE_LOG(LogOnsetDataStore, Log, TEXT("FPgSQLStore: stub — PostgreSQL not yet implemented; connection string: %s"), *ConnectionString);
	return false;
}

bool FPgSQLStore::LoadAccount(const FString& Platform, const FString& PlatformID, FOnsetAccountData& OutAccount)
{
	UE_LOG(LogOnsetDataStore, Warning, TEXT("FPgSQLStore: LoadAccount not implemented"));
	return false;
}

bool FPgSQLStore::CreateAccount(const FString& Platform, const FString& PlatformID)
{
	UE_LOG(LogOnsetDataStore, Warning, TEXT("FPgSQLStore: CreateAccount not implemented"));
	return false;
}

bool FPgSQLStore::LoadCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex, FOnsetFullCharacterData& OutData)
{
	UE_LOG(LogOnsetDataStore, Warning, TEXT("FPgSQLStore: LoadCharacter not implemented"));
	return false;
}

bool FPgSQLStore::SaveCharacter(const FString& Platform, const FString& PlatformID, const FOnsetFullCharacterData& Data)
{
	UE_LOG(LogOnsetDataStore, Warning, TEXT("FPgSQLStore: SaveCharacter not implemented"));
	return false;
}

bool FPgSQLStore::DeleteCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex)
{
	UE_LOG(LogOnsetDataStore, Warning, TEXT("FPgSQLStore: DeleteCharacter not implemented"));
	return false;
}

void FPgSQLStore::SaveAll()
{
	UE_LOG(LogOnsetDataStore, Log, TEXT("FPgSQLStore: SaveAll stub — no-op"));
}

#endif
