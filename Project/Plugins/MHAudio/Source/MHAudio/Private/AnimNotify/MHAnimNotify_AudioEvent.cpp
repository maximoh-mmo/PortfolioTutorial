// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimNotify/MHAnimNotify_AudioEvent.h"
#include "Components/SkeletalMeshComponent.h"
#include "MHAudioSubsystem.h"

UMHAnimNotify_AudioEvent::UMHAnimNotify_AudioEvent()
{
#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(80, 180, 255, 255);
#endif
}

FString UMHAnimNotify_AudioEvent::GetNotifyName_Implementation() const
{
	if (AudioEventTag.IsValid())
	{
		return AudioEventTag.ToString();
	}
	return TEXT("MHAudio Event");
}

void UMHAnimNotify_AudioEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !AudioEventTag.IsValid())
	{
		return;
	}

	UMHAudioSubsystem* AudioSubsystem = UMHAudioSubsystem::Get(MeshComp);
	if (!AudioSubsystem)
	{
		return;
	}

	if (bAttachToSocket)
	{
		AudioSubsystem->PostAudioEventAttached(AudioEventTag, MeshComp, SocketName);
	}
	else
	{
		const FVector Location = SocketName.IsNone() ? MeshComp->GetComponentLocation() : MeshComp->GetSocketLocation(SocketName);
		AudioSubsystem->PostAudioEvent(AudioEventTag, Location, MeshComp->GetComponentRotation());
	}
}
