// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "OnsetAppearanceTypes.h"
#include "CharacterCreationStageWidget.generated.h"

class AOnsetAppearancePreviewRig;
class UCanvasPanel;
class UCommonButtonBase;
class UMaterialInterface;
class UAnimSequence;
class UOnsetAppearanceCatalog;

/**
 * Owns the 3D capture rig, turntable mouse/gamepad rotation, camera framing modes, and idle animations.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ONSET_API UCharacterCreationStageWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UCharacterCreationStageWidget();

	UFUNCTION(BlueprintCallable, Category = "Onset|Stage")
	void SetAppearanceRecipe(const FOnsetAppearanceRecipe& Recipe);

	UFUNCTION(BlueprintCallable, Category = "Onset|Stage")
	void SetCameraFraming(bool bFaceCloseUp);

	UFUNCTION(BlueprintCallable, Category = "Onset|Stage")
	void ResetTurntableRotation();

	UFUNCTION(BlueprintPure, Category = "Onset|Stage")
	AOnsetAppearancePreviewRig* GetRig() const { return RigActor; }

	UFUNCTION(BlueprintPure, Category = "Onset|Stage")
	bool IsFaceFraming() const { return bIsFaceFraming; }

	// --- Config Properties ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Preview")
	TSoftObjectPtr<UMaterialInterface> PreviewMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Preview")
	TSoftObjectPtr<UAnimSequence> DefaultIdleAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Camera")
	FVector FullBodyCameraOffset = FVector(290.f, 0.f, 162.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Camera")
	FRotator FullBodyCameraRotation = FRotator(-13.5f, 180.f, 0.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Camera")
	float FullBodyFOV = 32.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Camera")
	FVector FaceCameraOffset = FVector(115.f, 0.f, 168.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Camera")
	FRotator FaceCameraRotation = FRotator(-4.0f, 180.f, 0.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Camera")
	float FaceFOV = 24.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Camera")
	float CameraInterpSpeed = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Turntable")
	float MouseDragSensitivity = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Turntable")
	float GamepadRotationSpeed = 120.f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Onset|Stage")
	TObjectPtr<UCanvasPanel> CharacterPreviewCanvas;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|Stage")
	TObjectPtr<UCommonButtonBase> ResetAngleButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|Stage")
	TObjectPtr<UCommonButtonBase> FullBodyCameraButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Onset|Stage")
	TObjectPtr<UCommonButtonBase> FaceCameraButton;

private:
	void EnsureRig();

	UPROPERTY(Transient)
	TObjectPtr<AOnsetAppearancePreviewRig> RigActor;

	UPROPERTY(Transient)
	TObjectPtr<UOnsetAppearanceCatalog> AppearanceCatalog;

	bool bIsDraggingPreview = false;
	bool bIsFaceFraming = false;
};
