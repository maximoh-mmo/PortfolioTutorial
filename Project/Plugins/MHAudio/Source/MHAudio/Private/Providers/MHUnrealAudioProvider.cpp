// Copyright Epic Games, Inc. All Rights Reserved.

#include "Providers/MHUnrealAudioProvider.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MHAudioCatalog.h"
#include "MHAudioModule.h"
#include "MHAudioSubsystem.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"

FMHUnrealAudioProvider::FMHUnrealAudioProvider(UMHAudioSubsystem* InSubsystem)
	: Subsystem(InSubsystem)
{
	// Initialize default volumes to 1.0 (100%)
	BusVolumes.Add(EMHAudioBus::Master, 1.0f);
	BusVolumes.Add(EMHAudioBus::Music, 1.0f);
	BusVolumes.Add(EMHAudioBus::SFX, 1.0f);
	BusVolumes.Add(EMHAudioBus::Ambience, 1.0f);
	BusVolumes.Add(EMHAudioBus::Voice, 1.0f);
	BusVolumes.Add(EMHAudioBus::UI, 1.0f);
}

FMHUnrealAudioProvider::~FMHUnrealAudioProvider()
{
	Shutdown();
}

void FMHUnrealAudioProvider::Initialize(UWorld* InWorld)
{
	WorldPtr = InWorld;
	UE_LOG(LogMHAudio, Log, TEXT("FMHUnrealAudioProvider initialized for World: %s"), *GetNameSafe(InWorld));
}

void FMHUnrealAudioProvider::Shutdown()
{
	if (ActiveMusicComp.IsValid())
	{
		ActiveMusicComp->Stop();
		ActiveMusicComp->DestroyComponent();
		ActiveMusicComp.Reset();
	}
	if (FadingMusicComp.IsValid())
	{
		FadingMusicComp->Stop();
		FadingMusicComp->DestroyComponent();
		FadingMusicComp.Reset();
	}
}

UAudioComponent* FMHUnrealAudioProvider::CreateMusicComponent(const FString& Name)
{
	UWorld* World = WorldPtr.Get();
	if (!World)
	{
		return nullptr;
	}

	UAudioComponent* AudioComp = NewObject<UAudioComponent>(World, *Name, RF_Transient);
	if (AudioComp)
	{
		AudioComp->bAutoActivate = false;
		AudioComp->bIsUISound = true; // Music is non-spatialized
		AudioComp->RegisterComponentWithWorld(World);
	}
	return AudioComp;
}

bool FMHUnrealAudioProvider::PostEvent(const FGameplayTag& EventTag, const FVector& Location, FRotator Rotation)
{
	UWorld* World = WorldPtr.Get();
	if (!World || !Subsystem)
	{
		return false;
	}

	UMHAudioCatalog* Catalog = Subsystem->GetAudioCatalog();
	if (!Catalog)
	{
		return false;
	}

	FMHAudioEventDefinition Def;
	if (!Catalog->FindEvent(EventTag, Def))
	{
		return false;
	}

	USoundBase* Sound = Def.Sound.LoadSynchronous();
	if (!Sound)
	{
		return false;
	}

	USoundAttenuation* Atten = Def.Attenuation.LoadSynchronous();
	const float FinalVolume = Def.VolumeMultiplier * GetBusVolume(Def.Bus) * GetBusVolume(EMHAudioBus::Master);

	UGameplayStatics::PlaySoundAtLocation(
		World,
		Sound,
		Location,
		Rotation,
		FinalVolume,
		Def.PitchMultiplier,
		0.0f,
		Atten
	);

	return true;
}

