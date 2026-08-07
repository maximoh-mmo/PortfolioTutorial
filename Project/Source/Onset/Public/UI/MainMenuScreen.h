#pragma once

#include "CoreMinimal.h"
#include "CharacterSelectScreen.h"
#include "OnsetScreenBase.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuScreen.generated.h"

class UButton;

UCLASS()
class ONSET_API UMainMenuScreen : public UOnsetScreenBase
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Onset|Menu")
	void ConnectToServer() const;
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Menu")                                     
	TSubclassOf<UCharacterSelectScreen> CharacterSelectScreenClass;    
};
