#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OnsetAppearanceTypes.h"
#include "OnsetAppearancePreviewActor.generated.h"

class UOnsetAppearanceComponent;
class UOnsetAppearanceCatalog;
class UOnsetAppearancePreset;
class UAnimSequence;
class USkeletalMeshComponent;

UCLASS()
class ONSETAPPEARANCE_API AOnsetAppearancePreviewActor : public AActor
{
    GENERATED_BODY()
public:
    AOnsetAppearancePreviewActor();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Appearance") TObjectPtr<UOnsetAppearanceComponent> Appearance;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance") TSoftObjectPtr<UOnsetAppearanceCatalog> Catalog;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance") TSoftObjectPtr<UOnsetAppearancePreset> Preset;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance") FOnsetAppearanceRecipe Recipe;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance") EOnsetAppearanceUse Use = EOnsetAppearanceUse::AuthoredCharacter;
    UFUNCTION(CallInEditor, BlueprintCallable, Category="Appearance") bool RebuildPreview();
    UFUNCTION(CallInEditor, BlueprintCallable, Category="Appearance") bool LoadPreset();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation Preview") TSoftObjectPtr<UAnimSequence> PreviewAnimation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation Preview") TArray<TSoftObjectPtr<UAnimSequence>> SampleAnimations;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation Preview", meta=(ClampMin="-4", ClampMax="4")) float PlaybackSpeed = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation Preview", meta=(ClampMin="0")) float PreviewTime = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation Preview") bool bLoopAnimation = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation Preview") bool bAutoPlay = true;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Animation Preview") bool bPreviewPlaying = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Animation Preview") float AnimationLength = 0.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Animation Preview") FString AnimationStatus;

    UFUNCTION(CallInEditor, BlueprintCallable, Category="Animation Preview") bool PlayPreview();
    UFUNCTION(CallInEditor, BlueprintCallable, Category="Animation Preview") void PausePreview();
    UFUNCTION(CallInEditor, BlueprintCallable, Category="Animation Preview") bool RestartPreview();
    UFUNCTION(CallInEditor, BlueprintCallable, Category="Animation Preview") void StopPreview();
    UFUNCTION(CallInEditor, BlueprintCallable, Category="Animation Preview") bool ScrubPreview();
    UFUNCTION(CallInEditor, BlueprintCallable, Category="Animation Preview") void ShowReferencePose();
    UFUNCTION(CallInEditor, BlueprintCallable, Category="Animation Preview") bool NextSampleAnimation();
    UFUNCTION(CallInEditor, BlueprintCallable, Category="Animation Preview") bool PreviousSampleAnimation();
    UFUNCTION(BlueprintCallable, Category="Animation Preview") bool SetPreviewPosition(float Seconds);
    UFUNCTION(BlueprintPure, Category="Animation Preview") FName GetActiveAnimationName() const;

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual bool ShouldTickIfViewportsOnly() const override { return true; }
protected:
    virtual void BeginPlay() override;
private:
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> ActiveAnimation;
    TWeakObjectPtr<USkeletalMeshComponent> ActiveBody;
    bool bPreviewInitialized = false;
    UFUNCTION() void HandleAppearanceReady();
    bool ConfigureAnimation();
    void EvaluatePreviewPose();
    bool SelectSample(int32 Direction);
};
