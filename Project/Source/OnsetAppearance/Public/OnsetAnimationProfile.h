#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "OnsetAnimationProfile.generated.h"

class USkeleton;
class UBlendSpace;
class UAnimSequenceBase;
class UAnimSequence;
class UAnimMontage;

/**
 * Reusable animation profile defining rig compatibility, locomotion blendspace,
 * idle postures, fidgets, conversational talk gestures, and protocol salutes.
 */
UCLASS(BlueprintType)
class ONSETAPPEARANCE_API UOnsetAnimationProfile : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Unique identifier for this animation profile (e.g. AnimProfile_ConfederateMilitary). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profile")
	FName ProfileId;

	/** Human-readable display name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profile")
	FText DisplayName;

	/** Compatible rig tag (e.g. "EpicHumanoidAdult", "Kid", "Quadruped"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compatibility")
	FName RigCompatibility = TEXT("EpicHumanoidAdult");

	/** Target skeleton this profile is authored for. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compatibility")
	TSoftObjectPtr<USkeleton> TargetSkeleton;

	/** Default standing idle animation (sequence or blend space). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion & Idle")
	TSoftObjectPtr<UAnimSequenceBase> DefaultIdleAnim;

	/** Locomotion BlendSpace (Idle/Walk/Run). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Locomotion & Idle")
	TSoftObjectPtr<UBlendSpace> LocomotionBlendSpace;

	/** Random fidget montages or sequences played while standing at ease. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Idles & Fidgets")
	TArray<TSoftObjectPtr<UAnimMontage>> IdleFidgets;

	/** Conversational gestures played during dialogue interactions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue & Gestures")
	TArray<TSoftObjectPtr<UAnimMontage>> TalkGestures;

	/** Primary protocol salute / greeting montage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Protocol & Etiquette")
	TSoftObjectPtr<UAnimMontage> SaluteMontage;

	/** Acknowledging salute montage played by superiors responding to an inferior rank. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Protocol & Etiquette")
	TSoftObjectPtr<UAnimMontage> SaluteAcknowledgeMontage;

	/** Out-of-combat weapon inspection or posture montage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flavour")
	TSoftObjectPtr<UAnimMontage> InspectWeaponMontage;

	/** Hit reaction montages (light, medium, heavy flinch). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reactions")
	TArray<TSoftObjectPtr<UAnimMontage>> HitReactMontages;

	/** Death montages. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reactions")
	TArray<TSoftObjectPtr<UAnimMontage>> DeathMontages;

	/** Sample animation sequences for previewing and turntable scrub in the Appearance Studio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preview")
	TArray<TSoftObjectPtr<UAnimSequence>> SamplePreviewAnimations;

	/** Author notes / lore / guidelines. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profile", meta = (MultiLine = "true"))
	FString Notes;
};
