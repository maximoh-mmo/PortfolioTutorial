#pragma once

#include "CoreMinimal.h"
#include "IPlayerDataStore.h"

#ifndef ONSETDATASTORE_CLIENT_ONLY

class FSQLiteStore : public IPlayerDataStore
{
public:
	FSQLiteStore() = default;
	virtual ~FSQLiteStore() override;

	FSQLiteStore(const FSQLiteStore&) = delete;
	FSQLiteStore& operator=(const FSQLiteStore&) = delete;

	virtual bool Initialize(const FString& ConnectionString) override;

	virtual bool LoadAccount(const FString& Platform, const FString& PlatformID, FOnsetAccountData& OutAccount) override;
	virtual bool CreateAccount(const FString& Platform, const FString& PlatformID) override;
	virtual bool LoadCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex, FOnsetFullCharacterData& OutData) override;
	virtual bool SaveCharacter(const FString& Platform, const FString& PlatformID, const FOnsetFullCharacterData& Data) override;
	virtual bool DeleteCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex) override;
	virtual void SaveAll() override;

private:
	bool Exec(const char* SQL);
	bool PrepareAndBind(const char* SQL);

	struct sqlite3* DB = nullptr;
	struct sqlite3_stmt* ActiveStmt = nullptr;

	bool bWALEnabled = false;
	FString DBPath;

	bool EnsureSchema();
	int32 GetSchemaVersion();
	void RunMigration(int32 FromVersion);
};

#endif
