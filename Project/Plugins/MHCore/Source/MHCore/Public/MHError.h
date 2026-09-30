#pragma once

#include "CoreMinimal.h"

/** Stable categories; diagnostics must not contain credentials or player secrets. */
enum class EMHErrorCode : uint8
{
    Cancelled,
    InvalidArgument,
    NotConfigured,
    Unavailable,
    Timeout,
    Unauthorized,
    NotFound,
    Conflict,
    Internal
};

struct FMHError
{
    EMHErrorCode Code;
    FString Message;
};

/** Success payload for operations with no return value. */
struct FMHUnit {};
