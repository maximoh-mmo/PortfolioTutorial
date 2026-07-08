// Fill out your copyright notice in the Description page of Project Settings.
#include "UI/OnsetScreenBase.h"
#include "CommonInputModeTypes.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
 
UOnsetScreenBase::UOnsetScreenBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}
 
void UOnsetScreenBase::NativeOnActivated()
{
	Super::NativeOnActivated();
 
	if (AmbientMusicCue)
	{
		UGameplayStatics::PlaySound2D(this, AmbientMusicCue);
	}
 
	BP_OnScreenActivated();
}
 
void UOnsetScreenBase::NativeOnDeactivated()
{
	BP_OnScreenDeactivated();
 
	Super::NativeOnDeactivated();
}
 
TOptional<FUIInputConfig> UOnsetScreenBase::GetDesiredInputConfig() const
{
	if (bUseMenuInputMode)
	{
		return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
	}
 
	return Super::GetDesiredInputConfig();
}
 