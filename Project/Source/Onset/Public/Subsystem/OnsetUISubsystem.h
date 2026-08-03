// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UI/OnsetActivatableWidgetStack.h"
#include "OnsetUISubsystem.generated.h"

class UOnsetRootLayout;
class UOnsetScreenBase;
class UOnsetLoadingScreen;

/**
 * Owns the root UI layout and exposes simple, Blueprint-callable navigation.
 * Nothing else in the project needs a direct widget reference to push or
 * pop a screen - gameplay code and Blueprints just call this subsystem.
 */
UCLASS()
class ONSET_API UOnsetUISubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Creates and adds WBP_RootLayout (or your subclass) to the viewport. Call once at boot. */
	UFUNCTION(BlueprintCallable, Category = "Onset|UI")
	void InitializeRootLayout(TSubclassOf<UOnsetRootLayout> RootLayoutClass, int32 ZOrder = 0);

	/** Pushes ScreenClass onto the given layer's stack and returns the created screen. */
	UFUNCTION(BlueprintCallable, Category = "Onset|UI")
	UOnsetScreenBase* PushScreen(EOnsetUILayer Layer, TSubclassOf<UOnsetScreenBase> ScreenClass);

	/** Pops the top-most screen off the given layer's stack, if any. */
	UFUNCTION(BlueprintCallable, Category = "Onset|UI")
	void PopScreen(EOnsetUILayer Layer);

	/** Removes the entire RootLayout from the viewport and clears the internal reference.
	 *  Call before traveling away from MainMenu to avoid carrying stale UI into the game world. */
	UFUNCTION(BlueprintCallable, Category = "Onset|UI")
	void CleanupUI();

	UFUNCTION(BlueprintPure, Category = "Onset|UI")
	UOnsetRootLayout* GetRootLayout() const { return RootLayout; }

	/** Shows the full-screen loading overlay, tearing down any menu UI first.
	 *  Safe to call multiple times while already showing. */
	UFUNCTION(BlueprintCallable, Category = "Onset|UI")
	void ShowLoadingScreen();

	/** Hides the loading overlay once the minimum display time has elapsed. */
	UFUNCTION(BlueprintCallable, Category = "Onset|UI")
	void HideLoadingScreen();

	/** True while the loading overlay is on screen. */
	UFUNCTION(BlueprintPure, Category = "Onset|UI")
	bool IsLoadingScreenVisible() const { return LoadingScreen != nullptr; }

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	UOnsetActivatableWidgetStack* GetStackForLayer(EOnsetUILayer Layer) const;

	/** Enforced when the travel/auth never completes, so the UI never hangs. */
	void OnLoadingTimeout();

	UPROPERTY(Transient)
	TObjectPtr<UOnsetRootLayout> RootLayout;

	UPROPERTY(Transient)
	TObjectPtr<UOnsetLoadingScreen> LoadingScreen;

	/** Blueprint subclass that provides the loading screen visuals. Resolved from [Onset.UI] LoadingScreenClass. */
	UPROPERTY(EditDefaultsOnly, Category = "Onset|UI")
	TSubclassOf<UOnsetLoadingScreen> LoadingScreenClass;

	UPROPERTY(Transient)
	FTimerHandle LoadingScreenTimerHandle;

	/** Seconds the loading screen stays visible at minimum to avoid a blink. */
	UPROPERTY(EditDefaultsOnly, Category = "Onset|UI")
	float MinLoadingScreenTime = 0.5f;

	/** Seconds before the loading screen force-hides and reports a timeout. */
	UPROPERTY(EditDefaultsOnly, Category = "Onset|UI")
	float LoadingScreenTimeoutSeconds = 10.0f;

	double LoadingScreenShowTime = 0.0;
};