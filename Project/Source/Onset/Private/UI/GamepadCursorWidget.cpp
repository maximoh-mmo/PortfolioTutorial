// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/GamepadCursorWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/GameplayStatics.h"

void UGamepadCursorWidget::SetCursorPosition(const FVector2D& ScreenPosition)
{
	if (CrosshairImage && GetWorld())
	{
		FVector2D ViewportSize = GetWorld()->GetGameViewport()->Viewport->GetSizeXY();
		FVector2D WidgetSize = GetCachedGeometry().GetLocalSize();
		FVector2D ScaledPos = ScreenPosition * (WidgetSize / ViewportSize);

		FVector2D ImageSize = CrosshairImage->GetDesiredSize();
		FVector2D ClampedPos;
		ClampedPos.X = FMath::Clamp(ScaledPos.X, 0.0f, WidgetSize.X - ImageSize.X);
		ClampedPos.Y = FMath::Clamp(ScaledPos.Y, 0.0f, WidgetSize.Y - ImageSize.Y);

		CursorPosition = ClampedPos;
		if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(CrosshairImage->Slot))
		{
			PanelSlot->SetPosition(ClampedPos);
		}
	}
}

void UGamepadCursorWidget::ShowCursor()
{
	if (!bIsVisible)
	{
		bIsVisible = true;
		SetVisibility(ESlateVisibility::Visible);
	}
}

void UGamepadCursorWidget::HideCursor()
{
	if (bIsVisible)
	{
		bIsVisible = false;
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UGamepadCursorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UGamepadCursorWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	CrosshairImage = Cast<UImage>(GetWidgetFromName(TEXT("Crosshair")));
}

                
