#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "OnsetClassInfoTypes.generated.h"

USTRUCT(BlueprintType)
struct FOnsetCharacterClassInfo : public FTableRowBase
{
    GENERATED_BODY()
     
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
    FText ClassName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
    TSoftObjectPtr<UTexture2D> ClassIcon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
    float StartingMaxHealth = 100.0f;

    // --- Base stats (combat-formulas §3/§11) ---

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
    float BaseStrength = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
    float BaseIntellect = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
    float BaseVitality = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
    float BaseDefense = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
    float BaseAgility = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
    float BaseLuck = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Class")
    TArray<int32> AvailablePresetIndices;
};
