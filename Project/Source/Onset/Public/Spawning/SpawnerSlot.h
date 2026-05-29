#pragma once
#include "CoreMinimal.h"
#include "SpawnerSlot.generated.h"

class AOnsetEnemy;

USTRUCT()                                                                                  
struct FSpawnerSlot                                                                                         
{
	GENERATED_BODY();
	
	FTransform SpawnTransform;                                                                            
	TObjectPtr<AOnsetEnemy> Occupant = nullptr;                                                                        
};                                                                                                          