#pragma once

#include "CoreMinimal.h"
#include "IPlayerDataStore.h"

#ifndef ONSETDATASTORE_CLIENT_ONLY

class FHttpStore : public IPlayerDataStore
{
public:
	FHttpStore() = default;
	virtual ~FHttpStore() override;

	FHttpStore(const FHttpStore&) = delete;
	FHttpStore& operator=(const FHttpStore&) = delete;

	virtual bool Initialize(const FString& ConnectionString) override;

	virtual bool LoadAccount(const FString& Platform, const FString& PlatformID, FOnsetAccountData& OutAccount) override;
	virtual bool CreateAccount(const FString& Platform, const FString& PlatformID) override;
	virtual bool LoadCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex, FOnsetFullCharacterData& OutData) override;
	virtual bool SaveCharacter(const FString& Platform, const FString& PlatformID, const FOnsetFullCharacterData& Data) override;
	virtual bool DeleteCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex) override;
	virtual void SaveAll() override;

private:
	bool SendRequest(const FString& Verb, const FString& Path, const FString& Body, const FString& StoreToken, FString& OutBody, int32& OutStatusCode);
	FString BuildSignedToken(const FString& Platform, const FString& PlatformID, int32 SlotIndex) const;

	FString BaseURL;
	FString APIKey;
	FString AuthTokenSecret;
	int32 TokenLifetimeSeconds = 300;
};

#endif
