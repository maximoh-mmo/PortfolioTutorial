#include "Data/SteamAuth.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Game/OnsetGameModeBase.h"
#include "Net/UnrealNetwork.h"
#include "Player/OnsetPlayerState.h"

bool FSteamAuth::ValidateTicket(APlayerController* PC, const FString& Ticket)
{
	// Gameplay-level tickets are not an authentication boundary. The Steam packet
	// handler validates the connection before a player controller is admitted. Fail
	// closed here so future callers cannot accidentally restore the old empty-string
	// check as an authentication path.
	UE_LOG(LogSteamAuth, Warning, TEXT("FSteamAuth: deprecated gameplay ticket validation was ignored."));
	return false;
}

FString FSteamAuth::GetPlatformID(APlayerController* PC)
{
	if (!PC)
		return TEXT("");

	AOnsetPlayerState* PS = PC->GetPlayerState<AOnsetPlayerState>();
	if (!PS)
		return TEXT("");

	FUniqueNetIdRepl UniqueId = PS->GetUniqueId();
	if (UniqueId.IsValid())
	{
		return UniqueId->ToString();
	}

	UE_LOG(LogSteamAuth, Warning, TEXT("FSteamAuth: no unique net ID for %s"), *PC->GetName());
	return TEXT("");
}

FString FSteamAuth::GetPlatformName() const
{
	return TEXT("Steam");
}
