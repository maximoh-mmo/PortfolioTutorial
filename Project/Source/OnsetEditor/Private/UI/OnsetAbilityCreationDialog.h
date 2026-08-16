// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UAbilityCreationData;

/**
 * Modal dialog for authoring a new DT_Abilities row. Hosts a UDetailsView bound to a
 * UAbilityCreationData so the form (type-gated ranges, effect checkboxes) auto-generates.
 * Create/Cancel close the owner window; AddDefinition reads the result afterwards.
 */
class SAbilityCreationDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAbilityCreationDialog) {}
	SLATE_END_ARGS()

	/** Builds the dialog around the given form-data object. */
	void Construct(const FArguments& InArgs, UAbilityCreationData* InData);

	/** Sets the owner window so Create/Cancel can close it. */
	void SetOwnerWindow(const TWeakPtr<SWindow>& InOwnerWindow);

	/** True if the user confirmed with Create. */
	bool ShouldCreate() const { return bCreate; }

	/** The form data (read after the modal loop). */
	UAbilityCreationData* GetData() const { return Data; }

private:
	FReply OnCreate();
	FReply OnCancel();
	void CloseOwnerWindow();

	UAbilityCreationData* Data = nullptr;
	TWeakPtr<SWindow> OwnerWindow;
	bool bCreate = false;
};