#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OnsetAppearanceCatalog.h"
#include "OnsetAppearanceLibrary.generated.h"

UCLASS()
class ONSETAPPEARANCE_API UOnsetAppearanceLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="Onset|Appearance")
    static bool EncodeRecipe(const FOnsetAppearanceRecipe& Recipe, FString& Json);
    UFUNCTION(BlueprintCallable, Category="Onset|Appearance")
    static bool DecodeAndValidateRecipe(const FString& Json, const UOnsetAppearanceCatalog* Catalog,
        EOnsetAppearanceUse Use, FOnsetAppearanceRecipe& Recipe, TArray<FOnsetAppearanceIssue>& Issues);
};
