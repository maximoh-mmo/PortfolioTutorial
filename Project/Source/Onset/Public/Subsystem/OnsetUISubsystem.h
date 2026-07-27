// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UI/OnsetActivatableWidgetStack.h"
#include "OnsetUISubsystem.generated.h"

class UOnsetRootLayout;
class UOnsetScreenBase;

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

private:
	UOnsetActivatableWidgetStack* GetStackForLayer(EOnsetUILayer Layer) const;

	UPROPERTY(Transient)
	TObjectPtr<UOnsetRootLayout> RootLayout;
};