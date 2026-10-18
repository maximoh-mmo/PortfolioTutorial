#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "OnsetAppearanceIntegrationAuditCommandlet.generated.h"
UCLASS()
class UOnsetAppearanceIntegrationAuditCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UOnsetAppearanceIntegrationAuditCommandlet();
    virtual int32 Main(const FString& Params) override;
};
