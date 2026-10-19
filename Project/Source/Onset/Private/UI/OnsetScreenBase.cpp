#include "UI/OnsetScreenBase.h"
#include "CommonButtonBase.h"
#include "CommonInputModeTypes.h"
#include "Components/PanelWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
 
UOnsetScreenBase::UOnsetScreenBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsBackHandler = true;
	bAutoRestoreFocus = true;
	SetConsumePointerInput(true);
}
 
void UOnsetScreenBase::NativeOnActivated()
{
	Super::NativeOnActivated();
 
	if (AmbientMusicCue)
	{
		UGameplayStatics::PlaySound2D(this, AmbientMusicCue);
	}
 
	BP_OnScreenActivated();
}
 
void UOnsetScreenBase::NativeOnDeactivated()
{
	BP_OnScreenDeactivated();
 
	Super::NativeOnDeactivated();
}
 
TOptional<FUIInputConfig> UOnsetScreenBase::GetDesiredInputConfig() const
{
	if (bUseMenuInputMode)
	{
		return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
	}
 
	return Super::GetDesiredInputConfig();
}

UWidget* UOnsetScreenBase::NativeGetDesiredFocusTarget() const
{
	if (UWidget* ExplicitFocus = GetDesiredFocusWidget())
	{
		const ESlateVisibility Vis = ExplicitFocus->GetVisibility();
		if (Vis != ESlateVisibility::Collapsed && Vis != ESlateVisibility::Hidden && ExplicitFocus->GetIsEnabled())
		{
			return ExplicitFocus;
		}
	}

	if (WidgetTree && WidgetTree->RootWidget)
	{
		if (UWidget* Found = FindFirstFocusableChild(WidgetTree->RootWidget))
		{
			return Found;
		}
	}

	return Super::NativeGetDesiredFocusTarget();
}

UWidget* UOnsetScreenBase::FindFirstFocusableChild(UWidget* Root) const
{
	if (!Root)
	{
		return nullptr;
	}

	if (UCommonButtonBase* Button = Cast<UCommonButtonBase>(Root))
	{
		const ESlateVisibility Vis = Button->GetVisibility();
		if (Vis != ESlateVisibility::Collapsed && Vis != ESlateVisibility::Hidden && Button->GetIsEnabled() && Button->GetIsFocusable())
		{
			return Button;
		}
	}

	if (UPanelWidget* Panel = Cast<UPanelWidget>(Root))
	{
		const int32 ChildCount = Panel->GetChildrenCount();
		for (int32 Index = 0; Index < ChildCount; ++Index)
		{
			if (UWidget* Found = FindFirstFocusableChild(Panel->GetChildAt(Index)))
			{
				return Found;
			}
		}
	}
	else if (UUserWidget* UserWidget = Cast<UUserWidget>(Root))
	{
		if (UserWidget->WidgetTree && UserWidget->WidgetTree->RootWidget)
		{
			if (UWidget* Found = FindFirstFocusableChild(UserWidget->WidgetTree->RootWidget))
			{
				return Found;
			}
		}
	}

	return nullptr;
}

bool UOnsetScreenBase::NativeOnHandleBackAction()
{
	if (Super::NativeOnHandleBackAction())
	{
		return true;
	}

	if (UCommonButtonBase* BackBtn = Cast<UCommonButtonBase>(GetWidgetFromName(TEXT("BackButton"))))
	{
		const ESlateVisibility Vis = BackBtn->GetVisibility();
		if (Vis != ESlateVisibility::Collapsed && Vis != ESlateVisibility::Hidden && BackBtn->GetIsEnabled())
		{
			BackBtn->OnClicked().Broadcast();
			return true;
		}
	}

	DeactivateWidget();
	return true;
}
 