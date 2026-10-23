// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Creation/CharacterClassPickerWidget.h"
#include "Combat/OnsetEquipmentLibrary.h"
#include "Data/OnsetClassInfoTypes.h"
#include "CommonButtonBase.h"
#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Engine/Texture2D.h"

void UCharacterClassPickerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (TankButton)
	{
		TankButton->OnClicked().AddWeakLambda(this, [this]() { SelectClass(EOnsetCharacterClass::Tank); });
	}

	if (DpsButton)
	{
		DpsButton->OnClicked().AddWeakLambda(this, [this]() { SelectClass(EOnsetCharacterClass::DPS); });
	}

	if (RangedButton)
	{
		RangedButton->OnClicked().AddWeakLambda(this, [this]() { SelectClass(EOnsetCharacterClass::Ranged); });
	}

	if (SupportButton)
	{
		SupportButton->OnClicked().AddWeakLambda(this, [this]() { SelectClass(EOnsetCharacterClass::Support); });
	}
}

void UCharacterClassPickerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SelectClass(SelectedClass);
}

void UCharacterClassPickerWidget::SelectClass(EOnsetCharacterClass NewClass)
{
	SelectedClass = NewClass;

	if (TankButton)
	{
		TankButton->SetIsSelected(NewClass == EOnsetCharacterClass::Tank);
	}
	if (DpsButton)
	{
		DpsButton->SetIsSelected(NewClass == EOnsetCharacterClass::DPS);
	}
	if (RangedButton)
	{
		RangedButton->SetIsSelected(NewClass == EOnsetCharacterClass::Ranged);
	}
	if (SupportButton)
	{
		SupportButton->SetIsSelected(NewClass == EOnsetCharacterClass::Support);
	}

	UpdateDetailsDisplay();
	OnClassSelected.Broadcast(SelectedClass);
}

void UCharacterClassPickerWidget::UpdateDetailsDisplay()
{
	const FOnsetCharacterClassInfo* Info = UOnsetEquipmentLibrary::GetClassInfo(SelectedClass);

	if (ClassNameText)
	{
		if (Info && !Info->ClassName.IsEmpty())
		{
			ClassNameText->SetText(Info->ClassName);
		}
		else
		{
			FText FallbackName = NSLOCTEXT("OnsetCreation", "ClassTank", "Vanguard");
			switch (SelectedClass)
			{
			case EOnsetCharacterClass::DPS: FallbackName = NSLOCTEXT("OnsetCreation", "ClassDPS", "Infiltrator"); break;
			case EOnsetCharacterClass::Ranged: FallbackName = NSLOCTEXT("OnsetCreation", "ClassRanged", "Ranger"); break;
			case EOnsetCharacterClass::Support: FallbackName = NSLOCTEXT("OnsetCreation", "ClassSupport", "Psion"); break;
			default: break;
			}
			ClassNameText->SetText(FallbackName);
		}
	}

	if (ClassRoleText)
	{
		FText RoleText = NSLOCTEXT("OnsetCreation", "RoleTank", "Frontline Defender / Threat Controller");
		switch (SelectedClass)
		{
		case EOnsetCharacterClass::DPS: RoleText = NSLOCTEXT("OnsetCreation", "RoleDPS", "Melee Infiltrator / Burst DPS"); break;
		case EOnsetCharacterClass::Ranged: RoleText = NSLOCTEXT("OnsetCreation", "RoleRanged", "Tactical Marksman / Sustained Damage"); break;
		case EOnsetCharacterClass::Support: RoleText = NSLOCTEXT("OnsetCreation", "RoleSupport", "Psion Specialist / Healing & Support"); break;
		default: break;
		}
		ClassRoleText->SetText(RoleText);
	}

	if (ClassDescriptionText && Info)
	{
		ClassDescriptionText->SetText(Info->Description);
	}

	if (ClassIcon && Info)
	{
		if (Info->ClassIcon.IsValid())
		{
			ClassIcon->SetBrushFromTexture(Info->ClassIcon.Get());
		}
		else if (!Info->ClassIcon.IsNull())
		{
			UTexture2D* LoadedIcon = Info->ClassIcon.LoadSynchronous();
			if (LoadedIcon)
			{
				ClassIcon->SetBrushFromTexture(LoadedIcon);
			}
		}
	}

	if (Info)
	{
		const float MaxVal = MaxStatReferenceValue > KINDA_SMALL_NUMBER ? MaxStatReferenceValue : 25.0f;

		if (StrengthBar)
		{
			StrengthBar->SetPercent(FMath::Clamp(Info->BaseStrength / MaxVal, 0.0f, 1.0f));
		}
		if (AgilityBar)
		{
			AgilityBar->SetPercent(FMath::Clamp(Info->BaseAgility / MaxVal, 0.0f, 1.0f));
		}
		if (IntellectBar)
		{
			IntellectBar->SetPercent(FMath::Clamp(Info->BaseIntellect / MaxVal, 0.0f, 1.0f));
		}
		if (DefenseBar)
		{
			DefenseBar->SetPercent(FMath::Clamp(Info->BaseDefense / MaxVal, 0.0f, 1.0f));
		}
		if (VitalityBar)
		{
			VitalityBar->SetPercent(FMath::Clamp(Info->BaseVitality / MaxVal, 0.0f, 1.0f));
		}
	}
}

UCommonButtonBase* UCharacterClassPickerWidget::GetSelectedClassButton() const
{
	switch (SelectedClass)
	{
	case EOnsetCharacterClass::Tank: return TankButton;
	case EOnsetCharacterClass::DPS: return DpsButton;
	case EOnsetCharacterClass::Ranged: return RangedButton;
	case EOnsetCharacterClass::Support: return SupportButton;
	default: return TankButton;
	}
}

TArray<UCommonButtonBase*> UCharacterClassPickerWidget::GetAllClassButtons() const
{
	TArray<UCommonButtonBase*> Buttons;
	if (TankButton) Buttons.Add(TankButton);
	if (DpsButton) Buttons.Add(DpsButton);
	if (RangedButton) Buttons.Add(RangedButton);
	if (SupportButton) Buttons.Add(SupportButton);
	return Buttons;
}
