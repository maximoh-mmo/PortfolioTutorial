// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/OnsetNotifyDetailsView.h"

void UOnsetNotifyDetailsView::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	Super::NotifyPostChange(PropertyChangedEvent, PropertyThatChanged);
	OnPropertyEdited.Broadcast();
}
