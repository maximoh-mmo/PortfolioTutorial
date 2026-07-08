#pragma once

#include "CoreMinimal.h"
#include "OnsetGameModeBase.h"
#include "UI/OnsetRootLayout.h"
#include "UI/OnsetScreenBase.h"
#include "UObject/ConstructorHelpers.h"
#include "OnsetMenuGameMode.generated.h"

UCLASS()
class ONSET_API AOnsetMenuGameMode : public AOnsetGameModeBase
{
	GENERATED_BODY()

public:
	AOnsetMenuGameMode();
	
	virtual void StartPlay() override;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UOnsetRootLayout> RootLayoutClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UOnsetScreenBase> MainMenuScreenClass;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UOnsetScreenBase> CharacterSelectScreenClass;
};
