// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OnsetAppearanceTypes.h"
#include "OnsetPlayerDataTypes.h"
#include "OnsetAppearancePreviewRig.generated.h"

class USceneCaptureComponent2D;
class UPointLightComponent;
class UDirectionalLightComponent;
class UTextureRenderTarget2D;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UImage;
class UScaleBox;
class UCanvasPanel;
class UWidgetTree;
class AOnsetAppearancePreviewActor;
class UOnsetAppearanceCatalog;
class UAnimSequence;

/**
 * Reusable 3D capture rig for character preview (used by Character Creation and Character Select slots).
 * Owns a USceneCaptureComponent2D, an isolated lighting studio, an AOnsetAppearancePreviewActor,
 * and handles rendering directly into a UCanvasPanel via a dynamic material instance.
 */
UCLASS()
class ONSET_API AOnsetAppearancePreviewRig : public AActor
{
	GENERATED_BODY()

public:
	AOnsetAppearancePreviewRig();

	/** Factory helper to spawn and configure a rig in the given world. */
	static AOnsetAppearancePreviewRig* SpawnPreviewRig(
		UWorld* World,
		const FVector& WorldLocation,
		const FIntPoint& Resolution = FIntPoint(512, 640),
		TSoftObjectPtr<UMaterialInterface> InPreviewMaterial = nullptr,
		UOnsetAppearanceCatalog* InCatalog = nullptr
	);

	/** Attaches or binds the dynamic preview image into the given CanvasPanel. */
	bool AttachToCanvas(UCanvasPanel* TargetCanvas, UWidgetTree* InWidgetTree = nullptr);

	/** Applies an appearance recipe to the preview actor. */
	bool ApplyRecipe(const FOnsetAppearanceRecipe& InRecipe, EOnsetAppearanceUse InUse = EOnsetAppearanceUse::Player);

	/** Decodes JSON appearance or falls back to the class default preset. */
	bool ApplyAppearanceFromJSON(const FString& InJSON, EOnsetCharacterClass FallbackClass = EOnsetCharacterClass::DPS);

	/** Assigns and optionally plays a preview animation sequence. */
	void SetAnimation(UAnimSequence* InAnimation, bool bAutoPlay = true);

	/** Enables or disables capture and image visibility. */
	void SetActive(bool bInActive);

	virtual void Tick(float DeltaTime) override;

	/** Updates the camera location, rotation, and field of view relative to the rig root. Smoothly interpolates unless bImmediate is true. */
	void SetCameraSettings(const FVector& InRelativeLocation, const FRotator& InRelativeRotation, float InFOV, bool bImmediate = false);

	/** Sets camera interpolation speed (exponential ease-out, default 5.0f). */
	void SetCameraInterpSpeed(float InSpeed) { CameraInterpSpeed = InSpeed; }
	float GetCameraInterpSpeed() const { return CameraInterpSpeed; }
	bool IsInterpolatingCamera() const { return bIsInterpolatingCamera; }

	/** Rotates the preview character by the specified yaw delta. */
	void SetCharacterRotation(const FRotator& InRotation);

	void AddCharacterYaw(float DeltaYaw);

	/** Accessors */
	AOnsetAppearancePreviewActor* GetPreviewActor() const { return PreviewActor; }
	USceneCaptureComponent2D* GetCaptureComponent() const { return PreviewCaptureComponent; }
	UTextureRenderTarget2D* GetRenderTarget() const { return PreviewRenderTarget; }
	UMaterialInstanceDynamic* GetMaterialInstance() const { return PreviewMaterialInstance; }
	UImage* GetPreviewImage() const { return PreviewImage; }
	UScaleBox* GetScaleBox() const { return PreviewScaleBox; }

	/** Default idle animation clip to play on the preview actor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Onset|Preview")
	TSoftObjectPtr<UAnimSequence> DefaultIdleAnimation = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Characters/Humanoid/Adult/Anims/Unarmed/MM_Idle_F_Adult.MM_Idle_F_Adult")));

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void InitializeRig(const FIntPoint& Resolution, TSoftObjectPtr<UMaterialInterface> InPreviewMaterial, UOnsetAppearanceCatalog* InCatalog);

	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<USceneComponent> RigRoot;

	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<USceneCaptureComponent2D> PreviewCaptureComponent;

	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<UPointLightComponent> CameraLight;

	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<UDirectionalLightComponent> KeyLight;

	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<UDirectionalLightComponent> FillLight;

	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<UDirectionalLightComponent> RimLight;

	UPROPERTY(VisibleAnywhere, Category = "Preview")
	TObjectPtr<UDirectionalLightComponent> BounceLight;

	UPROPERTY(Transient)
	TObjectPtr<AOnsetAppearancePreviewActor> PreviewActor;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> PreviewRenderTarget;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PreviewMaterialInstance;

	UPROPERTY(Transient)
	TObjectPtr<UScaleBox> PreviewScaleBox;

	UPROPERTY(Transient)
	TObjectPtr<UImage> PreviewImage;

	UPROPERTY(Transient)
	TObjectPtr<UOnsetAppearanceCatalog> Catalog;

	UPROPERTY(EditDefaultsOnly, Category = "Preview|Camera")
	float CameraInterpSpeed = 5.0f;

	FVector CurrentCameraLocation = FVector(290.f, 0.f, 162.f);
	FRotator CurrentCameraRotation = FRotator(-13.5f, 180.f, 0.f);
	float CurrentCameraFOV = 32.f;

	FVector TargetCameraLocation = FVector(290.f, 0.f, 162.f);
	FRotator TargetCameraRotation = FRotator(-13.5f, 180.f, 0.f);
	float TargetCameraFOV = 32.f;

	bool bIsInterpolatingCamera = false;
	bool bIsActive = true;
};
