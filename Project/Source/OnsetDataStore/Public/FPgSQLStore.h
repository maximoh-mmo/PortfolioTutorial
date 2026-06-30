#pragma once

#include "CoreMinimal.h"
#include "IPlayerDataStore.h"

#ifndef ONSETDATASTORE_CLIENT_ONLY

class FPgSQLStore : public IPlayerDataStore
{
public:
	FPgSQLStore() = default;
	virtual ~FPgSQLStore() override = default;

	FPgSQLStore(const FPgSQLStore&) = delete;
	FPgSQLStore& operator=(const FPgSQLStore&) = delete;

	virtual bool Initialize(const FString& ConnectionString) override;
	virtual bool LoadAccount(const FString& Platform, const FString& PlatformID, FOnsetAccountData& OutAccount) override;
	virtual bool CreateAccount(const FString& Platform, const FString& PlatformID) override;
	virtual bool LoadCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex, FOnsetFullCharacterData& OutData) override;
	virtual bool SaveCharacter(const FString& Platform, const FString& PlatformID, const FOnsetFullCharacterData& Data) override;
	virtual bool DeleteCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex) override;
	virtual void SaveAll() override;
};

#endif
