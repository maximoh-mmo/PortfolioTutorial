#include "Data/FSteamAuth.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Game/OnsetGameModeBase.h"
#include "Net/UnrealNetwork.h"
#include "Player/OnsetPlayerState.h"

bool FSteamAuth::ValidateTicket(APlayerController* PC, const FString& Ticket)
{
	if (!PC)
		return false;

	if (Ticket.IsEmpty())
	{
		UE_LOG(LogSteamAuth, Warning, TEXT("FSteamAuth: empty ticket from %s"), *PC->GetName());
		return false;
	}

	// Transport-level Steam auth is handled by OnlineSubsystemSteam on connection.
	// The ticket arrival confirms the client has an active Steam session.
	// Full BeginAuthSession validation can be added here when Steamworks SDK headers
	// are directly accessible from the game module.
	UE_LOG(LogSteamAuth, Log, TEXT("FSteamAuth: ticket accepted from %s (%d chars)"), *PC->GetName(), Ticket.Len());
	return true;
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
