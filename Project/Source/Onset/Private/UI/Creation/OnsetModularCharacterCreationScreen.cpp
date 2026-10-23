// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Creation/OnsetModularCharacterCreationScreen.h"
#include "UI/Creation/CharacterClassPickerWidget.h"
#include "UI/Creation/CharacterNameInputWidget.h"
#include "UI/Creation/CharacterCreationStageWidget.h"
#include "UI/Creation/CharacterCustomizerWidget.h"
#include "UI/Creation/CharacterCreationFooterWidget.h"
#include "Player/OnsetPlayerController.h"
#include "Player/OnsetCharacterAppearanceComponent.h"
#include "OnsetAppearanceCatalog.h"
#include "OnsetAppearanceLibrary.h"
#include "CommonButtonBase.h"

void UOnsetModularCharacterCreationScreen::SetPlayerController(AOnsetPlayerController* Controller)
{
	if (CachedPlayerController == Controller)
	{
		return;
	}

	if (CachedPlayerController)
	{
		CachedPlayerController->OnCharacterAppearanceResult.RemoveDynamic(this, &ThisClass::HandleAppearanceResult);
	}

	CachedPlayerController = Controller;

	if (Controller)
	{
		Controller->OnCharacterAppearanceResult.AddDynamic(this, &ThisClass::HandleAppearanceResult);
	}
}

void UOnsetModularCharacterCreationScreen::SetSlotIndex(int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;
}

void UOnsetModularCharacterCreationScreen::EnsureDraft()
{
	if (bDraftReady)
	{
		return;
	}

	AppearanceCatalog = UOnsetCharacterAppearanceComponent::LoadDefaultCatalog();
	FOnsetAppearanceRecipe Preset;
	TArray<FOnsetAppearanceIssue> Issues;

	if (AppearanceCatalog && UOnsetCharacterAppearanceComponent::GetPlayerPreset(0, Preset)
		&& AppearanceCatalog->ValidateRecipe(Preset, EOnsetAppearanceUse::Player, Issues))
	{
		DraftAppearance = MoveTemp(Preset);
		bDraftReady = true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Modular Character Creation: Default preset unavailable or invalid: %s"),
			Issues.IsEmpty() ? TEXT("Catalog/preset load failure") : *Issues[0].Message);
	}
}

void UOnsetModularCharacterCreationScreen::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (!CachedPlayerController)
	{
		SetPlayerController(GetOwningPlayer<AOnsetPlayerController>());
	}

	EnsureDraft();

	if (StageWidget)
	{
		StageWidget->SetAppearanceRecipe(DraftAppearance);
	}

	if (Customizer)
	{
		Customizer->InitializeCustomizer(AppearanceCatalog, DraftAppearance);
		Customizer->OnAppearanceDraftChanged.RemoveDynamic(this, &ThisClass::HandleAppearanceDraftChanged);
		Customizer->OnAppearanceDraftChanged.AddDynamic(this, &ThisClass::HandleAppearanceDraftChanged);

		Customizer->OnCameraFramingRequested.RemoveDynamic(this, &ThisClass::HandleCameraFramingRequested);
		Customizer->OnCameraFramingRequested.AddDynamic(this, &ThisClass::HandleCameraFramingRequested);
	}

	if (NameInput)
	{
		NameInput->SetDefaultName(DefaultCharacterName);
		NameInput->OnNameChanged.RemoveDynamic(this, &ThisClass::HandleNameChanged);
		NameInput->OnNameChanged.AddDynamic(this, &ThisClass::HandleNameChanged);
	}

	if (ClassPicker)
	{
		ClassPicker->OnClassSelected.RemoveDynamic(this, &ThisClass::HandleClassSelected);
		ClassPicker->OnClassSelected.AddDynamic(this, &ThisClass::HandleClassSelected);
	}

	if (Footer)
	{
		Footer->OnCreateClicked.RemoveDynamic(this, &ThisClass::HandleCreateClicked);
		Footer->OnCreateClicked.AddDynamic(this, &ThisClass::HandleCreateClicked);

		Footer->OnBackClicked.RemoveDynamic(this, &ThisClass::HandleBackClicked);
		Footer->OnBackClicked.AddDynamic(this, &ThisClass::HandleBackClicked);

		Footer->SetBusy(false);
		Footer->SetFeedbackMessage(FText::GetEmpty(), false);
	}
}

