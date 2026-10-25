// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MHAudioTypes.generated.h"

class USoundBase;
class USoundAttenuation;

/**
 * Universal audio volume channels (buses) shared across UI settings, middleware, and gameplay.
 */
UENUM(BlueprintType)
enum class EMHAudioBus : uint8
{
	Master		UMETA(DisplayName = "Master"),
	Music		UMETA(DisplayName = "Music"),
	SFX			UMETA(DisplayName = "SFX"),
	Ambience	UMETA(DisplayName = "Ambience"),
	Voice		UMETA(DisplayName = "Voice"),
	UI			UMETA(DisplayName = "UI")
};

/**
 * Maps a provider-neutral gameplay tag (e.g. Audio.Event.Combat.FireballCast)
 * to a native sound asset and playback parameters.
 */
USTRUCT(BlueprintType)
struct MHAUDIO_API FMHAudioEventDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Event")
	FGameplayTag EventTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Event")
	TSoftObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Event")
	EMHAudioBus Bus = EMHAudioBus::SFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Event", meta = (ClampMin = "0.0", ClampMax = "4.0"))
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Event", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float PitchMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Event")
	TSoftObjectPtr<USoundAttenuation> Attenuation;
};

/**
 * Background / combat dynamic music track configuration.
 */
USTRUCT(BlueprintType)
struct MHAUDIO_API FMHAudioMusicTrack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	FGameplayTag TrackTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	TSoftObjectPtr<USoundBase> Sound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music", meta = (ClampMin = "0.0"))
	float DefaultFadeDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Music")
	bool bLoop = true;
};
