// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "OnsetScreenBase.generated.h"

class USoundBase;

/**
 * Base class for every full-screen UI "screen" in Onset
 * (Main Menu, Login, Character Select, Character Creation, ...).
 *
 * Individual screens should derive from this in Blueprint (or a further
 * C++ subclass) and only need to implement their own content - shared
 * activation/deactivation behavior lives here once.
 */
UCLASS(Abstract)
class ONSET_API UOnsetScreenBase : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UOnsetScreenBase(const FObjectInitializer& ObjectInitializer);

protected:
	//~ Begin UCommonActivatableWidget interface
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	//~ End UCommonActivatableWidget interface

	/** Fired after the screen has activated. Hook per-screen setup here in Blueprint. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Onset|Screen", meta = (DisplayName = "On Screen Activated"))
	void BP_OnScreenActivated();

	/** Fired right before the screen deactivates. Hook per-screen teardown here in Blueprint. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Onset|Screen", meta = (DisplayName = "On Screen Deactivated"))
	void BP_OnScreenDeactivated();

	/** Ambient music to play while this screen is active. Leave unset to keep whatever is already playing. */
	UPROPERTY(EditDefaultsOnly, Category = "Onset|Screen|Audio")
	TObjectPtr<USoundBase> AmbientMusicCue;

	/** Whether this screen should request "menu" style input (mouse free, UI-focused) while active. */
	UPROPERTY(EditDefaultsOnly, Category = "Onset|Screen|Input")
	bool bUseMenuInputMode = true;
};