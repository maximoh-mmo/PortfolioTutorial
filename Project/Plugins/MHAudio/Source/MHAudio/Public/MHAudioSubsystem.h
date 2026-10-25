// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "MHAudioTypes.h"
#include "MHAudioCatalog.h"
#include "IMHAudioProvider.h"
#include "MHAudioSubsystem.generated.h"

class USceneComponent;

/**
 * Primary GameInstance subsystem managing provider-neutral audio, dynamic music,
 * RTPC parameters, and volume buses across levels.
 */
UCLASS()
class MHAUDIO_API UMHAudioSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	/** Static helper to retrieve the audio subsystem from any world context object. */
	UFUNCTION(BlueprintPure, Category = "MHAudio", meta = (WorldContext = "WorldContextObject"))
	static UMHAudioSubsystem* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// --- FTickableGameObject ---
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return ActiveProvider != nullptr; }

	// --- Catalog & Provider Configuration ---
	UFUNCTION(BlueprintCallable, Category = "MHAudio")
	void SetAudioCatalog(UMHAudioCatalog* InCatalog);

	UFUNCTION(BlueprintPure, Category = "MHAudio")
	UMHAudioCatalog* GetAudioCatalog() const { return CurrentCatalog; }

	// --- Events & Playback ---
	UFUNCTION(BlueprintCallable, Category = "MHAudio", meta = (WorldContext = "WorldContextObject"))
	bool PostAudioEvent(FGameplayTag EventTag, FVector Location, FRotator Rotation = FRotator::ZeroRotator);

	UFUNCTION(BlueprintCallable, Category = "MHAudio")
	bool PostAudioEventAttached(FGameplayTag EventTag, USceneComponent* AttachToComponent, FName SocketName = NAME_None);

	// --- Dynamic Interactive Music ---
	UFUNCTION(BlueprintCallable, Category = "MHAudio")
	bool PlayMusicTrack(FGameplayTag TrackTag, float FadeInDuration = 1.5f);

	UFUNCTION(BlueprintCallable, Category = "MHAudio")
	void StopMusic(float FadeOutDuration = 1.5f);

	// --- States & Real-Time Parameter Controls (RTPCs) ---
	UFUNCTION(BlueprintCallable, Category = "MHAudio")
	void SetMusicState(FName StateGroup, FName State);

	UFUNCTION(BlueprintCallable, Category = "MHAudio")
	void SetRTPC(FName ParameterName, float Value);

	// --- Buses & Volume Controls ---
	UFUNCTION(BlueprintCallable, Category = "MHAudio")
	void SetBusVolume(EMHAudioBus Bus, float Volume01);

	UFUNCTION(BlueprintPure, Category = "MHAudio")
	float GetBusVolume(EMHAudioBus Bus) const;

	IMHAudioProvider* GetActiveProvider() const { return ActiveProvider.Get(); }

private:
	TUniquePtr<IMHAudioProvider> ActiveProvider;

	UPROPERTY(Transient)
	TObjectPtr<UMHAudioCatalog> CurrentCatalog;
};
