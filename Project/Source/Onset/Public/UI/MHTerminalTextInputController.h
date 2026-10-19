// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Types/SlateEnums.h"
#include "MHTerminalTextInputController.generated.h"

class UBorder;
class UCommonTextBlock;
class UEditableText;
class UHorizontalBox;
class UUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMHTerminalTextChanged, const FString&, NewText, bool, bIsPlaceholder);

/**
 * Controller for terminal-style text input fields.
 * Manages placeholder text, dynamic trailing cursor (_), first-keystroke clear,
 * backspace restoration, gamepad virtual keyboard trigger, and Slate presentation layering.
 *
 * Designed for modular extraction to MHCore / MHUI.
 */
UCLASS(BlueprintType)
class ONSET_API UMHTerminalInputController : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Configures and activates terminal text input on the given widget set.
	 *
	 * @param InOwnerWidget       The parent UserWidget owning the controls.
	 * @param InInputField        The underlying UEditableText capturing keystrokes and focus.
	 * @param InDisplayTextBlock  The text block displaying the current callsign / placeholder.
	 * @param InCursorTextBlock   The text block displaying the terminal cursor ('_').
	 * @param InContainerBorder   The border containing the display presentation.
	 * @param InPlaceholderText   Default placeholder text (e.g. "Kael").
	 * @param InBlinkInterval     Blink rate in seconds (default: 0.5s).
	 * @param InMaxCharacters     Max character limit (default: 16, 0 for unlimited).
	 */
	UFUNCTION(BlueprintCallable, Category = "MH|UI|TerminalInput")
	void Initialize(
		UUserWidget* InOwnerWidget,
		UEditableText* InInputField,
		UCommonTextBlock* InDisplayTextBlock,
		UCommonTextBlock* InCursorTextBlock,
		UBorder* InDefaultBorder,
		UBorder* InCursorBorder,
		const FString& InPlaceholderText = TEXT("Kael"),
		float InBlinkInterval = 0.5f,
		int32 InMaxCharacters = 16
	);

	/** Shuts down timers and unbinds delegates. */
	UFUNCTION(BlueprintCallable, Category = "MH|UI|TerminalInput")
	void Shutdown();

	/** Updates the default placeholder text. If no user text has been entered, immediately refreshes display. */
	UFUNCTION(BlueprintCallable, Category = "MH|UI|TerminalInput")
	void SetPlaceholderText(const FString& NewPlaceholder);

	/** Returns the final character name: typed name if set, or default placeholder if empty. */
	UFUNCTION(BlueprintPure, Category = "MH|UI|TerminalInput")
	FString GetFinalText() const;

	/** Returns true if the field is currently displaying the placeholder. */
	UFUNCTION(BlueprintPure, Category = "MH|UI|TerminalInput")
	bool IsPlaceholderActive() const { return bIsPlaceholderActive; }

	/** Focuses the input field and displays the platform virtual keyboard (for gamepad / console). */
	UFUNCTION(BlueprintCallable, Category = "MH|UI|TerminalInput")
	void OpenVirtualKeyboard();

	/** Sets visual focus styling on the container. */
	UFUNCTION(BlueprintCallable, Category = "MH|UI|TerminalInput")
	void SetFocused(bool bInFocused);

	/** Returns the underlying input field widget. */
	UFUNCTION(BlueprintPure, Category = "MH|UI|TerminalInput")
	UEditableText* GetInputField() const { return InputField.Get(); }

	/** Delegate for when the final resolved text changes. */
	UPROPERTY(BlueprintAssignable, Category = "MH|UI|TerminalInput")
	FOnMHTerminalTextChanged OnTerminalTextChanged;

protected:
	UFUNCTION()
	void HandleTextChanged(const FText& InText);

	UFUNCTION()
	void HandleTextCommitted(const FText& InText, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void HandleBlinkTimer();

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<UUserWidget> OwnerWidget;

	UPROPERTY(Transient)
	TWeakObjectPtr<UEditableText> InputField;

	UPROPERTY(Transient)
	TWeakObjectPtr<UCommonTextBlock> DisplayTextBlock;

	UPROPERTY(Transient)
	TWeakObjectPtr<UCommonTextBlock> CursorTextBlock;

	UPROPERTY(Transient)
	TWeakObjectPtr<UBorder> DefaultBorder;

	UPROPERTY(Transient)
	TWeakObjectPtr<UBorder> CursorBorder;

	UPROPERTY(EditAnywhere, Category = "MH|UI|TerminalInput")
	FString PlaceholderText = TEXT("Kael");

	UPROPERTY(EditAnywhere, Category = "MH|UI|TerminalInput")
	float BlinkInterval = 0.5f;

	UPROPERTY(EditAnywhere, Category = "MH|UI|TerminalInput")
	int32 MaxCharacters = 16;

	UPROPERTY(EditAnywhere, Category = "MH|UI|TerminalInput")
	FLinearColor PlaceholderColor = FLinearColor(1.f, 1.f, 1.f, 0.55f);

	UPROPERTY(EditAnywhere, Category = "MH|UI|TerminalInput")
	FLinearColor ActiveColor = FLinearColor(1.f, 1.f, 1.f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "MH|UI|TerminalInput")
	FString CursorChar = TEXT("_");

	UPROPERTY(EditAnywhere, Category = "MH|UI|TerminalInput")
	float CursorPadding = 2.0f;

	float CursorPosY = -40.0f;

	bool bIsPlaceholderActive = true;
	bool bBlinkVisible = true;
	bool bIsFocused = false;

	FTimerHandle BlinkTimerHandle;

	float CalculateTextWidth(const FString& InStringToMeasure) const;
	void UpdateCursorPosition();
	void UpdateDisplayPresentation();
	void ResetBlink();
};
