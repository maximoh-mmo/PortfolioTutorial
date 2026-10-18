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
class UViewport;

UCLASS(Abstract)
class ONSET_API UCharacterCreationScreen : public UOnsetScreenBase
{
	GENERATED_BODY()

public:
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

protected:
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AOnsetPlayerController> CachedPlayerController;

	UPROPERTY(BlueprintReadOnly)
	int32 SlotIndex = -1;

    UPROPERTY(BlueprintReadOnly, Category="Onset|Character Creation")
    FOnsetAppearanceRecipe DraftAppearance;

private:
    UPROPERTY(Transient) TObjectPtr<UOnsetAppearanceCatalog> AppearanceCatalog;
    UPROPERTY(Transient) TObjectPtr<UScrollBox> OptionsScroll;
    UPROPERTY(Transient) TObjectPtr<UVerticalBox> OptionsList;
    UPROPERTY(Transient) TObjectPtr<UCommonTextBlock> FeedbackText;
    UPROPERTY(Transient) TObjectPtr<UViewport> PreviewViewport;
    UPROPERTY(Transient) TObjectPtr<AOnsetAppearancePreviewActor> PreviewActor;
    UPROPERTY(Transient) TObjectPtr<UCommonButtonBase> NativeCreateButton;
    bool bDraftReady = false;
    bool bRequestPending = false;

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
