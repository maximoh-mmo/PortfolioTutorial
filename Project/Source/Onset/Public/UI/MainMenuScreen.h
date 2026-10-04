#pragma once

#include "CoreMinimal.h"
#include "CharacterSelectScreen.h"
#include "OnsetScreenBase.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuScreen.generated.h"

class AOnsetPlayerController;
class UCommonButtonBase;

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

	/** Disabled until Client_AccountData confirms that server-side authentication completed. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonButtonBase> ConnectButton;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Menu")                                     
	TSubclassOf<UCharacterSelectScreen> CharacterSelectScreenClass;    

private:
	void RefreshConnectAvailability();

	UFUNCTION()
	void HandleAccountDataChanged();

	TObjectPtr<AOnsetPlayerController> BoundPlayerController;
};
