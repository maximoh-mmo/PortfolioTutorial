// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CharacterSelectScreen.h"
#include "UI/CharacterCreationScreen.h"
#include "UI/CharacterSlot.h"
#include "Player/OnsetPlayerController.h"
#include "Subsystem/OnsetUISubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"

void UCharacterSelectScreen::SetAccountData(const FOnsetAccountData& InAccountData)
{
	CachedAccountData = InAccountData;
	BuildSlotWidgets();
	RefreshSlotWidgets();
	BP_OnAccountDataReady(CachedAccountData);
}

void UCharacterSelectScreen::SetPlayerController(AOnsetPlayerController* PlayerController)
{
	if (CachedPlayerController == PlayerController) return;

	if (CachedPlayerController)
	{
		CachedPlayerController->OnAccountDataChanged.RemoveDynamic(this, &UCharacterSelectScreen::HandleAccountDataChanged);
	}

	CachedPlayerController = PlayerController;

	if (CachedPlayerController)
	{
		CachedPlayerController->OnAccountDataChanged.AddDynamic(this, &UCharacterSelectScreen::HandleAccountDataChanged);
	}
}

void UCharacterSelectScreen::SelectSlot(int32 SlotIndex)
{
	if (!CachedPlayerController) return;
	if (SlotIndex < 0 || SlotIndex >= CachedAccountData.Slots.Num()) return;
	
	if (CachedAccountData.Slots[SlotIndex].bOccupied)
	{
		SelectedSlot = SlotIndex;
		CachedPlayerController->Server_SelectCharacter(SelectedSlot);
	}
	else
	{
		OpenCreateCharacter(SlotIndex);
	}
}

void UCharacterSelectScreen::EnterWorld() const
{
	if (SelectedSlot < 0 || !CachedPlayerController) return;
	CachedPlayerController->Server_SelectCharacter(SelectedSlot);	
}

void UCharacterSelectScreen::DeleteCharacter(int32 SlotIndex)
{
	if (!CachedPlayerController) return;
	CachedPlayerController->Server_DeleteCharacter(SlotIndex);
}

void UCharacterSelectScreen::OpenCreateCharacter(int32 SlotIndex)
{
	if (!CachedPlayerController) return;

	UOnsetUISubsystem* UI = GetGameInstance()->GetSubsystem<UOnsetUISubsystem>();
	if (!UI || !CharacterCreationScreenClass) return;

	UCharacterCreationScreen* CreationScreen = Cast<UCharacterCreationScreen>(
		UI->PushScreen(EOnsetUILayer::Menu, CharacterCreationScreenClass));
	if (CreationScreen)
	{
		CreationScreen->SetPlayerController(CachedPlayerController);
		CreationScreen->SetSlotIndex(SlotIndex);
	}
}

void UCharacterSelectScreen::RefreshAccountData()
{
	if (!CachedPlayerController) return;
	SetAccountData(CachedPlayerController->GetCachedAccountData());
}

void UCharacterSelectScreen::CreateCharacter(int32 SlotIndex, const FString& CharacterName, EOnsetCharacterClass CharacterClass, int32 AppearancePresetIndex)
{
	if (!CachedPlayerController) return;
	CachedPlayerController->Server_CreateCharacter(SlotIndex, CharacterName, CharacterClass, AppearancePresetIndex);
}

void UCharacterSelectScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	RefreshAccountData();
}

void UCharacterSelectScreen::NativeDestruct()
{
	if (CachedPlayerController)
	{
		CachedPlayerController->OnAccountDataChanged.RemoveDynamic(this, &UCharacterSelectScreen::HandleAccountDataChanged);
	}

	for (UCharacterSlot* SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->OnSlotActivated.RemoveDynamic(this, &UCharacterSelectScreen::HandleSlotActivated);
			SlotWidget->OnDeleteRequested.RemoveDynamic(this, &UCharacterSelectScreen::HandleDeleteRequested);
		}
	}
	SlotWidgets.Empty();

	Super::NativeDestruct();
}

void UCharacterSelectScreen::HandleAccountDataChanged()
{
	RefreshAccountData();
}

void UCharacterSelectScreen::HandleSlotActivated(int32 InSlotIndex)
{
	SelectSlot(InSlotIndex);
}

void UCharacterSelectScreen::HandleDeleteRequested(int32 InSlotIndex)
{
	DeleteCharacter(InSlotIndex);
}

void UCharacterSelectScreen::BuildSlotWidgets()
{
	if (!CharacterSlotContainer || !CharacterSlotClass)
	{
		return;
	}

	const int32 DesiredCount = FMath::Max(3, CachedAccountData.Slots.Num());
	if (SlotWidgets.Num() == DesiredCount)
	{
		return;
	}

	for (UCharacterSlot* SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->OnSlotActivated.RemoveDynamic(this, &UCharacterSelectScreen::HandleSlotActivated);
			SlotWidget->OnDeleteRequested.RemoveDynamic(this, &UCharacterSelectScreen::HandleDeleteRequested);
		}
	}
	SlotWidgets.Empty();
	CharacterSlotContainer->ClearChildren();

	APlayerController* OwningPlayer = GetOwningPlayer();
	if (!OwningPlayer)
	{
		return;
	}

	for (int32 Index = 0; Index < DesiredCount; ++Index)
	{
		UCharacterSlot* SlotWidget = CreateWidget<UCharacterSlot>(OwningPlayer, CharacterSlotClass);
		if (!SlotWidget)
		{
			continue;
		}

		CharacterSlotContainer->AddChild(SlotWidget);

		if (UHorizontalBoxSlot* HorizontalSlot = Cast<UHorizontalBoxSlot>(SlotWidget->Slot))
		{
			HorizontalSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		SlotWidget->SetSlotIndex(Index);
		SlotWidget->OnSlotActivated.AddDynamic(this, &UCharacterSelectScreen::HandleSlotActivated);
		SlotWidget->OnDeleteRequested.AddDynamic(this, &UCharacterSelectScreen::HandleDeleteRequested);
		SlotWidgets.Add(SlotWidget);
	}
}

void UCharacterSelectScreen::RefreshSlotWidgets()
{
	for (int32 Index = 0; Index < SlotWidgets.Num(); ++Index)
	{
		UCharacterSlot* SlotWidget = SlotWidgets[Index];
		if (!SlotWidget)
		{
			continue;
		}

		if (Index < CachedAccountData.Slots.Num())
		{
			SlotWidget->SetSlotData(CachedAccountData.Slots[Index]);
		}
		else
		{
			FOnsetCharacterSlotData EmptySlot;
			EmptySlot.SlotIndex = Index;
			SlotWidget->SetSlotData(EmptySlot);
		}
	}
}
