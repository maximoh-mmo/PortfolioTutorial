#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "OnsetAppearanceCatalog.h"
#include "OnsetAppearanceComponent.generated.h"

class USkeletalMeshComponent;
struct FStreamableHandle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnsetAppearanceReady);

/** Portable Onset visual assembly. No legacy creator Blueprint or account dependencies. */
UCLASS(ClassGroup=(Onset), meta=(BlueprintSpawnableComponent))
class ONSETAPPEARANCE_API UOnsetAppearanceComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    UOnsetAppearanceComponent();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance") TObjectPtr<UOnsetAppearanceCatalog> Catalog;
    UPROPERTY(BlueprintAssignable, Category="Appearance") FOnsetAppearanceReady OnAppearanceReady;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Appearance") TArray<FOnsetAppearanceIssue> LastIssues;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Appearance") FOnsetAppearanceRecipe AppliedRecipe;

    UFUNCTION(BlueprintCallable, Category="Onset|Appearance")
    bool ApplyRecipe(const FOnsetAppearanceRecipe& Recipe, EOnsetAppearanceUse Use = EOnsetAppearanceUse::AuthoredCharacter);
    UFUNCTION(BlueprintCallable, Category="Onset|Appearance")
    bool ApplyRecipeAsync(const FOnsetAppearanceRecipe& Recipe, EOnsetAppearanceUse Use = EOnsetAppearanceUse::AuthoredCharacter);
    UFUNCTION(BlueprintCallable, Category="Onset|Appearance") void CancelPendingBuild();
    UFUNCTION(BlueprintPure, Category="Onset|Appearance") USkeletalMeshComponent* GetBodyMesh() const { return BodyMesh; }
    UFUNCTION(BlueprintPure, Category="Onset|Appearance") USkeletalMeshComponent* GetHeadMesh() const { return HeadMesh; }
    /** Refresh attachments/followers and restore recipe morphs after deterministic preview pose evaluation. */
    void RefreshPreviewPose();
    /** Borrow an ACharacter mesh; the component never destroys it or changes its transform. */
    void BindExternalBodyMesh(USkeletalMeshComponent* Mesh);

protected:
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
private:
    UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> BodyMesh;
    UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> HeadMesh;
    UPROPERTY(Transient) TArray<TObjectPtr<USkeletalMeshComponent>> BuiltMeshes;
    UPROPERTY(Transient) TMap<FName, TObjectPtr<USkeletalMeshComponent>> PartMeshes;
    TSharedPtr<FStreamableHandle> PendingLoad;
    uint64 BuildRevision = 0;
    TWeakObjectPtr<USkeletalMeshComponent> ExternalBodyMesh;
    UPROPERTY(Transient) TObjectPtr<UClass> ExternalAnimationClass;
    bool BuildLoaded(const FOnsetAppearanceRecipe& Recipe);
    void UpdateParameters(const FOnsetAppearanceRecipe& Recipe);
    USkeletalMeshComponent* MakeMesh(USkeletalMesh* Mesh, USceneComponent* Parent, FName Socket, const FTransform& Relative);
    void ClearMeshes();
};
