// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MHAudioTypes.h"

class USceneComponent;
class UWorld;

/**
 * Pure abstract provider interface for MHAudio.
 * Decouples gameplay code from native UE audio, MetaSounds, Wwise, FMOD, or custom backends.
 */
class MHAUDIO_API IMHAudioProvider
{
public:
	virtual ~IMHAudioProvider() = default;

	virtual void Initialize(UWorld* InWorld) = 0;
	virtual void Shutdown() = 0;

	// --- One-Shot & 3D Spatial Events ---
	virtual bool PostEvent(const FGameplayTag& EventTag, const FVector& Location, FRotator Rotation = FRotator::ZeroRotator) = 0;
	virtual bool PostEventAttached(const FGameplayTag& EventTag, USceneComponent* AttachToComponent, FName SocketName = NAME_None) = 0;

	// --- Interactive Music ---
	virtual bool PlayMusicTrack(const FGameplayTag& TrackTag, float FadeInDuration = 1.5f) = 0;
	virtual void StopMusic(float FadeOutDuration = 1.5f) = 0;

	// --- States, Switches & RTPCs (Real-Time Parameter Controls) ---
	virtual void SetState(FName StateGroup, FName State) = 0;
	virtual void SetRTPC(FName ParameterName, float Value) = 0;

	// --- Volume Submixes / Buses ---
	virtual void SetBusVolume(EMHAudioBus Bus, float Volume01) = 0;
	virtual float GetBusVolume(EMHAudioBus Bus) const = 0;

	// --- Frame Updates ---
	virtual void Tick(float DeltaTime) = 0;
};
