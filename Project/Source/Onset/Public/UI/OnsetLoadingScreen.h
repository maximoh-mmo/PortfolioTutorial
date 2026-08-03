#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OnsetLoadingScreen.generated.h"

/**
 * Full-screen overlay shown while the client travels to another server/world.
 *
 * All visuals live in a Blueprint subclass (WBP_LoadingScreen); this class only
 * exposes lifecycle hooks for intro/outro animation and failure handling.
 */
UCLASS(Abstract)
class ONSET_API UOnsetLoadingScreen : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Called when the loading screen is shown. Play intro animation here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Onset|LoadingScreen", meta = (DisplayName = "On Loading Screen Shown"))
	void BP_OnLoadingScreenShown();

	/** Called when the loading screen is hidden. Play outro animation here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Onset|LoadingScreen", meta = (DisplayName = "On Loading Screen Hidden"))
	void BP_OnLoadingScreenHidden();

	/** Called if the loading screen times out (travel/auth failed). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Onset|LoadingScreen", meta = (DisplayName = "On Loading Screen Timeout"))
	void BP_OnLoadingTimeout();
};
