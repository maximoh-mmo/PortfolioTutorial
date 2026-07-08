// Fill out your copyright notice in the Description page of Project Settings.

#include "Subsystem/OnsetUISubsystem.h"
#include "UI/OnsetRootLayout.h"
#include "UI/OnsetScreenBase.h"
#include "Blueprint/UserWidget.h"

void UOnsetUISubsystem::InitializeRootLayout(TSubclassOf<UOnsetRootLayout> RootLayoutClass, int32 ZOrder)
{
	if (RootLayout || !RootLayoutClass)
	{
		return;
	}

	RootLayout = CreateWidget<UOnsetRootLayout>(GetGameInstance(), RootLayoutClass);
	if (RootLayout)
	{
		RootLayout->AddToViewport(ZOrder);
	}
}

UOnsetScreenBase* UOnsetUISubsystem::PushScreen(EOnsetUILayer Layer, TSubclassOf<UOnsetScreenBase> ScreenClass)
{
	if (UOnsetActivatableWidgetStack* Stack = GetStackForLayer(Layer))
	{
		return Cast<UOnsetScreenBase>(Stack->AddWidget(ScreenClass));
	}

	return nullptr;
}

void UOnsetUISubsystem::PopScreen(EOnsetUILayer Layer)
{
	if (UOnsetActivatableWidgetStack* Stack = GetStackForLayer(Layer))
	{
		if (UCommonActivatableWidget* ActiveWidget = Stack->GetActiveWidget())
		{
			Stack->RemoveWidget(*ActiveWidget);
		}
	}
}

UOnsetActivatableWidgetStack* UOnsetUISubsystem::GetStackForLayer(EOnsetUILayer Layer) const
{
	return RootLayout ? RootLayout->GetStackForLayer(Layer) : nullptr;
}