#pragma once

#include "CoreMinimal.h"
#include "OnsetPlayerDataTypes.generated.h"

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
};
