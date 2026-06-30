#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Data/OnsetPlayerDataTypes.h"
#include "OnsetLobbyHUD.generated.h"

UCLASS()
class ONSET_API AOnsetLobbyHUD : public AHUD
{
	GENERATED_BODY()

public:
	void ShowAccountData(const FOnsetAccountData& InAccountData);

protected:
	virtual void DrawHUD() override;

private:
	FOnsetAccountData AccountData;
	bool bHasAccountData = false;
};
