#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AIProfile.generated.h"

class UStateTree;
class UMaterialInterface;
class USkeletalMesh;
class UAnimInstance;

UCLASS(BlueprintType)
class ONSET_API UAIProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	TSubclassOf<UAnimInstance> AnimBlueprintClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
	TObjectPtr<UMaterialInterface> OverrideMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StateTree")
	TObjectPtr<UStateTree> StateTreeAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	float SightRange = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	float SightAngle = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Perception")
	float HearingRange = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Aggression = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FleeThreshold = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Behavior")
	float AssistRadius = 600.0f;
};
