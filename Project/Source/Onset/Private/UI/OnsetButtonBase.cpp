// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/OnsetButtonBase.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
 
UOnsetButtonBase::UOnsetButtonBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}
 
void UOnsetButtonBase::NativeOnHovered()
{
	Super::NativeOnHovered();
 
	if (HoverSound)
	{
		UGameplayStatics::PlaySound2D(this, HoverSound);
	}
}
 
void UOnsetButtonBase::NativeOnUnhovered()
{
	Super::NativeOnUnhovered();
}
 
void UOnsetButtonBase::NativeOnClicked()
{
	Super::NativeOnClicked();
 
	if (ClickSound)
	{
		UGameplayStatics::PlaySound2D(this, ClickSound);
	}
}
 