// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Creation/CharacterCreationStageWidget.h"
#include "UI/OnsetAppearancePreviewRig.h"
#include "Player/OnsetCharacterAppearanceComponent.h"
#include "OnsetAppearanceCatalog.h"
#include "Components/CanvasPanel.h"
#include "CommonButtonBase.h"
#include "Animation/AnimSequence.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/WidgetTree.h"

UCharacterCreationStageWidget::UCharacterCreationStageWidget()
{
	bHasScriptImplementedTick = false;

	PreviewMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/M_CharacterPreview.M_CharacterPreview")));
	DefaultIdleAnimation = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Characters/Humanoid/Adult/Anims/Unarmed/MM_Idle_F_Adult.MM_Idle_F_Adult")));
}

void UCharacterCreationStageWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!AppearanceCatalog)
	{
		AppearanceCatalog = UOnsetCharacterAppearanceComponent::LoadDefaultCatalog();
	}

	EnsureRig();

	if (ResetAngleButton)
	{
		ResetAngleButton->OnClicked().AddUObject(this, &ThisClass::ResetTurntableRotation);
	}

	if (FullBodyCameraButton)
	{
		FullBodyCameraButton->OnClicked().AddWeakLambda(this, [this]()
		{
			SetCameraFraming(false);
		});
	}

	if (FaceCameraButton)
	{
		FaceCameraButton->OnClicked().AddWeakLambda(this, [this]()
		{
			SetCameraFraming(true);
		});
	}

	SetCameraFraming(false);
}

void UCharacterCreationStageWidget::NativeDestruct()
{
	bIsDraggingPreview = false;

	if (RigActor)
	{
		RigActor->Destroy();
		RigActor = nullptr;
	}

	Super::NativeDestruct();
}

void UCharacterCreationStageWidget::EnsureRig()
{
	if (RigActor || !CharacterPreviewCanvas || !GetWorld())
	{
		return;
	}

	RigActor = AOnsetAppearancePreviewRig::SpawnPreviewRig(
		GetWorld(),
		FVector(0.f, 0.f, -10000.f),
		FIntPoint(1024, 1280),
		PreviewMaterial,
		AppearanceCatalog
	);

	if (RigActor)
	{
		RigActor->AttachToCanvas(CharacterPreviewCanvas, WidgetTree);

		UAnimSequence* Anim = DefaultIdleAnimation.IsValid() ? DefaultIdleAnimation.Get() : DefaultIdleAnimation.LoadSynchronous();
		if (!Anim)
		{
			Anim = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Humanoid/Adult/Anims/Unarmed/MM_Idle_F_Adult.MM_Idle_F_Adult"));
		}

		if (Anim)
		{
			RigActor->SetAnimation(Anim, true);
		}
	}
}

void UCharacterCreationStageWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!RigActor)
	{
		EnsureRig();
	}

	APlayerController* PC = GetOwningPlayer();
	if (PC && RigActor)
	{
		const float RightX = PC->GetInputAnalogKeyState(EKeys::Gamepad_RightX);
		if (FMath::Abs(RightX) > 0.15f)
		{
			const float YawDelta = -RightX * GamepadRotationSpeed * InDeltaTime;
			RigActor->AddCharacterYaw(YawDelta);
		}
	}
}

FReply UCharacterCreationStageWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && CharacterPreviewCanvas)
	{
		if (CharacterPreviewCanvas->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
		{
			bIsDraggingPreview = true;
			return FReply::Handled().CaptureMouse(TakeWidget());
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UCharacterCreationStageWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsDraggingPreview)
	{
		bIsDraggingPreview = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UCharacterCreationStageWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDraggingPreview && RigActor)
	{
		const float DeltaX = InMouseEvent.GetCursorDelta().X;
		if (FMath::Abs(DeltaX) > KINDA_SMALL_NUMBER)
		{
			RigActor->AddCharacterYaw(-DeltaX * MouseDragSensitivity);
			return FReply::Handled();
		}
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UCharacterCreationStageWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	bIsDraggingPreview = false;
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
}

void UCharacterCreationStageWidget::SetAppearanceRecipe(const FOnsetAppearanceRecipe& Recipe)
{
	EnsureRig();

	if (RigActor)
	{
		RigActor->ApplyRecipe(Recipe, EOnsetAppearanceUse::Player);
	}
}

void UCharacterCreationStageWidget::SetCameraFraming(bool bFaceCloseUp)
{
	bIsFaceFraming = bFaceCloseUp;

	EnsureRig();

	if (RigActor)
	{
		if (bFaceCloseUp)
		{
			RigActor->SetCameraSettings(FaceCameraOffset, FaceCameraRotation, FaceFOV);
		}
		else
		{
			RigActor->SetCameraSettings(FullBodyCameraOffset, FullBodyCameraRotation, FullBodyFOV);
		}
	}

	if (FullBodyCameraButton)
	{
		FullBodyCameraButton->SetIsSelected(!bFaceCloseUp);
	}

	if (FaceCameraButton)
	{
		FaceCameraButton->SetIsSelected(bFaceCloseUp);
	}
}

void UCharacterCreationStageWidget::ResetTurntableRotation()
{
	if (RigActor)
	{
		RigActor->SetCharacterRotation(FRotator::ZeroRotator);
	}
}
