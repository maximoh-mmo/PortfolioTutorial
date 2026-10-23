// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Creation/OnsetColorWheel.h"
#include "Widgets/Colors/SColorWheel.h"
#include "Components/SlateWrapperTypes.h"

UOnsetColorWheel::UOnsetColorWheel()
	: SelectedColor(FLinearColor::White)
{
}

TSharedRef<SWidget> UOnsetColorWheel::RebuildWidget()
{
	MyColorWheel = SNew(SColorWheel)
		.SelectedColor(BIND_UOBJECT_ATTRIBUTE(FLinearColor, GetCurrentColor))
		.OnValueChanged(BIND_UOBJECT_DELEGATE(FOnLinearColorValueChanged, HandleOnValueChanged));

	return MyColorWheel.ToSharedRef();
}

void UOnsetColorWheel::SynchronizeProperties()
{
	Super::SynchronizeProperties();
}

void UOnsetColorWheel::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyColorWheel.Reset();
}

void UOnsetColorWheel::SetSelectedColor(FLinearColor InColor)
{
	SelectedColor = InColor;
}

void UOnsetColorWheel::HandleOnValueChanged(FLinearColor InColor)
{
	SelectedColor = InColor;
	OnColorChanged.Broadcast(InColor);
}

#if WITH_EDITOR
const FText UOnsetColorWheel::GetPaletteCategory()
{
	return NSLOCTEXT("OnsetUI", "PaletteCategory", "Onset UI");
}
#endif
