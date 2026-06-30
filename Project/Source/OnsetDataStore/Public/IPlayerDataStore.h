#pragma once

#include "CoreMinimal.h"
#include "OnsetPlayerDataTypes.h"

struct IPlayerDataStore
{
	virtual ~IPlayerDataStore() = default;

	virtual bool Initialize(const FString& ConnectionString) = 0;

	virtual bool LoadAccount(const FString& Platform, const FString& PlatformID, FOnsetAccountData& OutAccount) = 0;

	virtual bool CreateAccount(const FString& Platform, const FString& PlatformID) = 0;

	virtual bool LoadCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex, FOnsetFullCharacterData& OutData) = 0;

	virtual bool SaveCharacter(const FString& Platform, const FString& PlatformID, const FOnsetFullCharacterData& Data) = 0;

	virtual bool DeleteCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex) = 0;

	virtual void SaveAll() = 0;
};
