// Copyright Epic Games, Inc. All Rights Reserved.

#include "MHAudioSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameplayTagsManager.h"
#include "MHAudioModule.h"
#include "Providers/MHUnrealAudioProvider.h"

UMHAudioSubsystem* UMHAudioSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GI = World->GetGameInstance();
	return GI ? GI->GetSubsystem<UMHAudioSubsystem>() : nullptr;
}

void UMHAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Register core MHAudio gameplay tags
	UGameplayTagsManager& TagManager = UGameplayTagsManager::Get();
	TagManager.AddNativeGameplayTag(FName("Audio.Event"), TEXT("Root tag for MHAudio sound events"));
	TagManager.AddNativeGameplayTag(FName("Audio.Music"), TEXT("Root tag for MHAudio music tracks"));
	TagManager.AddNativeGameplayTag(FName("Audio.Music.Combat"), TEXT("Dynamic combat music track"));
	TagManager.AddNativeGameplayTag(FName("Audio.Music.Boss"), TEXT("Boss encounter music track"));
	TagManager.AddNativeGameplayTag(FName("Audio.Music.Exploration"), TEXT("Exploration/ambient music track"));

	// Initialize the default native Unreal Engine audio provider
	ActiveProvider = MakeUnique<FMHUnrealAudioProvider>(this);
	if (UWorld* World = GetGameInstance()->GetWorld())
	{
		ActiveProvider->Initialize(World);
	}

	UE_LOG(LogMHAudio, Log, TEXT("UMHAudioSubsystem initialized with FMHUnrealAudioProvider."));
}

void UMHAudioSubsystem::Deinitialize()
{
	if (ActiveProvider)
	{
		ActiveProvider->Shutdown();
		ActiveProvider.Reset();
	}

	Super::Deinitialize();
}

void UMHAudioSubsystem::Tick(float DeltaTime)
{
	if (ActiveProvider)
	{
		ActiveProvider->Tick(DeltaTime);
	}
}

TStatId UMHAudioSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UMHAudioSubsystem, STATGROUP_Tickables);
}

void UMHAudioSubsystem::SetAudioCatalog(UMHAudioCatalog* InCatalog)
{
	CurrentCatalog = InCatalog;
	if (CurrentCatalog)
	{
		CurrentCatalog->RebuildLookupMaps();
		UE_LOG(LogMHAudio, Log, TEXT("UMHAudioSubsystem: Loaded catalog '%s' with %d events and %d tracks."),
			*GetNameSafe(InCatalog), InCatalog->Events.Num(), InCatalog->MusicTracks.Num());
	}
}

bool UMHAudioSubsystem::PostAudioEvent(FGameplayTag EventTag, FVector Location, FRotator Rotation)
{
	if (!ActiveProvider || !EventTag.IsValid())
	{
		return false;
	}
	return ActiveProvider->PostEvent(EventTag, Location, Rotation);
}

bool UMHAudioSubsystem::PostAudioEventAttached(FGameplayTag EventTag, USceneComponent* AttachToComponent, FName SocketName)
{
	if (!ActiveProvider || !EventTag.IsValid() || !AttachToComponent)
	{
		return false;
	}
	return ActiveProvider->PostEventAttached(EventTag, AttachToComponent, SocketName);
}

bool UMHAudioSubsystem::PlayMusicTrack(FGameplayTag TrackTag, float FadeInDuration)
{
	if (!ActiveProvider || !TrackTag.IsValid())
	{
		return false;
	}
	return ActiveProvider->PlayMusicTrack(TrackTag, FadeInDuration);
}

void UMHAudioSubsystem::StopMusic(float FadeOutDuration)
{
	if (ActiveProvider)
	{
		ActiveProvider->StopMusic(FadeOutDuration);
	}
}

void UMHAudioSubsystem::SetMusicState(FName StateGroup, FName State)
{
	if (ActiveProvider)
	{
		ActiveProvider->SetState(StateGroup, State);
	}
}

void UMHAudioSubsystem::SetRTPC(FName ParameterName, float Value)
{
	if (ActiveProvider)
	{
		ActiveProvider->SetRTPC(ParameterName, Value);
	}
}

void UMHAudioSubsystem::SetBusVolume(EMHAudioBus Bus, float Volume01)
{
	if (ActiveProvider)
	{
		ActiveProvider->SetBusVolume(Bus, Volume01);
	}
}

float UMHAudioSubsystem::GetBusVolume(EMHAudioBus Bus) const
{
	return ActiveProvider ? ActiveProvider->GetBusVolume(Bus) : 1.0f;
}
