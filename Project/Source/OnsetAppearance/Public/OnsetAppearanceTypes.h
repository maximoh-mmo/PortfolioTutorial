#pragma once

#include "CoreMinimal.h"
#include "OnsetAppearanceTypes.generated.h"

UENUM(BlueprintType)
enum class EOnsetAppearanceUse : uint8 { Player, AuthoredCharacter };

USTRUCT(BlueprintType)
struct ONSETAPPEARANCE_API FOnsetAppearanceIssue
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FName Code;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString Field;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) FString Message;
};

USTRUCT(BlueprintType)
struct ONSETAPPEARANCE_API FOnsetPartSelection
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName PartId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bOverrideColors = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor ColorA = FLinearColor::White;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor ColorB = FLinearColor::White;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor ColorC = FLinearColor::White;
};

/** Onset-owned appearance data. No account, class, inventory or demo Blueprint types. */
USTRUCT(BlueprintType)
struct ONSETAPPEARANCE_API FOnsetAppearanceRecipe
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 SchemaVersion = 1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName BodyId = TEXT("epic.adult.male");
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName HeadId = TEXT("epic_hmn_adult_M_1");
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<FName, float> BodyMorphs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<FName, float> FaceMorphs;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor SkinColor = FLinearColor(0.75f, 0.55f, 0.38f, 1.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor EyeColor = FLinearColor(0.12f, 0.3f, 0.5f, 1.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FOnsetPartSelection> Parts;
};
