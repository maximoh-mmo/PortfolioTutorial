#include "Subsystem/OnsetPlayerDataSubsystem.h"
#include "DataStoreFactory.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY(LogPlayerData)

namespace
{
	FString MakeCharacterCacheKey(const FString& Platform, const FString& PlatformID, int32 SlotIndex)
	{
		return FString::Printf(TEXT("%s|%s|%d"), *Platform, *PlatformID, SlotIndex);
	}
}

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

	FString CmdLineType;
	if (FParse::Value(FCommandLine::Get(), TEXT("OnsetDataStoreType="), CmdLineType))
	{
		DataStoreType = CmdLineType;
	}
	FString CmdLineConnectionString;
	if (FParse::Value(FCommandLine::Get(), TEXT("OnsetDataStoreURL="), CmdLineConnectionString))
	{
		ConnectionString = CmdLineConnectionString;
	}

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

	// HTTP-backed stores (account server) save on demand; the periodic timer is a no-op there,
	// so only arm it for local stores.
	if (DataStoreType != TEXT("HttpApi"))
	{
		StartAutoSaveTimer();
	}
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
	const bool bLoaded = Store->LoadCharacter(Platform, PlatformID, SlotIndex, OutData);
	if (bLoaded)
	{
		IdentityCache.Add(MakeCharacterCacheKey(Platform, PlatformID, SlotIndex), OutData);
	}
	return bLoaded;
}

bool UOnsetPlayerDataSubsystem::SaveCharacter(const FString& Platform, const FString& PlatformID, const FOnsetFullCharacterData& Data)
{
	if (!Store) return false;
	return Store->SaveCharacter(Platform, PlatformID, Data);
}

bool UOnsetPlayerDataSubsystem::SaveCharacterPreservingIdentity(const FString& Platform, const FString& PlatformID, FOnsetFullCharacterData& Data)
{
	const FString CacheKey = MakeCharacterCacheKey(Platform, PlatformID, Data.SlotIndex);
	const FOnsetFullCharacterData* Cached = IdentityCache.Find(CacheKey);
	if (Cached)
	{
		Data.CharacterName = Cached->CharacterName;
		Data.Level = Cached->Level;
		Data.Experience = Cached->Experience;
		Data.CharacterClass = Cached->CharacterClass;
		Data.AppearanceJSON = Cached->AppearanceJSON;
		return SaveCharacter(Platform, PlatformID, Data);
	}

	// Cold cache (e.g. first save after a server restart with no prior character load): read once
	// to preserve identity; LoadCharacter seeds the cache so subsequent saves are write-only.
	FOnsetFullCharacterData Existing;
	if (LoadCharacter(Platform, PlatformID, Data.SlotIndex, Existing))
	{
		Data.CharacterName = Existing.CharacterName;
		Data.Level = Existing.Level;
		Data.Experience = Existing.Experience;
		Data.UnspentStatPoints = Existing.UnspentStatPoints;
		Data.CharacterClass = Existing.CharacterClass;
		Data.AppearanceJSON = Existing.AppearanceJSON;
	}
	return SaveCharacter(Platform, PlatformID, Data);
}

void UOnsetPlayerDataSubsystem::UpdateRuntimeProgression(const FString& Platform, const FString& PlatformID, int32 SlotIndex, int32 Level, int32 Experience, int32 UnspentStatPoints)
{
	const FString CacheKey = MakeCharacterCacheKey(Platform, PlatformID, SlotIndex);
	FOnsetFullCharacterData* Cached = IdentityCache.Find(CacheKey);
	if (Cached)
	{
		Cached->Level = Level;
		Cached->Experience = Experience;
		Cached->UnspentStatPoints = UnspentStatPoints;
	}
	else
	{
		// Cold cache (server restart with an already-loaded session): seed it so a later
		// SaveCharacterPreservingIdentity doesn't clobber the fresh progression values.
		FOnsetFullCharacterData Seed;
		Seed.SlotIndex = SlotIndex;
		Seed.Level = Level;
		Seed.Experience = Experience;
		Seed.UnspentStatPoints = UnspentStatPoints;
		IdentityCache.Add(CacheKey, Seed);
	}
}

bool UOnsetPlayerDataSubsystem::DeleteCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex)
{
	if (!Store) return false;
	const bool bDeleted = Store->DeleteCharacter(Platform, PlatformID, SlotIndex);
	if (bDeleted)
	{
		IdentityCache.Remove(MakeCharacterCacheKey(Platform, PlatformID, SlotIndex));
	}
	return bDeleted;
}

void UOnsetPlayerDataSubsystem::SaveAll()
{
	if (Store)
		Store->SaveAll();
}
