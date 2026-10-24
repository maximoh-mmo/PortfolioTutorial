#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "OnsetAnimationProfile.h"
#include "OnsetAppearanceTypes.h"
#include "OnsetAuthoredAppearance.generated.h"

class UTexture2D;

/**
 * A master authored character appearance asset in the Onset Appearance Library.
 * Stores full modular mesh assembly, color/material parameters, rank insignia,
 * and a linked animation profile.
 */
UCLASS(BlueprintType)
class ONSETAPPEARANCE_API UOnsetAuthoredAppearance : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Unique identifier for this appearance asset (e.g. App_CadreInstructor_M). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance Info")
	FName AppearanceId;

	/** Human-readable display name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance Info")
	FText DisplayName;

	/** Target role / category. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance Info")
	EOnsetAppearanceCategory Category = EOnsetAppearanceCategory::NPC;

	/** Faction or Branch affiliation tag (e.g. Military, Governance, Substrate, Logistics, SpecialOps). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance Info")
	FName FactionAffiliation = TEXT("Military");

	/** Visual rank insignia or ceremonial rank badge ID (e.g. SeniorOfficer, TribuneOverseer). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance Info")
	FName RankInsignia = TEXT("Civilian");

	/** The full modular appearance recipe (Body mesh, Head mesh, morph targets, colors, modular parts). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Modular Recipe")
	FOnsetAppearanceRecipe Recipe;

	/** Linked animation profile driving idle postures, fidgets, talk gestures, and salutes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TSoftObjectPtr<UOnsetAnimationProfile> AnimationProfile;

	/** Optional pre-rendered or captured thumbnail icon for the library catalog. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance Info")
	TSoftObjectPtr<UTexture2D> Thumbnail;

	/** Descriptive tags for search and categorization (e.g. "HeavyArmor", "Academy", "Cadre"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance Info")
	TArray<FName> Tags;

	/** Designer / lore notes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance Info", meta = (MultiLine = "true"))
	FString LoreNotes;
};
