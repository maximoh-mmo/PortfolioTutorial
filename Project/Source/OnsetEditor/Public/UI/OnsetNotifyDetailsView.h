// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/DetailsView.h"
#include "OnsetNotifyDetailsView.generated.h"

/**
 * UDetailsView that broadcasts OnPropertyEdited whenever the user changes any
 * property in the bound object. The editor widgets listen to this instead of
 * relying on a Save button: any edit marks the row dirty so it can be committed
 * to the DataTable and persisted when the editor window closes.
 */
UCLASS(Transient)
class UOnsetNotifyDetailsView : public UDetailsView
{
	GENERATED_BODY()

public:
	/** Fired after the user changes a property in the bound object. */
	FSimpleMulticastDelegate OnPropertyEdited;

protected:
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;
};
