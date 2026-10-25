// Copyright Epic Games, Inc. All Rights Reserved.

#include "MHAudioCatalog.h"

void UMHAudioCatalog::PostLoad()
{
	Super::PostLoad();
	RebuildLookupMaps();
}

#if WITH_EDITOR
void UMHAudioCatalog::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildLookupMaps();
}
#endif

void UMHAudioCatalog::RebuildLookupMaps()
{
	EventTagIndexMap.Reset();
	EventTagIndexMap.Reserve(Events.Num());
	for (int32 Index = 0; Index < Events.Num(); ++Index)
	{
		if (Events[Index].EventTag.IsValid())
		{
			EventTagIndexMap.Add(Events[Index].EventTag, Index);
		}
	}

	TrackTagIndexMap.Reset();
	TrackTagIndexMap.Reserve(MusicTracks.Num());
	for (int32 Index = 0; Index < MusicTracks.Num(); ++Index)
	{
		if (MusicTracks[Index].TrackTag.IsValid())
		{
			TrackTagIndexMap.Add(MusicTracks[Index].TrackTag, Index);
		}
	}
}

bool UMHAudioCatalog::FindEvent(FGameplayTag EventTag, FMHAudioEventDefinition& OutEvent) const
{
	if (!EventTag.IsValid())
	{
		return false;
	}

	if (const int32* FoundIndex = EventTagIndexMap.Find(EventTag))
	{
		if (Events.IsValidIndex(*FoundIndex))
		{
			OutEvent = Events[*FoundIndex];
			return true;
		}
	}

	// Fallback linear scan if map is uninitialized
	for (const FMHAudioEventDefinition& Def : Events)
	{
		if (Def.EventTag == EventTag)
		{
			OutEvent = Def;
			return true;
		}
	}

	return false;
}

bool UMHAudioCatalog::FindMusicTrack(FGameplayTag TrackTag, FMHAudioMusicTrack& OutTrack) const
{
	if (!TrackTag.IsValid())
	{
		return false;
	}

	if (const int32* FoundIndex = TrackTagIndexMap.Find(TrackTag))
	{
		if (MusicTracks.IsValidIndex(*FoundIndex))
		{
			OutTrack = MusicTracks[*FoundIndex];
			return true;
		}
	}

	// Fallback linear scan
	for (const FMHAudioMusicTrack& Track : MusicTracks)
	{
		if (Track.TrackTag == TrackTag)
		{
			OutTrack = Track;
			return true;
		}
	}

	return false;
}
