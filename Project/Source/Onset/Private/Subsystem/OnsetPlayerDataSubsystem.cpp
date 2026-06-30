#include "Subsystem/OnsetPlayerDataSubsystem.h"
#include "Data/FSQLiteStore.h"
#include "Data/FPgSQLStore.h"
#include "Engine/World.h"
#include "Misc/ConfigCacheIni.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY(LogPlayerData)

void UOnsetPlayerDataSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (IsRunningCommandlet())
	{
		UE_LOG(LogPlayerData, Log, TEXT("UOnsetPlayerDataSubsystem: skipping init during commandlet (cook)"));
		return;
	}

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

	if (DataStoreType.Equals(TEXT("SQLite"), ESearchCase::IgnoreCase))
	{
		TUniquePtr<FSQLiteStore> SQLiteStore = MakeUnique<FSQLiteStore>();
		bInitialized = SQLiteStore->Initialize(ConnectionString);
		if (bInitialized)
			Store = MoveTemp(SQLiteStore);
	}
	else if (DataStoreType.Equals(TEXT("Postgres"), ESearchCase::IgnoreCase) || DataStoreType.Equals(TEXT("PgSQL"), ESearchCase::IgnoreCase))
	{
		TUniquePtr<FPgSQLStore> PgStore = MakeUnique<FPgSQLStore>();
		bInitialized = PgStore->Initialize(ConnectionString);
		if (bInitialized)
			Store = MoveTemp(PgStore);
	}
	else
	{
		UE_LOG(LogPlayerData, Error, TEXT("Unknown DataStore type: %s — falling back to SQLite"), *DataStoreType);
		TUniquePtr<FSQLiteStore> SQLiteStore = MakeUnique<FSQLiteStore>();
		bInitialized = SQLiteStore->Initialize(ConnectionString);
		if (bInitialized)
			Store = MoveTemp(SQLiteStore);
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

	if (IsRunningCommandlet())
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
