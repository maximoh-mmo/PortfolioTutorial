// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IMHAudioProvider.h"
#include "MHAudioTypes.h"

class UAudioComponent;
class UMHAudioSubsystem;
class UWorld;

/**
 * Native Unreal Engine provider for MHAudio.
 * Supports 2D/3D audio spatialization, dual-track dynamic music crossfading,
 * RTPC parameter mapping, and bus volume management out of the box.
 */
class FMHUnrealAudioProvider : public IMHAudioProvider
{
public:
	explicit FMHUnrealAudioProvider(UMHAudioSubsystem* InSubsystem);
	virtual ~FMHUnrealAudioProvider() override;

	virtual void Initialize(UWorld* InWorld) override;
	virtual void Shutdown() override;

	virtual bool PostEvent(const FGameplayTag& EventTag, const FVector& Location, FRotator Rotation = FRotator::ZeroRotator) override;
	virtual bool PostEventAttached(const FGameplayTag& EventTag, USceneComponent* AttachToComponent, FName SocketName = NAME_None) override;

	virtual bool PlayMusicTrack(const FGameplayTag& TrackTag, float FadeInDuration = 1.5f) override;
	virtual void StopMusic(float FadeOutDuration = 1.5f) override;

	virtual void SetState(FName StateGroup, FName State) override;
	virtual void SetRTPC(FName ParameterName, float Value) override;

	virtual void SetBusVolume(EMHAudioBus Bus, float Volume01) override;
	virtual float GetBusVolume(EMHAudioBus Bus) const override;

	virtual void Tick(float DeltaTime) override;

private:
	UAudioComponent* CreateMusicComponent(const FString& Name);
	void UpdateMusicVolumes();

	UMHAudioSubsystem* Subsystem = nullptr;
	TWeakObjectPtr<UWorld> WorldPtr;

	// Dual audio components for smooth musical crossfading
	TWeakObjectPtr<UAudioComponent> ActiveMusicComp;
	TWeakObjectPtr<UAudioComponent> FadingMusicComp;

	FGameplayTag CurrentMusicTrackTag;
	float ActiveMusicTargetVolume = 1.0f;
	float ActiveMusicFadeSpeed = 1.0f;
	float FadingMusicFadeSpeed = 1.0f;

	TMap<EMHAudioBus, float> BusVolumes;
	TMap<FName, float> RTPCValues;
	TMap<FName, FName> ActiveStates;
};