bool FMHUnrealAudioProvider::PostEventAttached(const FGameplayTag& EventTag, USceneComponent* AttachToComponent, FName SocketName)
{
	UWorld* World = WorldPtr.Get();
	if (!World || !AttachToComponent || !Subsystem)
	{
		return false;
	}

	UMHAudioCatalog* Catalog = Subsystem->GetAudioCatalog();
	if (!Catalog)
	{
		return false;
	}

	FMHAudioEventDefinition Def;
	if (!Catalog->FindEvent(EventTag, Def))
	{
		return false;
	}

	USoundBase* Sound = Def.Sound.LoadSynchronous();
	if (!Sound)
	{
		return false;
	}

	USoundAttenuation* Atten = Def.Attenuation.LoadSynchronous();
	const float FinalVolume = Def.VolumeMultiplier * GetBusVolume(Def.Bus) * GetBusVolume(EMHAudioBus::Master);

	UGameplayStatics::SpawnSoundAttached(
		Sound,
		AttachToComponent,
		SocketName,
		FVector::ZeroVector,
		EAttachLocation::SnapToTarget,
		false,
		FinalVolume,
		Def.PitchMultiplier,
		0.0f,
		Atten
	);

	return true;
}

bool FMHUnrealAudioProvider::PlayMusicTrack(const FGameplayTag& TrackTag, float FadeInDuration)
{
	if (!Subsystem)
	{
		return false;
	}

	if (CurrentMusicTrackTag == TrackTag && ActiveMusicComp.IsValid() && ActiveMusicComp->IsPlaying())
	{
		return true; // Already playing this track
	}

	UMHAudioCatalog* Catalog = Subsystem->GetAudioCatalog();
	if (!Catalog)
	{
		return false;
	}

	FMHAudioMusicTrack Track;
	if (!Catalog->FindMusicTrack(TrackTag, Track))
	{
		UE_LOG(LogMHAudio, Warning, TEXT("FMHUnrealAudioProvider::PlayMusicTrack: track tag '%s' not found in catalog."), *TrackTag.ToString());
		return false;
	}

	USoundBase* MusicSound = Track.Sound.LoadSynchronous();
	if (!MusicSound)
	{
		return false;
	}

	const float Duration = FadeInDuration > 0.0f ? FadeInDuration : Track.DefaultFadeDuration;
	ActiveMusicTargetVolume = Track.VolumeMultiplier * GetBusVolume(EMHAudioBus::Music) * GetBusVolume(EMHAudioBus::Master);
	ActiveMusicFadeSpeed = Duration > 0.05f ? (ActiveMusicTargetVolume / Duration) : 100.0f;

	// Crossfade: if active music is currently playing, move it to fading component
	if (ActiveMusicComp.IsValid() && ActiveMusicComp->IsPlaying())
	{
		if (FadingMusicComp.IsValid())
		{
			FadingMusicComp->Stop();
		}
		FadingMusicComp = ActiveMusicComp;
		FadingMusicFadeSpeed = Duration > 0.05f ? (FadingMusicComp->VolumeMultiplier / Duration) : 100.0f;
		ActiveMusicComp.Reset();
	}

	// Create or reuse active music component
	UAudioComponent* NewMusicComp = CreateMusicComponent(TEXT("ActiveMusicAudioComponent"));
	if (!NewMusicComp)
	{
		return false;
	}

	NewMusicComp->SetSound(MusicSound);
	NewMusicComp->SetVolumeMultiplier(Duration > 0.05f ? 0.0f : ActiveMusicTargetVolume);
	NewMusicComp->Play();
	ActiveMusicComp = NewMusicComp;
	CurrentMusicTrackTag = TrackTag;

	UE_LOG(LogMHAudio, Log, TEXT("FMHUnrealAudioProvider: Playing music track '%s' (Fade: %.1fs)"), *TrackTag.ToString(), Duration);
	return true;
}

void FMHUnrealAudioProvider::StopMusic(float FadeOutDuration)
{
	CurrentMusicTrackTag = FGameplayTag();
	if (ActiveMusicComp.IsValid())
	{
		if (FadeOutDuration > 0.05f)
		{
			if (FadingMusicComp.IsValid())
			{
				FadingMusicComp->Stop();
			}
			FadingMusicComp = ActiveMusicComp;
			FadingMusicFadeSpeed = FadingMusicComp->VolumeMultiplier / FadeOutDuration;
			ActiveMusicComp.Reset();
		}
		else
		{
			ActiveMusicComp->Stop();
			ActiveMusicComp.Reset();
		}
	}
}

