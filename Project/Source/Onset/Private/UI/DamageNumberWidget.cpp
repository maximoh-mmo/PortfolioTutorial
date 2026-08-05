// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/DamageNumberWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"
#include "Fonts/SlateFontInfo.h"

void UDamageNumberWidget::ShowDamage(float Amount, FVector2D ScreenPosition)
{
	if (!NumberText)
	{
		return;
	}

	NumberText->SetText(FText::AsNumber(FMath::RoundToInt(Amount)));
	StartColor = FLinearColor::White;
	NumberText->SetColorAndOpacity(StartColor);

	// Apply a small random jitter so numbers from the same hit location don't overlap exactly.
	const float JitterX = FMath::FRandRange(-JitterRadius, JitterRadius);
	const float JitterY = FMath::FRandRange(-JitterRadius, JitterRadius);
	StartPosition = ScreenPosition + FVector2D(JitterX, JitterY);
	Elapsed = 0.0f;
	bIsAnimating = true;

	SetVisibility(ESlateVisibility::HitTestInvisible);

	FWidgetTransform Transform;
	Transform.Translation = StartPosition;
	SetRenderTransform(Transform);
}

void UDamageNumberWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree)
	{
		return;
	}

	// Build a minimal visual tree so the widget works standalone (no Blueprint needed).
	if (UCanvasPanel* RootPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootPanel")))
	{
		WidgetTree->RootWidget = RootPanel;

		if (!NumberText)
		{
			NumberText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NumberText"));
			if (NumberText)
			{
				NumberText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 20, TEXT("Bold")));
				NumberText->SetJustification(ETextJustify::Center);

				UCanvasPanelSlot* TextSlot = RootPanel->AddChildToCanvas(NumberText);
				if (TextSlot)
				{
					TextSlot->SetAnchors(FAnchors(0.0f, 0.0f));
					TextSlot->SetAlignment(FVector2D(0.5f, 0.5f));
					TextSlot->SetAutoSize(true);
				}
			}
		}
	}
}

void UDamageNumberWidget::Deactivate()
{
	if (!bIsAnimating)
	{
		return;
	}

	bIsAnimating = false;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UDamageNumberWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bIsAnimating || !NumberText)
	{
		return;
	}

	Elapsed += InDeltaTime;
	const float Alpha = FMath::Clamp(Elapsed / Lifetime, 0.0f, 1.0f);

	FWidgetTransform Transform;
	Transform.Translation = StartPosition + FVector2D(0.0f, -FloatDistance * Alpha);
	SetRenderTransform(Transform);

	NumberText->SetColorAndOpacity(StartColor.CopyWithNewOpacity(1.0f - Alpha));

	if (Alpha >= 1.0f)
	{
		Deactivate();
	}
}
