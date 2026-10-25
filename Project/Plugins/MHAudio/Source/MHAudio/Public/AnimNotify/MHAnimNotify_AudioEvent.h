// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "MHAnimNotify_AudioEvent.generated.h"

class USkeletalMeshComponent;
class UAnimSequenceBase;

/**
 * Universal Animation Notify for triggering MHAudio events (footsteps, melee swings, spell releases, cloth rustle)
 * on skeletal meshes during montages or sequences.
 */
UCLASS(const, hidecategories=Object, collapsecategories, meta=(DisplayName="MHAudio Event"))
class MHAUDIO_API UMHAnimNotify_AudioEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	UMHAnimNotify_AudioEvent();

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

	/** Provider-neutral GameplayTag identifying the audio event to trigger. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	FGameplayTag AudioEventTag;

	/** Optional socket/bone name to attach audio or emit from (e.g. "foot_r", "hand_r", "weapon_socket"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	FName SocketName = NAME_None;

	/** If true, attaches audio component to socket; if false, plays at world position of socket at notify trigger time. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	bool bAttachToSocket = true;
};
