#include "Game/OnsetLoginServerGameMode.h"

#include "Player/OnsetPlayerController.h"
#include "Player/OnsetPlayerState.h"
#include "Subsystem/OnsetAuthSubsystem.h"
#include "Engine/World.h"
#include "TimerManager.h"

AOnsetLoginServerGameMode::AOnsetLoginServerGameMode()
{
	HUDClass = nullptr;

	static ConstructorHelpers::FClassFinder<APlayerController> ControllerBP(TEXT("/Game/Input/MyOnsetPlayerController.MyOnsetPlayerController_C"));
	if (ControllerBP.Class != nullptr)
	{
		PlayerControllerClass = ControllerBP.Class;
	}
}

void AOnsetLoginServerGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer || !HasAuthority())
		return;

	AOnsetPlayerState* PS = NewPlayer->GetPlayerState<AOnsetPlayerState>();
	if (!PS || PS->SelectedCharacterSlot >= 0)
	{
		KickPlayer(NewPlayer);
		return;
	}

	AOnsetPlayerController* PC = Cast<AOnsetPlayerController>(NewPlayer);
	if (!PC)
	{
		KickPlayer(NewPlayer);
		return;
	}

	UOnsetAuthSubsystem* Auth = GetWorld()->GetSubsystem<UOnsetAuthSubsystem>();
	if (Auth && Auth->GetAuthMode() == EOnsetAuthMode::Token)
	{
		FString Token = Auth->GenerateToken(PS->PlayerPlatform, PS->PlayerPlatformID);
		if (!Token.IsEmpty())
		{
			PC->Client_SessionToken(Token);
			UE_LOG(LogOnsetAuth, Log, TEXT("LoginServer: sent token, kicking %s in %.1fs"),
				*NewPlayer->GetName(), KickDelay);
		}
	}

	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateUObject(this, &AOnsetLoginServerGameMode::KickPlayer, NewPlayer), KickDelay, false);
}

void AOnsetLoginServerGameMode::KickPlayer(APlayerController* Player)
{
	if (Player)
	{
		UE_LOG(LogOnsetAuth, Log, TEXT("LoginServer: kicking %s"), *Player->GetName());
		Player->Destroy();
	}
}
