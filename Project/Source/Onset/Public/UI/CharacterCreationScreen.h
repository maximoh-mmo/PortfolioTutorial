#pragma once

#include "CoreMinimal.h"
#include "OnsetPlayerDataTypes.h"
#include "OnsetScreenBase.h"
#include "CharacterCreationScreen.generated.h"

class AOnsetPlayerController;

UCLASS(Abstract)
class ONSET_API UCharacterCreationScreen : public UOnsetScreenBase
{
	GENERATED_BODY()

public:
	void SetPlayerController(AOnsetPlayerController* PlayerController);
	void SetSlotIndex(int32 InSlotIndex);

	UFUNCTION(BlueprintCallable)
	void CreateCharacter(const FString& CharacterName, EOnsetCharacterClass CharacterClass, int32 AppearancePresetIndex);

protected:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AOnsetPlayerController> CachedPlayerController;

	UPROPERTY(BlueprintReadOnly)
	int32 SlotIndex = -1;
};
