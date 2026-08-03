// Fill out your copyright notice in the Description page of Project Settings.

#include "Subsystem/OnsetUISubsystem.h"
#include "UI/OnsetRootLayout.h"
#include "UI/OnsetScreenBase.h"
#include "UI/OnsetLoadingScreen.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/ConfigCacheIni.h"

void UOnsetUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FString LoadingClassPath;
	if (GConfig->GetString(TEXT("Onset.UI"), TEXT("LoadingScreenClass"), LoadingClassPath, GEngineIni) && !LoadingClassPath.IsEmpty())
	{
		if (UClass* FoundClass = LoadClass<UOnsetLoadingScreen>(nullptr, *LoadingClassPath))
		{
			LoadingScreenClass = FoundClass;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UOnsetUISubsystem: could not load [Onset.UI] LoadingScreenClass '%s'"), *LoadingClassPath);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UOnsetUISubsystem: [Onset.UI] LoadingScreenClass not configured - loading screen disabled"));
	}
}

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

void UOnsetUISubsystem::CleanupUI()
{
	if (RootLayout)
	{
		RootLayout->RemoveFromParent();
		RootLayout = nullptr;
	}
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

void UOnsetUISubsystem::ShowLoadingScreen()
{
	if (LoadingScreen)
	{
		return;
	}

	// Strip any menu screens so nothing flashes underneath the overlay.
	CleanupUI();

	LoadingScreen = CreateWidget<UOnsetLoadingScreen>(GetGameInstance(), LoadingScreenClass);
	if (!LoadingScreen)
	{
		UE_LOG(LogTemp, Warning, TEXT("UOnsetUISubsystem: no loading screen class - traveling without a visual overlay"));
		return;
	}

	LoadingScreen->AddToViewport(1000);
	LoadingScreen->BP_OnLoadingScreenShown();

	LoadingScreenShowTime = FPlatformTime::Seconds();

	if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeUIOnly());
	}

	if (LoadingScreenTimeoutSeconds > 0.0f)
	{
		GetGameInstance()->GetTimerManager().SetTimer(
			LoadingScreenTimerHandle, this, &UOnsetUISubsystem::OnLoadingTimeout,
			LoadingScreenTimeoutSeconds, false);
	}
}

void UOnsetUISubsystem::HideLoadingScreen()
{
	if (!LoadingScreen)
	{
		return;
	}

	const float Elapsed = static_cast<float>(FPlatformTime::Seconds() - LoadingScreenShowTime);
	if (Elapsed < MinLoadingScreenTime)
	{
		// Defer until the minimum display time has elapsed so the transition never blinks.
		GetGameInstance()->GetTimerManager().SetTimer(
			LoadingScreenTimerHandle, this, &UOnsetUISubsystem::HideLoadingScreen,
			MinLoadingScreenTime - Elapsed, false);
		return;
	}

	GetGameInstance()->GetTimerManager().ClearTimer(LoadingScreenTimerHandle);

	LoadingScreen->BP_OnLoadingScreenHidden();
	LoadingScreen->RemoveFromParent();
	LoadingScreen = nullptr;

	if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeGameAndUI());
	}
}

void UOnsetUISubsystem::OnLoadingTimeout()
{
	if (!LoadingScreen)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("UOnsetUISubsystem: loading screen timed out after %.1fs"), LoadingScreenTimeoutSeconds);
	LoadingScreen->BP_OnLoadingTimeout();
	HideLoadingScreen();
}

UOnsetActivatableWidgetStack* UOnsetUISubsystem::GetStackForLayer(EOnsetUILayer Layer) const
{
	return RootLayout ? RootLayout->GetStackForLayer(Layer) : nullptr;
}
