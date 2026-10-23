#pragma once

#include "CoreMinimal.h"
#include "OnsetAppearanceTypes.h"
#include "OnsetPlayerDataTypes.h"
#include "OnsetScreenBase.h"
#include "CharacterCreationScreen.generated.h"

class AOnsetPlayerController;
class AOnsetAppearancePreviewActor;
class UCommonButtonBase;
class UCommonTextBlock;
class UOnsetAppearanceCatalog;
class UPanelWidget;
class UScrollBox;
class UVerticalBox;
class UImage;
class UTextureRenderTarget2D;
class USceneCaptureComponent2D;
class UMHTerminalInputController;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UAnimSequence;

UCLASS(Abstract, Config = Game)
class ONSET_API UCharacterCreationScreen : public UOnsetScreenBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Config, Category = "Onset|Character Creation", meta = (DisplayName = "Default Character Name"))
	FString DefaultCharacterName = TEXT("Kael");

	void SetPlayerController(AOnsetPlayerController* PlayerController);
	void SetSlotIndex(int32 InSlotIndex);

	UFUNCTION(BlueprintCallable)
	void CreateCharacter(const FString& CharacterName, EOnsetCharacterClass CharacterClass, int32 AppearancePresetIndex);
    UFUNCTION(BlueprintCallable, Category="Onset|Character Creation")
    void CreateCharacterWithAppearance(const FString& CharacterName, EOnsetCharacterClass CharacterClass, const FString& AppearanceJSON);

    UFUNCTION(BlueprintPure, Category="Onset|Character Creation")
    FOnsetAppearanceRecipe GetDraftAppearance() const { return DraftAppearance; }

    UFUNCTION(BlueprintCallable, Category="Onset|Character Creation")
    void SelectBodyPreset(int32 PresetIndex);
    UFUNCTION(BlueprintCallable, Category="Onset|Character Creation")
    void SelectHead(FName HeadId);
    UFUNCTION(BlueprintCallable, Category="Onset|Character Creation")
    void SelectPart(FName PartSlot, FName PartId);
    UFUNCTION(BlueprintCallable, Category="Onset|Character Creation")
    void AdjustMorph(FName Morph, bool bFace, float Delta);

    void InitializeTerminalInput();

    UFUNCTION(BlueprintPure, Category = "Onset|Character Creation")
    UMHTerminalInputController* GetTerminalInputController() const { return TerminalInputController; }

protected:
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual UWidget* NativeGetDesiredFocusTarget() const override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
    void CyclePanelFocus(bool bForward);
    void SetupNavigationRules();
    UWidget* GetSelectedClassButton() const;
    TArray<UWidget*> GetClassButtons() const;
    UWidget* GetActiveAppearanceOption() const;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AOnsetPlayerController> CachedPlayerController;

	UPROPERTY(BlueprintReadOnly)
	int32 SlotIndex = -1;

    UPROPERTY(BlueprintReadOnly, Category="Onset|Character Creation")
    FOnsetAppearanceRecipe DraftAppearance;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Onset|Preview")
    TSoftObjectPtr<UMaterialInterface> PreviewMaterial;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Onset|Preview")
    TSoftObjectPtr<UAnimSequence> DefaultIdleAnimation = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Characters/Humanoid/Adult/Anims/Unarmed/MM_Idle_F_Adult.MM_Idle_F_Adult")));

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Onset|Preview")
    FVector FullBodyCameraOffset = FVector(290.f, 0.f, 162.f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Onset|Preview")
    FRotator FullBodyCameraRotation = FRotator(-13.5f, 180.f, 0.f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Onset|Preview")
    float FullBodyFOV = 32.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Onset|Preview")
    FVector FaceCameraOffset = FVector(115.f, 0.f, 168.f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Onset|Preview")
    FRotator FaceCameraRotation = FRotator(-4.0f, 180.f, 0.f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Onset|Preview")
    float FaceFOV = 24.f;

    void SetCameraFraming(bool bFaceCloseUp);

private:
    UPROPERTY(Transient) TObjectPtr<UOnsetAppearanceCatalog> AppearanceCatalog;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> OptionsScroll;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> OptionsList;
    UPROPERTY(Transient) TObjectPtr<UCommonTextBlock> FeedbackText;
    UPROPERTY(Transient) TObjectPtr<UImage> PreviewImage;
    UPROPERTY(Transient) TObjectPtr<UTextureRenderTarget2D> PreviewRenderTarget;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> PreviewMaterialInstance;
    UPROPERTY(Transient) TObjectPtr<AActor> PreviewRigActor;
    UPROPERTY(Transient) TObjectPtr<USceneCaptureComponent2D> PreviewCaptureComponent;
    UPROPERTY(Transient) TObjectPtr<AOnsetAppearancePreviewActor> PreviewActor;
    UPROPERTY(Transient) TObjectPtr<UCommonButtonBase> NativeCreateButton;
    UPROPERTY(Transient) TObjectPtr<UWidget> LastFocusedAppearanceWidget;
    UPROPERTY(Transient) TObjectPtr<UMHTerminalInputController> TerminalInputController;
    bool bDraftReady = false;
    bool bRequestPending = false;
    bool bIsDraggingPreview = false;

    void EnsureDraft();
    void BuildAppearancePanel();
    void EnsurePreview();
    void EnsureCreateButton();
    void HandleNativeCreateClicked();
    void RefreshPreview();
    void CommitDraft(FOnsetAppearanceRecipe Candidate);
    void SetFeedback(const FText& Text);
    UCommonTextBlock* AddLabel(UPanelWidget* Parent, const FText& Text);
    UCommonButtonBase* AddOption(UPanelWidget* Parent, const FText& Label, bool bSelected = false);
    void SetSkinColor(FLinearColor Color);
    void SetEyeColor(FLinearColor Color);
    void SetOutfitColor(int32 Channel, FLinearColor Color);
    UFUNCTION() void HandleAppearanceResult(bool bSuccess, const TArray<FOnsetAppearanceIssue>& Issues);
};
