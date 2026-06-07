#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OnsetCorpse.generated.h"

UCLASS()
class ONSET_API AOnsetCorpse : public AActor
{
	GENERATED_BODY()

public:
	AOnsetCorpse();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> MeshComponent;
};
	