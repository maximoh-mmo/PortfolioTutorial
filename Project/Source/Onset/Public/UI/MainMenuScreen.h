#pragma once

#include "CoreMinimal.h"
#include "CharacterSelectScreen.h"
#include "OnsetScreenBase.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuScreen.generated.h"

class AOnsetPlayerController;

UCLASS()
class ONSET_API UMainMenuScreen : public UOnsetScreenBase
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Onset|Menu")
	void ConnectToServer() const;

	/** True once the server has accepted Steam packet authentication for this connection. */
	UFUNCTION(BlueprintPure, Category = "Onset|Menu")
	bool CanConnectToServer() const;
	
protected:
	virtual void NativeOnActivated() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Menu")                                     
	TSubclassOf<UCharacterSelectScreen> CharacterSelectScreenClass;    

private:
	void RefreshConnectAvailability();

	UFUNCTION()
	void HandleAccountDataChanged();

	TWeakObjectPtr<AOnsetPlayerController> BoundPlayerController;
};
