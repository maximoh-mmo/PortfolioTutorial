#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "OnsetMenuHUD.generated.h"

UCLASS()
class ONSET_API AOnsetMenuHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	bool bInputEnabled = true;
};
