#include "Subsystem/OnsetPlayerDataSubsystem.h"
#include "DataStoreFactory.h"
#include "Engine/World.h"
#include "Misc/ConfigCacheIni.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY(LogPlayerData)

void UOnsetPlayerDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
	{
		UE_LOG(LogPlayerData, Log, TEXT("UOnsetPlayerDataSubsystem: skipping init on client"));
		return;
	}

	FString DataStoreType;
	FString ConnectionString;
	GConfig->GetString(TEXT("Onset.DataStore"), TEXT("Type"), DataStoreType, GEngineIni);
	GConfig->GetString(TEXT("Onset.DataStore"), TEXT("ConnectionString"), ConnectionString, GEngineIni);

	if (DataStoreType.IsEmpty())
	{
		DataStoreType = TEXT("SQLite");
		ConnectionString = TEXT("");
	}

	bool bInitialized = false;
	Store = CreateDataStore(DataStoreType, ConnectionString, bInitialized);
	if (!Store)
	{
		UE_LOG(LogPlayerData, Warning, TEXT("DataStore type '%s' failed or unavailable — falling back to SQLite"), *DataStoreType);
		DataStoreType = TEXT("SQLite");
		Store = CreateDataStore(DataStoreType, ConnectionString, bInitialized);
	}

	UE_LOG(LogPlayerData, Log, TEXT("UOnsetPlayerDataSubsystem: initialized with %s (success=%d)"), *DataStoreType, bInitialized);

	StartAutoSaveTimer();
}

void UOnsetPlayerDataSubsystem::Deinitialize()
{
	StopAutoSaveTimer();
	SaveAll();
	Store.Reset();
	Super::Deinitialize();
}

void UOnsetPlayerDataSubsystem::StartAutoSaveTimer()
{
	float Interval = 300.0f; // default 5 minutes
	GConfig->GetFloat(TEXT("Onset.DataStore"), TEXT("AutoSaveInterval"), Interval, GEngineIni);
	if (Interval > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(AutoSaveTimerHandle, this,
			&UOnsetPlayerDataSubsystem::SaveAll, Interval, true);
		UE_LOG(LogPlayerData, Log, TEXT("Auto-save timer started (interval=%.1fs)"), Interval);
	}
}

void UOnsetPlayerDataSubsystem::StopAutoSaveTimer()
{
	if (AutoSaveTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(AutoSaveTimerHandle);
		UE_LOG(LogPlayerData, Log, TEXT("Auto-save timer stopped"));
	}
}

bool UOnsetPlayerDataSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
		return false;

	UWorld* World = Cast<UWorld>(Outer);
	return World && World->GetNetMode() != NM_Client;
}

bool UOnsetPlayerDataSubsystem::LoadAccount(const FString& Platform, const FString& PlatformID, FOnsetAccountData& OutAccount)
{
	if (!Store) return false;
	return Store->LoadAccount(Platform, PlatformID, OutAccount);
}

bool UOnsetPlayerDataSubsystem::CreateAccount(const FString& Platform, const FString& PlatformID)
{
	if (!Store) return false;
	return Store->CreateAccount(Platform, PlatformID);
}

bool UOnsetPlayerDataSubsystem::LoadCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex, FOnsetFullCharacterData& OutData)
{
	if (!Store) return false;
	return Store->LoadCharacter(Platform, PlatformID, SlotIndex, OutData);
}

bool UOnsetPlayerDataSubsystem::SaveCharacter(const FString& Platform, const FString& PlatformID, const FOnsetFullCharacterData& Data)
{
	if (!Store) return false;
	return Store->SaveCharacter(Platform, PlatformID, Data);
}

bool UOnsetPlayerDataSubsystem::DeleteCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex)
{
	if (!Store) return false;
	return Store->DeleteCharacter(Platform, PlatformID, SlotIndex);
}

void UOnsetPlayerDataSubsystem::SaveAll()
{
	if (Store)
		Store->SaveAll();
}