void UOnsetModularCharacterCreationScreen::NativeOnDeactivated()
{
	if (Customizer)
	{
		Customizer->OnAppearanceDraftChanged.RemoveDynamic(this, &ThisClass::HandleAppearanceDraftChanged);
		Customizer->OnCameraFramingRequested.RemoveDynamic(this, &ThisClass::HandleCameraFramingRequested);
	}

	if (NameInput)
	{
		NameInput->OnNameChanged.RemoveDynamic(this, &ThisClass::HandleNameChanged);
	}

	if (ClassPicker)
	{
		ClassPicker->OnClassSelected.RemoveDynamic(this, &ThisClass::HandleClassSelected);
	}

	if (Footer)
	{
		Footer->OnCreateClicked.RemoveDynamic(this, &ThisClass::HandleCreateClicked);
		Footer->OnBackClicked.RemoveDynamic(this, &ThisClass::HandleBackClicked);
	}

	bRequestPending = false;

	Super::NativeOnDeactivated();
}

void UOnsetModularCharacterCreationScreen::NativeDestruct()
{
	SetPlayerController(nullptr);
	Super::NativeDestruct();
}

UWidget* UOnsetModularCharacterCreationScreen::NativeGetDesiredFocusTarget() const
{
	if (ClassPicker)
	{
		if (UCommonButtonBase* SelectedClassBtn = ClassPicker->GetSelectedClassButton())
		{
			if (SelectedClassBtn->IsVisible() && SelectedClassBtn->GetIsEnabled())
			{
				return SelectedClassBtn;
			}
		}
	}

	if (NameInput && NameInput->IsVisible() && NameInput->GetIsEnabled())
	{
		return NameInput;
	}

	return Super::NativeGetDesiredFocusTarget();
}

