#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/OnsetPlayerDataTypes.h"
#include "OnsetPlayerDataSubsystem.generated.h"

struct IPlayerDataStore;

DECLARE_LOG_CATEGORY_EXTERN(LogPlayerData, Log, All);

UCLASS()
class ONSET_API UOnsetPlayerDataSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	UFUNCTION(BlueprintCallable, Category = "Player Data")
	bool LoadAccount(const FString& Platform, const FString& PlatformID, FOnsetAccountData& OutAccount);

	UFUNCTION(BlueprintCallable, Category = "Player Data")
	bool CreateAccount(const FString& Platform, const FString& PlatformID);

	UFUNCTION(BlueprintCallable, Category = "Player Data")
	bool LoadCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex, FOnsetFullCharacterData& OutData);

	UFUNCTION(BlueprintCallable, Category = "Player Data")
	bool SaveCharacter(const FString& Platform, const FString& PlatformID, const FOnsetFullCharacterData& Data);

	UFUNCTION(BlueprintCallable, Category = "Player Data")
	bool DeleteCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex);

	void SaveAll();

	IPlayerDataStore* GetStore() const { return Store.Get(); }

private:
	TUniquePtr<IPlayerDataStore> Store;
};
