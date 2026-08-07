// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/DamageNumberWidget.h"

#include "CommonTextBlock.h"

void UDamageNumberWidget::ShowDamage(float Amount, FVector2D ScreenPosition, FLinearColor Color)
{
	if (!NumberText)
	{
		return;
	}

	NumberText->SetText(FText::AsNumber(FMath::RoundToInt(Amount)));
	StartColor = Color;
	NumberText->SetColorAndOpacity(StartColor);

	static bool bLoggedColor = false;
	if (!bLoggedColor)
	{
		bLoggedColor = true;
		UE_LOG(LogTemp, Warning, TEXT("[HUDDiag] ShowDamage first call: RequestedColor=%s EffectiveColor=%s StartPos=%s"),
			*StartColor.ToString(), *NumberText->GetColorAndOpacity().GetSpecifiedColor().ToString(), *ScreenPosition.ToString());
	}

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