FReply UOnsetModularCharacterCreationScreen::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	// Shoulder bumpers: Cycle across panels (LB / RB)
	if (Key == EKeys::Gamepad_RightShoulder || Key == EKeys::Gamepad_LeftShoulder)
	{
		CyclePanelFocus(Key == EKeys::Gamepad_RightShoulder);
		return FReply::Handled();
	}

	// Direct Create Operative shortcut: Face Button Left (X) or Special Right (Menu/Start)
	if (Key == EKeys::Gamepad_FaceButton_Left || Key == EKeys::Gamepad_Special_Right)
	{
		HandleCreateClicked();
		return FReply::Handled();
	}

	// R3 Click: Reset turntable rotation
	if (Key == EKeys::Gamepad_RightThumbstick && StageWidget)
	{
		StageWidget->ResetTurntableRotation();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UOnsetModularCharacterCreationScreen::CyclePanelFocus(bool bForward)
{
	enum ECreationPanel : int32
	{
		Panel_Class = 0,
		Panel_Callsign = 1,
		Panel_Appearance = 2,
		Panel_Footer = 3,
		Panel_MAX = 4
	};

	ECreationPanel CurrentPanel = Panel_Class;
	APlayerController* PC = GetOwningPlayer();
	bool bFound = false;

	if (ClassPicker && ClassPicker->HasUserFocusedDescendants(PC))
	{
		CurrentPanel = Panel_Class;
		bFound = true;
	}
	else if (NameInput && NameInput->HasUserFocusedDescendants(PC))
	{
		CurrentPanel = Panel_Callsign;
		bFound = true;
	}
	else if (Customizer && Customizer->HasUserFocusedDescendants(PC))
	{
		CurrentPanel = Panel_Appearance;
		bFound = true;
	}
	else if (Footer && Footer->HasUserFocusedDescendants(PC))
	{
		CurrentPanel = Panel_Footer;
		bFound = true;
	}

	int32 NextPanelIdx = 0;
	if (bFound)
	{
		NextPanelIdx = bForward
			? ((static_cast<int32>(CurrentPanel) + 1) % Panel_MAX)
			: ((static_cast<int32>(CurrentPanel) - 1 + Panel_MAX) % Panel_MAX);
	}
	else
	{
		NextPanelIdx = bForward ? Panel_Callsign : Panel_Footer;
	}

	switch (static_cast<ECreationPanel>(NextPanelIdx))
	{
	case Panel_Class:
		if (ClassPicker)
		{
			if (UCommonButtonBase* Btn = ClassPicker->GetSelectedClassButton())
			{
				Btn->SetFocus();
			}
		}
		break;
	case Panel_Callsign:
		if (NameInput)
		{
			NameInput->SetFocus();
		}
		break;
	case Panel_Appearance:
		if (Customizer)
		{
			Customizer->SetFocus();
		}
		break;
	case Panel_Footer:
		if (Footer)
		{
			if (bForward && Footer->GetCreateButton())
			{
				Footer->GetCreateButton()->SetFocus();
			}
			else if (Footer->GetBackButton())
			{
				Footer->GetBackButton()->SetFocus();
			}
		}
		break;
	default:
		break;
	}
}

void UOnsetModularCharacterCreationScreen::HandleNameChanged(const FString& NewName, bool bIsValid)
{
	if (Footer)
	{
		if (!bIsValid && !NewName.IsEmpty())
		{
			Footer->SetFeedbackMessage(NSLOCTEXT("OnsetCreation", "InvalidNameLength", "Callsign must be 1-16 characters."), true);
		}
		else
		{
			Footer->SetFeedbackMessage(FText::GetEmpty(), false);
		}
	}
}

void UOnsetModularCharacterCreationScreen::HandleClassSelected(EOnsetCharacterClass NewClass)
{
	// Class selection updated
}

void UOnsetModularCharacterCreationScreen::HandleAppearanceDraftChanged(const FOnsetAppearanceRecipe& NewRecipe)
{
	DraftAppearance = NewRecipe;

	if (StageWidget)
	{
		StageWidget->SetAppearanceRecipe(DraftAppearance);
	}
}

void UOnsetModularCharacterCreationScreen::HandleCameraFramingRequested(bool bFaceCloseUp)
{
	if (StageWidget)
	{
		StageWidget->SetCameraFraming(bFaceCloseUp);
	}
}

void UOnsetModularCharacterCreationScreen::HandleCreateClicked()
{
	if (bRequestPending || !CachedPlayerController || SlotIndex < 0)
	{
		return;
	}

	const FString ChosenName = NameInput ? NameInput->GetTrimmedName() : DefaultCharacterName;
	if (ChosenName.IsEmpty() || (NameInput && !NameInput->IsNameValid()))
	{
		if (Footer)
		{
			Footer->SetFeedbackMessage(NSLOCTEXT("OnsetCreation", "NameRequired", "Enter a valid callsign (1-16 characters)."), true);
		}
		return;
	}

	const EOnsetCharacterClass ChosenClass = ClassPicker ? ClassPicker->GetSelectedClass() : EOnsetCharacterClass::Tank;
	if (!StaticEnum<EOnsetCharacterClass>()->IsValidEnumValue(static_cast<int64>(ChosenClass)))
	{
		if (Footer)
		{
			Footer->SetFeedbackMessage(NSLOCTEXT("OnsetCreation", "ClassRequired", "Choose a class."), true);
		}
		return;
	}

	TArray<FOnsetAppearanceIssue> Issues;
	if (AppearanceCatalog && !AppearanceCatalog->ValidateRecipe(DraftAppearance, EOnsetAppearanceUse::Player, Issues))
	{
		if (Footer)
		{
			Footer->SetFeedbackMessage(Issues.IsEmpty() ? NSLOCTEXT("OnsetCreation", "InvalidAppearance", "Invalid appearance configuration.") : FText::FromString(Issues[0].Message), true);
		}
		return;
	}

	FString JSON;
	if (!UOnsetAppearanceLibrary::EncodeRecipe(DraftAppearance, JSON))
	{
		if (Footer)
		{
			Footer->SetFeedbackMessage(NSLOCTEXT("OnsetCreation", "EncodeError", "Failed to encode appearance."), true);
		}
		return;
	}

	bRequestPending = true;

	if (Footer)
	{
		Footer->SetBusy(true);
		Footer->SetFeedbackMessage(NSLOCTEXT("OnsetCreation", "CreatingOperative", "Creating operative..."), false);
	}

	CachedPlayerController->Server_CreateCharacterWithAppearance(SlotIndex, ChosenName, ChosenClass, JSON);
}

void UOnsetModularCharacterCreationScreen::HandleBackClicked()
{
	DeactivateWidget();
}

void UOnsetModularCharacterCreationScreen::HandleAppearanceResult(bool bSuccess, const TArray<FOnsetAppearanceIssue>& Issues)
{
	if (!bRequestPending)
	{
		return;
	}

	bRequestPending = false;

	if (Footer)
	{
		Footer->SetBusy(false);

		if (bSuccess)
		{
			Footer->SetFeedbackMessage(NSLOCTEXT("OnsetCreation", "Success", "Operative created."), false);
			DeactivateWidget();
		}
		else
		{
			Footer->SetFeedbackMessage(
				Issues.IsEmpty() ? NSLOCTEXT("OnsetCreation", "CreationFailed", "Character creation failed.") : FText::FromString(Issues[0].Message),
				true
			);
		}
	}
}
