#pragma once

#include "CoreMinimal.h"
#include "Player/OnsetBaseCharacter.h"
#include "OnsetEnemy.generated.h"

class UGroupComponent;
class UStaticMeshComponent;
class UAIProfile;

UCLASS()
class ONSET_API AOnsetEnemy : public AOnsetBaseCharacter
{
	GENERATED_BODY()

public:
	AOnsetEnemy();

	UFUNCTION(BlueprintCallable, Category = "AI")
	void ApplyProfile(UAIProfile* InProfile);

	UPROPERTY()
	TObjectPtr<UGroupComponent> GroupComp;

	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<UStaticMeshComponent> FallbackMeshComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	TObjectPtr<UAIProfile> Profile;
};
