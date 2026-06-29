#include "UI/CharacterSelectWidget.h"

#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Player/OnsetPlayerController.h"

UButton* CreateButton(const FString& Label, UWidget* Outer)
{
	UButton* Btn = NewObject<UButton>(Outer);
	UTextBlock* BtnLabel = NewObject<UTextBlock>(Btn);
	BtnLabel->SetText(FText::FromString(Label));
	BtnLabel->SetJustification(ETextJustify::Center);
	Btn->SetContent(BtnLabel);
	return Btn;
}

void UCharacterSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildUI();
}

void UCharacterSelectWidget::BuildUI()
{
	UCanvasPanel* Root = NewObject<UCanvasPanel>(this);
	if (!Root) return;

	WidgetTree->RootWidget = Root;

	UVerticalBox* MainVBox = NewObject<UVerticalBox>(Root);
	Root->AddChild(MainVBox);
	if (UCanvasPanelSlot* VBoxSlot = Cast<UCanvasPanelSlot>(MainVBox->Slot))
	{
		VBoxSlot->SetAnchors(FAnchors(0.5f));
		VBoxSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		VBoxSlot->SetSize(FVector2D(400.0f, 500.0f));
	}

	UTextBlock* TitleText = NewObject<UTextBlock>(MainVBox);
	TitleText->SetText(FText::FromString(TEXT("SELECT CHARACTER")));
	TitleText->SetJustification(ETextJustify::Center);
	FSlateFontInfo TitleFont = FCoreStyle::Get().GetFontStyle("NormalFont");
	TitleFont.Size = 28;
	TitleText->SetFont(TitleFont);
	MainVBox->AddChild(TitleText);

	SlotContainer = NewObject<UVerticalBox>(MainVBox);
	MainVBox->AddChild(SlotContainer);
	if (UVerticalBoxSlot* ContainerSlot = Cast<UVerticalBoxSlot>(SlotContainer->Slot))
	{
		ContainerSlot->SetPadding(FMargin(0.0f, 20.0f, 0.0f, 20.0f));
	}

	ActionButtons.SetNum(3);
	NameLabels.SetNum(3);
	LevelLabels.SetNum(3);

	for (int32 i = 0; i < 3; ++i)
	{
		UBorder* Border = NewObject<UBorder>(SlotContainer);
		Border->SetPadding(FMargin(10.0f, 5.0f));

		UHorizontalBox* HBox = NewObject<UHorizontalBox>(Border);
		Border->SetContent(HBox);

		NameLabels[i] = NewObject<UTextBlock>(HBox);
		NameLabels[i]->SetText(FText::FromString(TEXT("Empty Slot")));
		HBox->AddChild(NameLabels[i]);
		if (UHorizontalBoxSlot* HSlot = Cast<UHorizontalBoxSlot>(NameLabels[i]->Slot))
		{
			HSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			HSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
		}

		LevelLabels[i] = NewObject<UTextBlock>(HBox);
		LevelLabels[i]->SetText(FText::FromString(TEXT("Lv. 1")));
		LevelLabels[i]->SetVisibility(ESlateVisibility::Collapsed);
		HBox->AddChild(LevelLabels[i]);
		if (UHorizontalBoxSlot* HSlot = Cast<UHorizontalBoxSlot>(LevelLabels[i]->Slot))
		{
			HSlot->SetPadding(FMargin(10.0f, 0.0f));
			HSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);
		}

		ActionButtons[i] = CreateButton(TEXT("Create"), HBox);
		HBox->AddChild(ActionButtons[i]);

		SlotContainer->AddChild(Border);
	}

	EnterWorldButton = CreateButton(TEXT("ENTER WORLD"), MainVBox);
	EnterWorldButton->SetIsEnabled(false);
	EnterWorldButton->OnClicked.AddDynamic(this, &UCharacterSelectWidget::OnEnterWorldClicked);
	MainVBox->AddChild(EnterWorldButton);

	// Bind slot button handlers
	ActionButtons[0]->OnClicked.AddDynamic(this, &UCharacterSelectWidget::OnClickSlot0);
	ActionButtons[1]->OnClicked.AddDynamic(this, &UCharacterSelectWidget::OnClickSlot1);
	ActionButtons[2]->OnClicked.AddDynamic(this, &UCharacterSelectWidget::OnClickSlot2);
}

void UCharacterSelectWidget::RefreshSlots()
{
	for (int32 i = 0; i < 3; ++i)
	{
		if (i < AccountData.Slots.Num() && AccountData.Slots[i].bOccupied)
		{
			NameLabels[i]->SetText(FText::FromString(AccountData.Slots[i].CharacterName));
			LevelLabels[i]->SetText(FText::FromString(FString::Printf(TEXT("Lv. %d"), AccountData.Slots[i].Level)));
			LevelLabels[i]->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			NameLabels[i]->SetText(FText::FromString(TEXT("Empty Slot")));
			LevelLabels[i]->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UCharacterSelectWidget::SetAccountData(const FOnsetAccountData& InAccountData)
{
	AccountData = InAccountData;
	if (SlotContainer)
		RefreshSlots();
}

void UCharacterSelectWidget::OnClickSlot0() { HandleSlotClicked(0); }
void UCharacterSelectWidget::OnClickSlot1() { HandleSlotClicked(1); }
void UCharacterSelectWidget::OnClickSlot2() { HandleSlotClicked(2); }

void UCharacterSelectWidget::HandleSlotClicked(int32 SlotIndex)
{
	AOnsetPlayerController* PC = Cast<AOnsetPlayerController>(GetOwningPlayer());
	if (!PC) return;

	if (SlotIndex < AccountData.Slots.Num() && AccountData.Slots[SlotIndex].bOccupied)
	{
		// Select this slot — highlight and enable Enter World
		SelectedSlot = SlotIndex;
		EnterWorldButton->SetIsEnabled(true);
	}
	else
	{
		// Create a character in this empty slot
		FString DefaultName = FString::Printf(TEXT("Hero_%d"), SlotIndex + 1);
		PC->Server_CreateCharacter(SlotIndex, DefaultName);
	}
}

void UCharacterSelectWidget::OnEnterWorldClicked()
{
	if (SelectedSlot < 0) return;

	AOnsetPlayerController* PC = Cast<AOnsetPlayerController>(GetOwningPlayer());
	if (!PC) return;

	PC->Server_SelectCharacter(SelectedSlot);
}
