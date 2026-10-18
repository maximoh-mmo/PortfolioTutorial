#pragma once
#include "CoreMinimal.h"
#include "OnsetAppearanceComponent.h"
#include "OnsetAppearancePreset.h"
#include "OnsetCharacterAppearanceComponent.generated.h"

class UAnimSequence;
/** Replicated recipe state; meshes are constructed locally on the character's existing animation mesh. */
UCLASS(ClassGroup=(Onset), meta=(BlueprintSpawnableComponent))
class ONSET_API UOnsetCharacterAppearanceComponent : public UOnsetAppearanceComponent
{
    GENERATED_BODY()
public:
    UOnsetCharacterAppearanceComponent();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance") TSoftObjectPtr<UOnsetAppearancePreset> AuthoredPreset;
    UPROPERTY(ReplicatedUsing=OnRep_AppearanceJSON, BlueprintReadOnly, Category="Appearance") FString AppearanceJSON;
    UFUNCTION(BlueprintCallable, Category="Appearance") bool SetAppearanceRecipe(const FOnsetAppearanceRecipe& Recipe, TArray<FOnsetAppearanceIssue>& Issues);
    void ReapplyAppearance();
    static UOnsetAppearanceCatalog* LoadDefaultCatalog();
    static bool ResolveStoredAppearance(const FString& JSON, FOnsetAppearanceRecipe& Recipe, TArray<FOnsetAppearanceIssue>& Issues);
    UFUNCTION(BlueprintCallable, Category="Appearance")
    static bool GetPlayerPreset(int32 Index, FOnsetAppearanceRecipe& Recipe);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
protected:
    virtual void BeginPlay() override;
    virtual void OnRegister() override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
#endif
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UFUNCTION() void OnRep_AppearanceJSON();
    UFUNCTION() void HandleReady();
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> ChildIdle;
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> ChildRun;
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> ChildAir;
    bool bPendingAppearance = false;
    bool BindCharacterMesh();
    EOnsetAppearanceUse GetUse() const;
};
