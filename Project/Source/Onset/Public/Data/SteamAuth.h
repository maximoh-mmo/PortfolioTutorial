#pragma once

#include "CoreMinimal.h"
#include "Data/IPlatformAuth.h"

class FSteamAuth : public IPlatformAuth
{
public:
	FSteamAuth() = default;
	virtual ~FSteamAuth() override = default;

	FSteamAuth(const FSteamAuth&) = delete;
	FSteamAuth& operator=(const FSteamAuth&) = delete;

	virtual bool ValidateTicket(APlayerController* PC, const FString& Ticket) override;
	virtual FString GetPlatformID(APlayerController* PC) override;
	virtual FString GetPlatformName() const override;
};
