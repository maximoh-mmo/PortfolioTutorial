// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/PlayerHealthBarWidget.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GAS/OnsetAttributeSet.h"
#include "Styling/CoreStyle.h"
#include "Fonts/SlateFontInfo.h"

void UPlayerHealthBarWidget::BindToASC(UAbilitySystemComponent* InASC)
{
	if (!InASC)
	{
		return;
	}

	if (BoundASC)
	{
		BoundASC->GetGameplayAttributeValueChangeDelegate(UOnsetAttributeSet::GetHealthAttribute()).RemoveAll(this);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UOnsetAttributeSet::GetMaxHealthAttribute()).RemoveAll(this);
	}

	BoundASC = InASC;
	BoundASC->GetGameplayAttributeValueChangeDelegate(UOnsetAttributeSet::GetHealthAttribute()).AddUObject(this, &UPlayerHealthBarWidget::HandleAttributeChanged);
	BoundASC->GetGameplayAttributeValueChangeDelegate(UOnsetAttributeSet::GetMaxHealthAttribute()).AddUObject(this, &UPlayerHealthBarWidget::HandleAttributeChanged);

	RefreshHealth();
}

void UPlayerHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!WidgetTree)
	{
		return;
	}

	if (UCanvasPanel* RootPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootPanel")))
	{
		WidgetTree->RootWidget = RootPanel;

		if (!HealthBar)
		{
			HealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthBar"));
			if (HealthBar)
			{
				HealthBar->SetFillColorAndOpacity(FLinearColor(0.6f, 0.0f, 0.0f, 1.0f));

				UCanvasPanelSlot* BarSlot = RootPanel->AddChildToCanvas(HealthBar);
				if (BarSlot)
				{
					BarSlot->SetAnchors(FAnchors(0.5f, 1.0f));
					BarSlot->SetAlignment(FVector2D(0.5f, 1.0f));
					BarSlot->SetSize(FVector2D(300.0f, 12.0f));
					BarSlot->SetPosition(FVector2D(0.0f, -40.0f));
				}
			}
		}

		if (!HealthText)
		{
			HealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HealthText"));
			if (HealthText)
			{
				HealthText->SetJustification(ETextJustify::Center);
				HealthText->SetFont(FSlateFontInfo(FCoreStyle::GetDefaultFont(), 14, TEXT("Bold")));

				UCanvasPanelSlot* TextSlot = RootPanel->AddChildToCanvas(HealthText);
				if (TextSlot)
				{
					TextSlot->SetAnchors(FAnchors(0.5f, 1.0f));
					TextSlot->SetAlignment(FVector2D(0.5f, 1.0f));
					TextSlot->SetAutoSize(true);
					TextSlot->SetPosition(FVector2D(0.0f, -60.0f));
				}
			}
		}
	}

	RefreshHealth();
}

void UPlayerHealthBarWidget::NativeDestruct()
{
	if (BoundASC)
	{
		BoundASC->GetGameplayAttributeValueChangeDelegate(UOnsetAttributeSet::GetHealthAttribute()).RemoveAll(this);
		BoundASC->GetGameplayAttributeValueChangeDelegate(UOnsetAttributeSet::GetMaxHealthAttribute()).RemoveAll(this);
		BoundASC = nullptr;
	}

	Super::NativeDestruct();
}

void UPlayerHealthBarWidget::HandleAttributeChanged(const FOnAttributeChangeData& Data)
{
	RefreshHealth();
}

void UPlayerHealthBarWidget::RefreshHealth()
{
	if (!BoundASC)
	{
		return;
	}

	const float MaxHealth = BoundASC->GetNumericAttribute(UOnsetAttributeSet::GetMaxHealthAttribute());
	const float Health = BoundASC->GetNumericAttribute(UOnsetAttributeSet::GetHealthAttribute());

	if (HealthBar)
	{
		HealthBar->SetPercent(MaxHealth > 0.0f ? FMath::Clamp(Health / MaxHealth, 0.0f, 1.0f) : 0.0f);
	}

	if (HealthText)
	{
		HealthText->SetText(FText::Format(FText::FromString(TEXT("{0} / {1}")), FText::AsNumber(FMath::RoundToInt(Health)), FText::AsNumber(FMath::RoundToInt(MaxHealth))));
	}
}
