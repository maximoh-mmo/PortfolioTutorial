// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/OnsetGameInstance.h"
#include "Misc/CommandLine.h"
#include "OnlineSubsystem.h"

void UOnsetGameInstance::Init()
{
	Super::Init();

#if !UE_BUILD_SHIPPING
	// Development regression hook. The packet handler owns Steam credentials, so a
	// malformed-credential test must use its native test control before travelling.
	if (FParse::Param(FCommandLine::Get(), TEXT("MHPacketAuthBadKey")))
	{
		if (IOnlineSubsystem* SteamSubsystem = IOnlineSubsystem::Get(TEXT("Steam")))
		{
			SteamSubsystem->Exec(GetWorld(), TEXT("AUTH BADKEY"), *GLog);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("MHPacketAuthBadKey requested but the Steam subsystem is unavailable"));
		}
	}
#endif
}
