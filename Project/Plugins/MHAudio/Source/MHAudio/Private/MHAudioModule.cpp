// Copyright Epic Games, Inc. All Rights Reserved.

#include "MHAudioModule.h"

DEFINE_LOG_CATEGORY(LogMHAudio);

void FMHAudioModule::StartupModule()
{
	UE_LOG(LogMHAudio, Log, TEXT("MHAudio module initialized."));
}

void FMHAudioModule::ShutdownModule()
{
	UE_LOG(LogMHAudio, Log, TEXT("MHAudio module shutdown."));
}

IMPLEMENT_MODULE(FMHAudioModule, MHAudio)
