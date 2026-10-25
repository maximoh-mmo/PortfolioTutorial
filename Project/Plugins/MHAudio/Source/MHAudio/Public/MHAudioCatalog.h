// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MHAudioTypes.h"
#include "MHAudioCatalog.generated.h"

class USoundClass;

/**
 * Data asset defining the audio mapping library for the current game/project.
 * Maps abstract GameplayTags to sound assets, volume multipliers, attenuation profiles, and music tracks.
 */
UCLASS(BlueprintType)
class MHAUDIO_API UMHAudioCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Registered sound events (SFX, impacts, casts, footsteps, UI). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio Events")
	TArray<FMHAudioEventDefinition> Events;

	/** Registered music tracks (ambient, combat, boss, victory). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music Tracks")
	TArray<FMHAudioMusicTrack> MusicTracks;

	// --- Bus SoundClass Overrides (Optional, used by Native Unreal Provider) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buses")
	TObjectPtr<USoundClass> MasterSoundClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buses")
	TObjectPtr<USoundClass> MusicSoundClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buses")
	TObjectPtr<USoundClass> SFXSoundClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buses")
	TObjectPtr<USoundClass> AmbienceSoundClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buses")
	TObjectPtr<USoundClass> VoiceSoundClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buses")
	TObjectPtr<USoundClass> UISoundClass;

	/** Find an event definition by tag. Returns true if found. */
	UFUNCTION(BlueprintCallable, Category = "Audio Catalog")
	bool FindEvent(FGameplayTag EventTag, FMHAudioEventDefinition& OutEvent) const;

	/** Find a music track definition by tag. Returns true if found. */
	UFUNCTION(BlueprintCallable, Category = "Audio Catalog")
	bool FindMusicTrack(FGameplayTag TrackTag, FMHAudioMusicTrack& OutTrack) const;

	/** Rebuilds lookup maps for O(1) query time. Called on load and post-edit. */
	void RebuildLookupMaps();

	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY(Transient)
	TMap<FGameplayTag, int32> EventTagIndexMap;

	UPROPERTY(Transient)
	TMap<FGameplayTag, int32> TrackTagIndexMap;
};
