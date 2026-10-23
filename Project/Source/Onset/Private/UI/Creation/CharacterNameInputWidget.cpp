// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Creation/CharacterNameInputWidget.h"
#include "UI/MHTerminalTextInputController.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/EditableText.h"
#include "Input/Events.h"

void UCharacterNameInputWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (InputField && DisplayText)
	{
		if (!TerminalController)
		{
			TerminalController = NewObject<UMHTerminalInputController>(this);
		}

		TerminalController->OnTerminalTextChanged.RemoveDynamic(this, &ThisClass::HandleTerminalTextChanged);
		TerminalController->OnTerminalTextChanged.AddDynamic(this, &ThisClass::HandleTerminalTextChanged);

		const FString InitialPlaceholder = DefaultName.IsEmpty() ? TEXT("Kael") : DefaultName;
		TerminalController->Initialize(
			this,
			InputField,
			DisplayText,
			CursorText,
			DefaultBorder,
			CursorBorder,
			InitialPlaceholder,
			BlinkInterval,
			MaxCharacters
		);

		UpdateCharCount(TerminalController->GetFinalText().TrimStartAndEnd().Len());
	}
}

void UCharacterNameInputWidget::NativeDestruct()
{
	if (TerminalController)
	{
		TerminalController->OnTerminalTextChanged.RemoveDynamic(this, &ThisClass::HandleTerminalTextChanged);
		TerminalController->Shutdown();
		TerminalController = nullptr;
	}

	Super::NativeDestruct();
}

FReply UCharacterNameInputWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Bottom)
	{
		APlayerController* PC = GetOwningPlayer();
		if (InputField && InputField->HasUserFocus(PC))
		{
			if (TerminalController)
			{
				TerminalController->OpenVirtualKeyboard();
				return FReply::Handled();
			}
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UCharacterNameInputWidget::HandleTerminalTextChanged(const FString& NewText, bool bIsPlaceholder)
{
	const FString Trimmed = NewText.TrimStartAndEnd();
	UpdateCharCount(Trimmed.Len());

	const bool bValid = Trimmed.Len() >= MinCharacters && Trimmed.Len() <= MaxCharacters;
	OnNameChanged.Broadcast(Trimmed, bValid);
}

void UCharacterNameInputWidget::UpdateCharCount(int32 CurrentLen)
{
	if (CharCountText)
	{
		CharCountText->SetText(FText::Format(
			NSLOCTEXT("OnsetCreation", "CharCountFormat", "{0}/{1}"),
			FText::AsNumber(CurrentLen),
			FText::AsNumber(MaxCharacters)
		));
	}
}

FString UCharacterNameInputWidget::GetTrimmedName() const
{
	if (TerminalController)
	{
		return TerminalController->GetFinalText().TrimStartAndEnd();
	}

	if (InputField)
	{
		FString Text = InputField->GetText().ToString().TrimStartAndEnd();
		if (Text.IsEmpty())
		{
			Text = DefaultName;
		}
		return Text;
	}

	return DefaultName;
}

void UCharacterNameInputWidget::SetDefaultName(const FString& InDefaultName)
{
	DefaultName = InDefaultName;
	if (TerminalController)
	{
		TerminalController->SetPlaceholderText(InDefaultName);
	}
}

bool UCharacterNameInputWidget::IsNameValid() const
{
	const FString Name = GetTrimmedName();
	return Name.Len() >= MinCharacters && Name.Len() <= MaxCharacters;
}
