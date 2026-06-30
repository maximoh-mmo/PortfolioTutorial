#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "OnsetPlayerDataTypes.h"
#include "CharacterSelectWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UCanvasPanel;

UCLASS()
class ONSET_API UCharacterSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetAccountData(const FOnsetAccountData& InAccountData);

protected:
	virtual void NativeConstruct() override;

	void BuildUI();
	void RefreshSlots();

	UFUNCTION()
	void OnClickSlot0();

	UFUNCTION()
	void OnClickSlot1();

	UFUNCTION()
	void OnClickSlot2();

	UFUNCTION()
	void OnEnterWorldClicked();

	void HandleSlotClicked(int32 SlotIndex);

	UPROPERTY()
	TObjectPtr<UVerticalBox> SlotContainer;

	UPROPERTY()
	TObjectPtr<UButton> EnterWorldButton;

	UPROPERTY()
	TArray<TObjectPtr<UButton>> ActionButtons;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> NameLabels;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> LevelLabels;

	FOnsetAccountData AccountData;
	int32 SelectedSlot = -1;
};
