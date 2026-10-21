// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/CharacterSlot.h"

#include "CommonButtonBase.h"
#include "CommonTextBlock.h"
#include "Components/CanvasPanel.h"
#include "UI/OnsetAppearancePreviewRig.h"
#include "OnsetAppearancePreviewActor.h"
#include "Animation/AnimSequence.h"

void UCharacterSlot::SetSlotIndex(int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;
	Refresh();
}

void UCharacterSlot::SetSlotData(const FOnsetCharacterSlotData& InSlotData)
{
	SlotData = InSlotData;
	Refresh();
}

FText UCharacterSlot::GetDisplayName() const
{
	return IsOccupied() ? FText::FromString(SlotData.CharacterName) : EmptySlotLabel;
}

FText UCharacterSlot::GetDisplayLevel() const
{
	if (!IsOccupied())
	{
		return FText::GetEmpty();
	}

	return FText::Format(NSLOCTEXT("CharacterSlot", "LevelFormat", "Level {0}"), FText::AsNumber(SlotData.Level));
}

void UCharacterSlot::HandleSlotClicked()
{
	OnSlotActivated.Broadcast(SlotIndex);
}

void UCharacterSlot::HandleDeleteClicked()
{
	OnDeleteRequested.Broadcast(SlotIndex);
}

void UCharacterSlot::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (InteractionButton)
	{
		InteractionButton->OnClicked().AddUObject(this, &UCharacterSlot::HandleSlotClicked);
	}

	if (DeleteButton)
	{
		DeleteButton->SetIsFocusable(false);
		DeleteButton->OnClicked().AddUObject(this, &UCharacterSlot::HandleDeleteClicked);
	}

	Refresh();
}

void UCharacterSlot::NativeConstruct()
{
	Super::NativeConstruct();

	if (!CharacterPreview)
	{
		CharacterPreview = Cast<UCanvasPanel>(GetWidgetFromName(TEXT("CharacterPreview")));
	}

	if (IsOccupied())
	{
		EnsurePreview();
		RefreshPreview();
	}
}

void UCharacterSlot::NativeDestruct()
{
	DestroyPreview();
	Super::NativeDestruct();
}

void UCharacterSlot::Refresh()
{
	const bool bOccupied = IsOccupied();

	if (NameLabel)
	{
		NameLabel->SetText(GetDisplayName());
	}

	if (LevelLabel)
	{
		LevelLabel->SetText(GetDisplayLevel());
		LevelLabel->SetVisibility(bOccupied ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}

	if (DeleteButton)
	{
		DeleteButton->SetVisibility(bOccupied ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}

	if (!CharacterPreview)
	{
		CharacterPreview = Cast<UCanvasPanel>(GetWidgetFromName(TEXT("CharacterPreview")));
	}

	if (CharacterPreview)
	{
		if (bOccupied)
		{
			EnsurePreview();
			RefreshPreview();
		}
		else if (PreviewRig)
		{
			PreviewRig->SetActive(false);
		}
	}

	BP_OnSlotDataChanged();
}

void UCharacterSlot::EnsurePreview()
{
	if (!IsOccupied() || !GetWorld())
	{
		return;
	}

	if (!CharacterPreview)
	{
		CharacterPreview = Cast<UCanvasPanel>(GetWidgetFromName(TEXT("CharacterPreview")));
	}

	if (!CharacterPreview)
	{
		return;
	}

	if (!PreviewRig)
	{
		// If the Blueprint retained legacy defaults, upgrade to eye-height full-body framing
		if ((CameraOffset.Equals(FVector(240.f, 0.f, 95.f), 1.0f) && CameraRotation.Equals(FRotator(0.f, 180.f, 0.f), 1.0f)) ||
		    (CameraOffset.Equals(FVector(280.f, 0.f, 130.f), 1.0f) && CameraRotation.Equals(FRotator(-7.5f, 180.f, 0.f), 1.0f)))
		{
			CameraOffset = FVector(290.f, 0.f, 162.f);
			CameraRotation = FRotator(-13.5f, 180.f, 0.f);
			CameraFOV = 32.f;
		}

		// Space rigs out in world space (1500 units apart, deep below the world) so multiple slots never intersect
		const FVector RigLocation(SlotIndex * 1500.f, 0.f, -10000.f);
		PreviewRig = AOnsetAppearancePreviewRig::SpawnPreviewRig(
			GetWorld(),
			RigLocation,
			RenderTargetResolution,
			PreviewMaterial
		);

		if (PreviewRig)
		{
			PreviewRig->SetCameraSettings(CameraOffset, CameraRotation, CameraFOV);
			PreviewRig->SetCharacterRotation(CharacterRotation);
			PreviewRig->AttachToCanvas(CharacterPreview, WidgetTree);
		}
	}
}

void UCharacterSlot::RefreshPreview()
{
	EnsurePreview();
	if (!PreviewRig)
	{
		return;
	}

	PreviewRig->SetActive(true);
	PreviewRig->ApplyAppearanceFromJSON(SlotData.AppearanceJSON, SlotData.CharacterClass);

	UAnimSequence* Anim = IdleAnimation.IsValid() ? IdleAnimation.Get() : IdleAnimation.LoadSynchronous();
	if (!Anim)
	{
		Anim = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Humanoid/Adult/Anims/Unarmed/MM_Idle_F_Adult.MM_Idle_F_Adult"));
	}
	if (Anim)
	{
		PreviewRig->SetAnimation(Anim, true);
	}
}

void UCharacterSlot::DestroyPreview()
{
	if (PreviewRig)
	{
		PreviewRig->Destroy();
		PreviewRig = nullptr;
	}
}

void UCharacterSlot::SetPreviewActive(bool bActive)
{
	if (PreviewRig)
	{
		PreviewRig->SetActive(bActive && IsOccupied());
	}
}

void UCharacterSlot::AddPreviewYaw(float YawDelta)
{
	if (PreviewRig)
	{
		PreviewRig->AddCharacterYaw(YawDelta);
	}
}

AOnsetAppearancePreviewActor* UCharacterSlot::GetPreviewActor() const
{
	return PreviewRig ? PreviewRig->GetPreviewActor() : nullptr;
}
