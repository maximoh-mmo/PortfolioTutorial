// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "OnsetGameModeBase.generated.h"

class UOnsetScreenBase;
class UOnsetRootLayout;
DECLARE_LOG_CATEGORY_EXTERN(LogSteamAuth, Log, All);

/** Default game mode. Sets the default pawn class OnsetPlayerCharacter and the player controller class. */
UCLASS()
class ONSET_API AOnsetGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	AOnsetGameModeBase();

	virtual void StartPlay() override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UOnsetRootLayout> RootLayoutClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UOnsetScreenBase> MainMenuScreenClass;

	/** Save all connected players' state, then ServerTravel to the target map. */
	void TravelToZone(const FString& MapName, const FString& EntryPoint = TEXT(""));

	/** Exec command: TravelZone <MapName> [EntryPoint] */
	UFUNCTION(Exec)
	void TravelZone(const FString& MapName, const FString& EntryPoint = TEXT(""));

	void ValidateAuthTicket(APlayerController* NewPlayer, const FString& AuthTicket);
};
