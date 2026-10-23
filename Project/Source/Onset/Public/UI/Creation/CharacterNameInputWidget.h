// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "CharacterNameInputWidget.generated.h"

class UEditableText;
class UCommonTextBlock;
class UBorder;
class UMHTerminalInputController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCreationNameChanged, const FString&, NewName, bool, bIsValid);

/**
 * Terminal-style text input field with blinking cursor, placeholder handling,
 * character limiting, and platform virtual keyboard integration.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ONSET_API UCharacterNameInputWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Onset|Callsign")
	FOnCreationNameChanged OnNameChanged;

	UFUNCTION(BlueprintPure, Category = "Onset|Callsign")
	FString GetTrimmedName() const;

	UFUNCTION(BlueprintCallable, Category = "Onset|Callsign")
	void SetDefaultName(const FString& InDefaultName);

	UFUNCTION(BlueprintPure, Category = "Onset|Callsign")
	bool IsNameValid() const;

	UFUNCTION(BlueprintPure, Category = "Onset|Callsign")
	UMHTerminalInputController* GetTerminalController() const { return TerminalController; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Callsign")
	FString DefaultName = TEXT("Kael");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Callsign")
	int32 MinCharacters = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Callsign")
	int32 MaxCharacters = 16;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Callsign")
	float BlinkInterval = 0.5f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UFUNCTION()
	virtual void HandleTerminalTextChanged(const FString& NewText, bool bIsPlaceholder);

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|Callsign")
	TObjectPtr<UEditableText> InputField;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|Callsign")
	TObjectPtr<UCommonTextBlock> DisplayText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|Callsign")
	TObjectPtr<UCommonTextBlock> CursorText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|Callsign")
	TObjectPtr<UBorder> DefaultBorder;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|Callsign")
	TObjectPtr<UBorder> CursorBorder;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|Callsign")
	TObjectPtr<UCommonTextBlock> CharCountText;

private:
	void UpdateCharCount(int32 CurrentLen);

	UPROPERTY(Transient)
	TObjectPtr<UMHTerminalInputController> TerminalController;
};
