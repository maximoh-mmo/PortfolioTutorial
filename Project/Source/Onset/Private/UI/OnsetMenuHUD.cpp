#include "UI/OnsetMenuHUD.h"
#include "Engine/Canvas.h"
#include "InputCoreTypes.h"
#include "Engine/Engine.h"
#include "Multiplayer/OnsetGameModeBase.h"

void AOnsetMenuHUD::DrawHUD()
{
	Super::DrawHUD();

	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;

	
	PC->bShowMouseCursor = true;
	PC->SetInputMode(FInputModeGameAndUI());

	if (bInputEnabled && (PC->WasInputKeyJustPressed(EKeys::Enter) || PC->WasInputKeyJustPressed(EKeys::SpaceBar)))
	{
		bInputEnabled = false;
		PC->ClientTravel(TEXT("127.0.0.1:7777"), TRAVEL_Absolute);
	}
	
	float CX = Canvas->SizeX * 0.5f;
	float CY = Canvas->SizeY * 0.5f;

	FCanvasTextItem Title(
		FVector2D(CX - 80.0f, CY - 60.0f),
		FText::FromString(TEXT("ONLINE")),
		GEngine->GetSmallFont(),
		FLinearColor::White
	);
	Title.EnableShadow(FLinearColor::Black);
	Title.Scale = FVector2D(3.0f, 3.0f);
	Canvas->DrawItem(Title);

	FCanvasTextItem BtnText(
		FVector2D(CX - 100.0f, CY + 20.0f),
		FText::FromString(TEXT("[ Press Enter to Connect ]")),
		GEngine->GetSmallFont(),
		FLinearColor::Green
	);
	BtnText.Scale = FVector2D(1.5f, 1.5f);
	Canvas->DrawItem(BtnText);
}
