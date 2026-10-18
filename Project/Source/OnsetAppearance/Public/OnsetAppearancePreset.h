#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "OnsetAppearanceTypes.h"
#include "OnsetAppearancePreset.generated.h"

UCLASS(BlueprintType)
class ONSETAPPEARANCE_API UOnsetAppearancePreset : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EOnsetAppearanceUse Use = EOnsetAppearanceUse::AuthoredCharacter;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FOnsetAppearanceRecipe Recipe;
};
