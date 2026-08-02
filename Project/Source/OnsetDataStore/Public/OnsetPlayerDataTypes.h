#pragma once

#include "CoreMinimal.h"
#include "OnsetPlayerDataTypes.generated.h"

UENUM(BlueprintType)
enum class EOnsetCharacterClass : uint8
{
	Tank    UMETA(DisplayName = "Tank"),
	Ranged  UMETA(DisplayName = "Ranged"),
	DPS     UMETA(DisplayName = "DPS"),
	Support UMETA(DisplayName = "Support")
};

USTRUCT(BlueprintType)
struct FOnsetCharacterAppearance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
	uint8 PresetIndex = 0;
};

USTRUCT(BlueprintType)
struct FOnsetCharacterSlotData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Account")
	int32 SlotIndex = -1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Account")
	FString CharacterName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Account")
	int32 Level = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Account")
	EOnsetCharacterClass CharacterClass = EOnsetCharacterClass::DPS;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Account")
	bool bOccupied = false;
};

USTRUCT(BlueprintType)
struct FOnsetAccountData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Account")
	FString PlatformID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Account")
	FString Platform;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Account")
	TArray<FOnsetCharacterSlotData> Slots;
};

USTRUCT(BlueprintType)
struct FOnsetFullCharacterData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	int32 SlotIndex = -1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	FString CharacterName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	int32 Level = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	int32 Experience = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	FString CurrentZone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	float SavedMaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	FVector SavedPosition = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	float SavedRotationYaw = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	FString InventoryJSON;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	FString EquipmentJSON;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	FString QuestsJSON;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	EOnsetCharacterClass CharacterClass = EOnsetCharacterClass::DPS;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character")
	FString AppearanceJSON;
};
