// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/MHTerminalTextInputController.h"

#include "Blueprint/UserWidget.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableText.h"
#include "Engine/World.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "TimerManager.h"

void UMHTerminalInputController::Initialize(
	UUserWidget* InOwnerWidget,
	UEditableText* InInputField,
	UCommonTextBlock* InDisplayTextBlock,
	UCommonTextBlock* InCursorTextBlock,
	UBorder* InDefaultBorder,
	UBorder* InCursorBorder,
	const FString& InPlaceholderText,
	float InBlinkInterval,
	int32 InMaxCharacters)
{
	OwnerWidget = InOwnerWidget;
	InputField = InInputField;
	DisplayTextBlock = InDisplayTextBlock;
	CursorTextBlock = InCursorTextBlock;
	DefaultBorder = InDefaultBorder;
	CursorBorder = InCursorBorder;
	PlaceholderText = InPlaceholderText;
	BlinkInterval = FMath::Max(0.1f, InBlinkInterval);
	MaxCharacters = InMaxCharacters;
	bIsPlaceholderActive = true;
	bBlinkVisible = true;
	bIsFocused = false;

	if (!OwnerWidget.IsValid() || !InputField.IsValid() || !DisplayTextBlock.IsValid())
	{
		return;
	}

	// 1. Detach old Blueprint event bindings from the input field to prevent Blueprint logic from hiding widgets
	InputField->OnTextChanged.Clear();
	InputField->OnTextCommitted.Clear();

	// 2. Hide InputField's visual text and native Slate caret so only our styled terminal presentation is visible
	InputField->SetRenderOpacity(0.0f);
	FEditableTextStyle Style = InputField->WidgetStyle;
	Style.SetColorAndOpacity(FSlateColor(FLinearColor::Transparent));
	Style.CaretImage.DrawAs = ESlateBrushDrawType::NoDrawType;
	Style.CaretImage.TintColor = FSlateColor(FLinearColor::Transparent);
	InputField->SetWidgetStyle(Style);
	InputField->SetText(FText::GetEmpty());

	// 3. Bind dynamic delegates for text input
	InputField->OnTextChanged.AddDynamic(this, &UMHTerminalInputController::HandleTextChanged);
	InputField->OnTextCommitted.AddDynamic(this, &UMHTerminalInputController::HandleTextCommitted);

	// 4. Capture original text styling
	PlaceholderColor = DisplayTextBlock->GetColorAndOpacity().GetSpecifiedColor();
	if (PlaceholderColor.A <= 0.05f)
	{
		PlaceholderColor = FLinearColor(1.f, 1.f, 1.f, 0.6f);
	}
	ActiveColor = FLinearColor(1.f, 1.f, 1.f, 1.0f);

	// 5. Ensure DefaultBorder is HitTestInvisible so mouse clicks pass through to InputField beneath it
	if (DefaultBorder.IsValid())
	{
		DefaultBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	// 6. Capture original CursorBorder Y position and ensure it is HitTestInvisible
	if (CursorBorder.IsValid())
	{
		CursorBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UCanvasPanelSlot* CursorSlot = Cast<UCanvasPanelSlot>(CursorBorder->Slot))
		{
			CursorPosY = CursorSlot->GetPosition().Y;
			CursorSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		}
	}

	// 7. Initialize CursorTextBlock: permanently holds '_' and only changes RenderOpacity on blink
	if (CursorTextBlock.IsValid())
	{
		CursorTextBlock->SetText(FText::FromString(CursorChar));
		CursorTextBlock->SetRenderOpacity(1.0f);
		CursorTextBlock->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	// 8. Set initial display state and position cursor
	UpdateDisplayPresentation();

	// 9. Start blink timer
	if (UWorld* World = OwnerWidget->GetWorld())
	{
		World->GetTimerManager().SetTimer(BlinkTimerHandle, this, &UMHTerminalInputController::HandleBlinkTimer, BlinkInterval, true);
	}
}

void UMHTerminalInputController::Shutdown()
{
	if (OwnerWidget.IsValid())
	{
		if (UWorld* World = OwnerWidget->GetWorld())
		{
			World->GetTimerManager().ClearTimer(BlinkTimerHandle);
		}
	}

	if (InputField.IsValid())
	{
		InputField->OnTextChanged.RemoveDynamic(this, &UMHTerminalInputController::HandleTextChanged);
		InputField->OnTextCommitted.RemoveDynamic(this, &UMHTerminalInputController::HandleTextCommitted);
	}

	OwnerWidget = nullptr;
	InputField = nullptr;
	DisplayTextBlock = nullptr;
	CursorTextBlock = nullptr;
	DefaultBorder = nullptr;
	CursorBorder = nullptr;
}

void UMHTerminalInputController::SetPlaceholderText(const FString& NewPlaceholder)
{
	PlaceholderText = NewPlaceholder;
	if (bIsPlaceholderActive)
	{
		UpdateDisplayPresentation();
		OnTerminalTextChanged.Broadcast(GetFinalText(), true);
	}
}

FString UMHTerminalInputController::GetFinalText() const
{
	if (bIsPlaceholderActive || !InputField.IsValid())
	{
		return PlaceholderText;
	}

	FString Typed = InputField->GetText().ToString().TrimStartAndEnd();
	return Typed.IsEmpty() ? PlaceholderText : Typed;
}

void UMHTerminalInputController::OpenVirtualKeyboard()
{
	if (InputField.IsValid())
	{
		InputField->SetFocus();
		InputField->ToggleVirtualKeyboard(true);
	}
}

void UMHTerminalInputController::SetFocused(bool bInFocused)
{
	bIsFocused = bInFocused;
	if (bIsFocused)
	{
		ResetBlink();
	}
}

void UMHTerminalInputController::HandleTextChanged(const FText& InText)
{
	FString Raw = InText.ToString();

	// Enforce max character limit
	if (MaxCharacters > 0 && Raw.Len() > MaxCharacters)
	{
		Raw = Raw.Left(MaxCharacters);
		if (InputField.IsValid())
		{
			InputField->SetText(FText::FromString(Raw));
		}
	}

	if (Raw.IsEmpty())
	{
		// All characters backspaced to empty -> restore placeholder
		bIsPlaceholderActive = true;
	}
	else
	{
		// Active characters entered -> placeholder cleared
		bIsPlaceholderActive = false;
	}

	ResetBlink();
	UpdateDisplayPresentation();
	OnTerminalTextChanged.Broadcast(GetFinalText(), bIsPlaceholderActive);
}

void UMHTerminalInputController::HandleTextCommitted(const FText& InText, ETextCommit::Type CommitMethod)
{
	FString Raw = InText.ToString().TrimStartAndEnd();

	if (Raw.IsEmpty())
	{
		bIsPlaceholderActive = true;
		if (InputField.IsValid())
		{
			InputField->SetText(FText::GetEmpty());
		}
	}
	else
	{
		bIsPlaceholderActive = false;
		if (InputField.IsValid())
		{
			InputField->SetText(FText::FromString(Raw));
		}
	}

	ResetBlink();
	UpdateDisplayPresentation();
	OnTerminalTextChanged.Broadcast(GetFinalText(), bIsPlaceholderActive);
}

void UMHTerminalInputController::HandleBlinkTimer()
{
	bBlinkVisible = !bBlinkVisible;
	if (CursorTextBlock.IsValid())
	{
		CursorTextBlock->SetRenderOpacity(bBlinkVisible ? 1.0f : 0.0f);
	}
}

void UMHTerminalInputController::ResetBlink()
{
	bBlinkVisible = true;
	if (CursorTextBlock.IsValid())
	{
		CursorTextBlock->SetRenderOpacity(1.0f);
	}

	if (OwnerWidget.IsValid())
	{
		if (UWorld* World = OwnerWidget->GetWorld())
		{
			World->GetTimerManager().SetTimer(BlinkTimerHandle, this, &UMHTerminalInputController::HandleBlinkTimer, BlinkInterval, true);
		}
	}
}

float UMHTerminalInputController::CalculateTextWidth(const FString& InStringToMeasure) const
{
	if (!DisplayTextBlock.IsValid() || InStringToMeasure.IsEmpty())
	{
		return 0.0f;
	}

	if (FSlateApplication::IsInitialized())
	{
		const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		return static_cast<float>(FontMeasure->Measure(InStringToMeasure, DisplayTextBlock->GetFont()).X);
	}

	return InStringToMeasure.Len() * 14.0f;
}

void UMHTerminalInputController::UpdateCursorPosition()
{
	if (!DisplayTextBlock.IsValid() || !CursorBorder.IsValid())
	{
		return;
	}

	const FString CurrentString = bIsPlaceholderActive ? PlaceholderText : (InputField.IsValid() ? InputField->GetText().ToString() : TEXT(""));
	const float TextWidth = CalculateTextWidth(CurrentString);

	if (UCanvasPanelSlot* CursorSlot = Cast<UCanvasPanelSlot>(CursorBorder->Slot))
	{
		CursorSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		CursorSlot->SetPosition(FVector2D((TextWidth / 2.0f) + CursorPadding, CursorPosY));
	}
}

void UMHTerminalInputController::UpdateDisplayPresentation()
{
	if (DisplayTextBlock.IsValid())
	{
		if (bIsPlaceholderActive)
		{
			DisplayTextBlock->SetText(FText::FromString(PlaceholderText));
			DisplayTextBlock->SetColorAndOpacity(FSlateColor(PlaceholderColor));
		}
		else
		{
			FString Current = InputField.IsValid() ? InputField->GetText().ToString() : TEXT("");
			DisplayTextBlock->SetText(FText::FromString(Current));
			DisplayTextBlock->SetColorAndOpacity(FSlateColor(ActiveColor));
		}
	}

	if (CursorTextBlock.IsValid())
	{
		CursorTextBlock->SetText(FText::FromString(CursorChar));
		CursorTextBlock->SetRenderOpacity(bBlinkVisible ? 1.0f : 0.0f);
		CursorTextBlock->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	UpdateCursorPosition();
}
