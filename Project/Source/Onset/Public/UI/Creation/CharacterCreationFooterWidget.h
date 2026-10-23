// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "CharacterCreationFooterWidget.generated.h"

class UCommonButtonBase;
class UCommonTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFooterCreateClicked);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFooterBackClicked);

/**
 * Confirmation and back buttons, validation error display, and server RPC progress feedback.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ONSET_API UCharacterCreationFooterWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Onset|Footer")
	FOnFooterCreateClicked OnCreateClicked;

	UPROPERTY(BlueprintAssignable, Category = "Onset|Footer")
	FOnFooterBackClicked OnBackClicked;

	UFUNCTION(BlueprintCallable, Category = "Onset|Footer")
	void SetCreateButtonEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Onset|Footer")
	void SetBusy(bool bIsBusy);

	UFUNCTION(BlueprintCallable, Category = "Onset|Footer")
	void SetFeedbackMessage(const FText& InMessage, bool bIsError = false);

	UFUNCTION(BlueprintPure, Category = "Onset|Footer")
	UCommonButtonBase* GetCreateButton() const { return CreateButton; }

	UFUNCTION(BlueprintPure, Category = "Onset|Footer")
	UCommonButtonBase* GetBackButton() const { return BackButton; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Footer")
	FLinearColor NormalFeedbackColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Footer")
	FLinearColor ErrorFeedbackColor = FLinearColor(0.9f, 0.2f, 0.2f);

protected:
	virtual void NativeOnInitialized() override;

	UFUNCTION()
	virtual void HandleCreateClicked();

	UFUNCTION()
	virtual void HandleBackClicked();

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|Footer")
	TObjectPtr<UCommonButtonBase> BackButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|Footer")
	TObjectPtr<UCommonButtonBase> CreateButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|Footer")
	TObjectPtr<UCommonTextBlock> FeedbackText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|Footer")
	TObjectPtr<UWidget> BusySpinner;
};
