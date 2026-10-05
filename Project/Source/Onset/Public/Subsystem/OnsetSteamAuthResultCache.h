#pragma once

#include "CoreMinimal.h"

/** Keeps callback results until the corresponding player reaches PostLogin. */
class FOnsetSteamAuthResultCache
{
public:
	void Record(const FString& UserId, bool bSuccess) { Results.Add(UserId, bSuccess); }
	bool Consume(const FString& UserId, bool& bSuccess) { return Results.RemoveAndCopyValue(UserId, bSuccess); }
	void Forget(const FString& UserId) { Results.Remove(UserId); }
	void Reset() { Results.Empty(); }

private:
	TMap<FString, bool> Results;
};
