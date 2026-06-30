#pragma once

#include "CoreMinimal.h"
#include "IPlayerDataStore.h"

ONSETDATASTORE_API TUniquePtr<IPlayerDataStore> CreateDataStore(const FString& Type, const FString& ConnectionString, bool& bOutSuccess);
