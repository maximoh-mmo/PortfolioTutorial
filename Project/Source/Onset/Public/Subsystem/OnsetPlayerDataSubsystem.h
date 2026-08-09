#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OnsetPlayerDataTypes.h"
#include "IPlayerDataStore.h"
#include "OnsetPlayerDataSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPlayerData, Log, All);

UCLASS()
class ONSET_API UOnsetPlayerDataSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	/** Flush all pending saves to disk / database immediately. */
	void SaveAll();

	/** Start / stop the auto-save periodic timer. */
	void StartAutoSaveTimer();
	void StopAutoSaveTimer();

	IPlayerDataStore* GetStore() const { return Store.Get(); }

	UFUNCTION(BlueprintCallable, Category = "Player Data")
	bool LoadAccount(const FString& Platform, const FString& PlatformID, FOnsetAccountData& OutAccount);

	UFUNCTION(BlueprintCallable, Category = "Player Data")
	bool CreateAccount(const FString& Platform, const FString& PlatformID);

	UFUNCTION(BlueprintCallable, Category = "Player Data")
	bool LoadCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex, FOnsetFullCharacterData& OutData);

	UFUNCTION(BlueprintCallable, Category = "Player Data")
	bool SaveCharacter(const FString& Platform, const FString& PlatformID, const FOnsetFullCharacterData& Data);

	/** Save a character but preserve stored identity fields (name, level, exp, class, appearance)
	    that the caller may not know about. */
	UFUNCTION(BlueprintCallable, Category = "Player Data")
	bool SaveCharacterPreservingIdentity(const FString& Platform, const FString& PlatformID, FOnsetFullCharacterData& Data);

	UFUNCTION(BlueprintCallable, Category = "Player Data")
	bool DeleteCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex);

private:
	TUniquePtr<IPlayerDataStore> Store;
	FTimerHandle AutoSaveTimerHandle;
};
