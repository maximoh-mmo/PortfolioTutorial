#include "UI/OnsetLobbyHUD.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Player/OnsetPlayerController.h"

void AOnsetLobbyHUD::ShowAccountData(const FOnsetAccountData& InAccountData)
{
	AccountData = InAccountData;
	bHasAccountData = true;
}

void AOnsetLobbyHUD::DrawHUD()
{
	Super::DrawHUD();

	AOnsetPlayerController* PC = Cast<AOnsetPlayerController>(GetOwningPlayerController());
	if (!PC) return;

	PC->bShowMouseCursor = true;
	PC->SetInputMode(FInputModeGameAndUI());

	float CX = Canvas->SizeX * 0.5f;

	FCanvasTextItem Title(
		FVector2D(CX - 160.0f, 40.0f),
		FText::FromString(TEXT("CHARACTER SELECT")),
		GEngine->GetSmallFont(),
		FLinearColor::Yellow
	);
	Title.EnableShadow(FLinearColor::Black);
	Title.Scale = FVector2D(2.5f, 2.5f);
	Canvas->DrawItem(Title);

	if (!bHasAccountData)
	{
		FCanvasTextItem WaitText(
			FVector2D(CX - 100.0f, 120.0f),
			FText::FromString(TEXT("Waiting for account data...")),
			GEngine->GetSmallFont(),
			FLinearColor::White
		);
		WaitText.Scale = FVector2D(1.2f, 1.2f);
		Canvas->DrawItem(WaitText);
		return;
	}

	TArray<FString> StatusLabels = { TEXT("Slot 1"), TEXT("Slot 2"), TEXT("Slot 3") };
	for (int32 i = 0; i < 3; ++i)
	{
		FString Label;
		FLinearColor Color;
		if (i < AccountData.Slots.Num() && AccountData.Slots[i].bOccupied)
		{
			Label = FString::Printf(TEXT("[%d] %s (Lv.%d)"), i + 1, *AccountData.Slots[i].CharacterName, AccountData.Slots[i].Level);
			Color = FLinearColor::Green;
		}
		else
		{
			Label = FString::Printf(TEXT("[%d] Empty Slot"), i + 1);
			Color = FLinearColor(0.5f, 0.5f, 0.5f);
		}

		FCanvasTextItem SlotText(
			FVector2D(CX - 120.0f, 120.0f + i * 40.0f),
			FText::FromString(Label),
			GEngine->GetSmallFont(),
			Color
		);
		SlotText.EnableShadow(FLinearColor::Black);
		SlotText.Scale = FVector2D(1.5f, 1.5f);
		Canvas->DrawItem(SlotText);
	}
}
