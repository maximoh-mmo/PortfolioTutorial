#include "UI/MainMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildUI();
}

void UMainMenuWidget::BuildUI()
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
		VBoxSlot->SetSize(FVector2D(400.0f, 300.0f));
	}

	UTextBlock* TitleText = NewObject<UTextBlock>(MainVBox);
	TitleText->SetText(FText::FromString(TEXT("ONLINE")));
	TitleText->SetJustification(ETextJustify::Center);
	FSlateFontInfo TitleFont = FCoreStyle::Get().GetFontStyle("NormalFont");
	TitleFont.Size = 48;
	TitleText->SetFont(TitleFont);
	MainVBox->AddChild(TitleText);
	if (UVerticalBoxSlot* TSlot = Cast<UVerticalBoxSlot>(TitleText->Slot))
	{
		TSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 40.0f));
	}

	ConnectButton = NewObject<UButton>(MainVBox);
	UTextBlock* BtnLabel = NewObject<UTextBlock>(ConnectButton);
	BtnLabel->SetText(FText::FromString(TEXT("Connect to Server")));
	BtnLabel->SetJustification(ETextJustify::Center);
	ConnectButton->SetContent(BtnLabel);
	ConnectButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnConnectClicked);
	MainVBox->AddChild(ConnectButton);

	StatusText = NewObject<UTextBlock>(MainVBox);
	StatusText->SetText(FText::FromString(TEXT("")));
	StatusText->SetJustification(ETextJustify::Center);
	MainVBox->AddChild(StatusText);
	if (UVerticalBoxSlot* SSlot = Cast<UVerticalBoxSlot>(StatusText->Slot))
	{
		SSlot->SetPadding(FMargin(0.0f, 20.0f, 0.0f, 0.0f));
	}
}

void UMainMenuWidget::OnConnectClicked()
{
	if (!ConnectButton) return;

	ConnectButton->SetIsEnabled(false);
	StatusText->SetText(FText::FromString(TEXT("Connecting...")));

	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	PC->ClientTravel(TEXT("127.0.0.1:7777"), TRAVEL_Absolute);
}
