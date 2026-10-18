#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "OnsetAppearanceTypes.h"
#include "OnsetAppearanceCatalog.generated.h"

class USkeletalMesh;
class UTexture2D;

USTRUCT(BlueprintType)
struct ONSETAPPEARANCE_API FOnsetBodyDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bPlayerSelectable = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<USkeletalMesh> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName HeadSocket = TEXT("NeckSocket");
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName HairSocket = TEXT("epic_hmn_HairSocket");
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> SupportedMorphs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName SkinParameter = TEXT("L1_Color");
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName SkinMaterialSlot;
};

USTRUCT(BlueprintType)
struct ONSETAPPEARANCE_API FOnsetHeadDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<USkeletalMesh> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> CompatibleBodies;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> SupportedMorphs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName SkinParameter = TEXT("L1_Color");
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName SkinMaterialSlot;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName EyeParameter = TEXT("IrisColor");
};

USTRUCT(BlueprintType)
struct ONSETAPPEARANCE_API FOnsetPartDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<USkeletalMesh> Mesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> CompatibleBodies;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Slot;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bAttachToHead = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bShareBodyPose = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bHair = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bHideHair = false;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Socket;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FTransform RelativeTransform;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> SuppressedSlots;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> BodyHideTexture;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName BodyHideParameter;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName, float> RequiredBodyMorphs;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ColorParameterA = TEXT("L1_Color");
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ColorParameterB = TEXT("L2_Color");
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ColorParameterC = TEXT("L3_Color");
};

UCLASS(BlueprintType)
class ONSETAPPEARANCE_API UOnsetAppearanceCatalog : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName, FOnsetBodyDefinition> Bodies;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName, FOnsetHeadDefinition> Heads;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName, FOnsetPartDefinition> Parts;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> BlankBodyHideTexture;

    UFUNCTION(BlueprintCallable, Category="Onset|Appearance")
    bool ValidateRecipe(const FOnsetAppearanceRecipe& Recipe, EOnsetAppearanceUse Use, TArray<FOnsetAppearanceIssue>& Issues) const;

    void GetRecipeAssets(const FOnsetAppearanceRecipe& Recipe, TArray<FSoftObjectPath>& Assets) const;
};
