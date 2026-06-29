#pragma once

#include "CoreMinimal.h"

class APlayerController;

struct IPlatformAuth
{
	virtual ~IPlatformAuth() = default;

	virtual bool ValidateTicket(APlayerController* PC, const FString& Ticket) = 0;

	virtual FString GetPlatformID(APlayerController* PC) = 0;

	virtual FString GetPlatformName() const = 0;
};