void FMHUnrealAudioProvider::SetState(FName StateGroup, FName State)
{
	ActiveStates.Add(StateGroup, State);
	UE_LOG(LogMHAudio, Log, TEXT("FMHUnrealAudioProvider::SetState: Group='%s', State='%s'"), *StateGroup.ToString(), *State.ToString());

	// Built-in dynamic music handling for CombatState
	if (StateGroup == FName("CombatState"))
	{
		if (State == FName("Combat"))
		{
			const FGameplayTag CombatMusicTag = FGameplayTag::RequestGameplayTag(FName("Audio.Music.Combat"), false);
			if (CombatMusicTag.IsValid())
			{
				PlayMusicTrack(CombatMusicTag, 1.0f);
			}
		}
		else if (State == FName("Boss"))
		{
			const FGameplayTag BossMusicTag = FGameplayTag::RequestGameplayTag(FName("Audio.Music.Boss"), false);
			if (BossMusicTag.IsValid())
			{
				PlayMusicTrack(BossMusicTag, 0.5f);
			}
		}
		else if (State == FName("Exploration") || State == FName("Ambient"))
		{
			const FGameplayTag AmbientMusicTag = FGameplayTag::RequestGameplayTag(FName("Audio.Music.Exploration"), false);
			if (AmbientMusicTag.IsValid())
			{
				PlayMusicTrack(AmbientMusicTag, 2.0f);
			}
		}
	}
}

void FMHUnrealAudioProvider::SetRTPC(FName ParameterName, float Value)
{
	RTPCValues.Add(ParameterName, Value);

	// Low health effect: slightly drop music volume and apply low pass filter feel
	if (ParameterName == FName("PlayerHealthPercent") && ActiveMusicComp.IsValid())
	{
		const float HealthPct = FMath::Clamp(Value, 0.0f, 1.0f);
		if (HealthPct < 0.25f)
		{
			// Low health tension ducking
			const float DuckedVolume = ActiveMusicTargetVolume * FMath::Lerp(0.35f, 1.0f, HealthPct / 0.25f);
			ActiveMusicComp->SetVolumeMultiplier(DuckedVolume);
		}
	}
}

void FMHUnrealAudioProvider::SetBusVolume(EMHAudioBus Bus, float Volume01)
{
	const float Clamped = FMath::Clamp(Volume01, 0.0f, 1.0f);
	BusVolumes.Add(Bus, Clamped);

	if (Bus == EMHAudioBus::Master || Bus == EMHAudioBus::Music)
	{
		UpdateMusicVolumes();
	}
}

float FMHUnrealAudioProvider::GetBusVolume(EMHAudioBus Bus) const
{
	if (const float* Found = BusVolumes.Find(Bus))
	{
		return *Found;
	}
	return 1.0f;
}

void FMHUnrealAudioProvider::UpdateMusicVolumes()
{
	if (ActiveMusicComp.IsValid())
	{
		const float MasterVol = GetBusVolume(EMHAudioBus::Master);
		const float MusicVol = GetBusVolume(EMHAudioBus::Music);
		ActiveMusicComp->SetVolumeMultiplier(ActiveMusicTargetVolume * MusicVol * MasterVol);
	}
}

void FMHUnrealAudioProvider::Tick(float DeltaTime)
{
	// Fade in active music
	if (ActiveMusicComp.IsValid() && ActiveMusicComp->IsPlaying())
	{
		const float CurrentVol = ActiveMusicComp->VolumeMultiplier;
		if (CurrentVol < ActiveMusicTargetVolume)
		{
			const float NewVol = FMath::Min(ActiveMusicTargetVolume, CurrentVol + ActiveMusicFadeSpeed * DeltaTime);
			ActiveMusicComp->SetVolumeMultiplier(NewVol);
		}
	}

	// Fade out fading music
	if (FadingMusicComp.IsValid() && FadingMusicComp->IsPlaying())
	{
		const float CurrentVol = FadingMusicComp->VolumeMultiplier;
		const float NewVol = CurrentVol - FadingMusicFadeSpeed * DeltaTime;
		if (NewVol <= 0.001f)
		{
			FadingMusicComp->Stop();
			FadingMusicComp.Reset();
		}
		else
		{
			FadingMusicComp->SetVolumeMultiplier(NewVol);
		}
	}
}
