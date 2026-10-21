// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/OnsetAppearancePreviewRig.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "OnsetAppearancePreviewActor.h"
#include "OnsetAppearanceComponent.h"
#include "OnsetAppearanceCatalog.h"
#include "Player/OnsetCharacterAppearanceComponent.h"
#include "Engine/World.h"

AOnsetAppearancePreviewRig::AOnsetAppearancePreviewRig()
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorEnableCollision(false);

	RigRoot = CreateDefaultSubobject<USceneComponent>(TEXT("RigRoot"));
	SetRootComponent(RigRoot);

	PreviewCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PreviewCapture"));
	PreviewCaptureComponent->SetupAttachment(RigRoot);
	PreviewCaptureComponent->SetRelativeLocationAndRotation(FVector(280.f, 0.f, 130.f), FRotator(-7.5f, 180.f, 0.f));
	PreviewCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	PreviewCaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	PreviewCaptureComponent->bCaptureEveryFrame = true;
	PreviewCaptureComponent->bCaptureOnMovement = false;
	PreviewCaptureComponent->bAlwaysPersistRenderingState = true;
	PreviewCaptureComponent->FOVAngle = 33.f;

	PreviewCaptureComponent->ShowFlags.SetAtmosphere(false);
	PreviewCaptureComponent->ShowFlags.SetFog(false);
	PreviewCaptureComponent->ShowFlags.SetVolumetricFog(false);
	PreviewCaptureComponent->ShowFlags.SetBloom(false);
	PreviewCaptureComponent->ShowFlags.SetEyeAdaptation(false);
	PreviewCaptureComponent->ShowFlags.TemporalAA = false;
	PreviewCaptureComponent->ShowFlags.MotionBlur = false;
	PreviewCaptureComponent->ShowFlags.SetLighting(true);
	PreviewCaptureComponent->ShowFlags.SetLumenGlobalIllumination(false);
	PreviewCaptureComponent->ShowFlags.SetLumenReflections(false);
	PreviewCaptureComponent->ShowFlags.SetAmbientOcclusion(false);

	// 1. Camera-aligned direct front fill light
	CameraLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CameraLight"));
	CameraLight->SetupAttachment(PreviewCaptureComponent);
	CameraLight->SetMobility(EComponentMobility::Movable);
	CameraLight->SetRelativeLocation(FVector(0.f, 0.f, 0.f));
	CameraLight->Intensity = 5000.f;
	CameraLight->AttenuationRadius = 1200.f;
	CameraLight->LightColor = FColor(255, 250, 245);
	CameraLight->CastShadows = false;
	CameraLight->CastDynamicShadows = false;
	CameraLight->CastStaticShadows = false;

	// 2. Key light (front-right, 20 deg down)
	KeyLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("KeyLight"));
	KeyLight->SetupAttachment(RigRoot);
	KeyLight->SetMobility(EComponentMobility::Movable);
	KeyLight->SetRelativeRotation(FRotator(-20.f, 160.f, 0.f));
	KeyLight->Intensity = 5.0f;
	KeyLight->LightColor = FColor(255, 250, 240);
	KeyLight->CastShadows = false;
	KeyLight->CastDynamicShadows = false;
	KeyLight->CastStaticShadows = false;
	KeyLight->bCastVolumetricShadow = false;

	// 3. Fill light (front-left, 10 deg down)
	FillLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("FillLight"));
	FillLight->SetupAttachment(RigRoot);
	FillLight->SetMobility(EComponentMobility::Movable);
	FillLight->SetRelativeRotation(FRotator(-10.f, -160.f, 0.f));
	FillLight->Intensity = 2.5f;
	FillLight->LightColor = FColor(215, 230, 255);
	FillLight->CastShadows = false;
	FillLight->CastDynamicShadows = false;
	FillLight->CastStaticShadows = false;
	FillLight->bCastVolumetricShadow = false;

	// 4. Rim / kicker light (behind character, pointing towards camera edges)
	RimLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("RimLight"));
	RimLight->SetupAttachment(RigRoot);
	RimLight->SetMobility(EComponentMobility::Movable);
	RimLight->SetRelativeRotation(FRotator(-25.f, 20.f, 0.f));
	RimLight->Intensity = 4.0f;
	RimLight->LightColor = FColor(255, 255, 255);
	RimLight->CastShadows = false;
	RimLight->CastDynamicShadows = false;
	RimLight->CastStaticShadows = false;
	RimLight->bCastVolumetricShadow = false;

	// 5. Bounce light (from below front, soft warm bounce)
	BounceLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("BounceLight"));
	BounceLight->SetupAttachment(RigRoot);
	BounceLight->SetMobility(EComponentMobility::Movable);
	BounceLight->SetRelativeRotation(FRotator(40.f, 170.f, 0.f));
	BounceLight->Intensity = 1.0f;
	BounceLight->LightColor = FColor(220, 215, 200);
	BounceLight->CastShadows = false;
	BounceLight->CastDynamicShadows = false;
	BounceLight->CastStaticShadows = false;
	BounceLight->bCastVolumetricShadow = false;
}

AOnsetAppearancePreviewRig* AOnsetAppearancePreviewRig::SpawnPreviewRig(
	UWorld* World,
	const FVector& WorldLocation,
	const FIntPoint& Resolution,
	TSoftObjectPtr<UMaterialInterface> InPreviewMaterial,
	UOnsetAppearanceCatalog* InCatalog)
{
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.bNoFail = true;

	AOnsetAppearancePreviewRig* Rig = World->SpawnActor<AOnsetAppearancePreviewRig>(
		AOnsetAppearancePreviewRig::StaticClass(),
		WorldLocation,
		FRotator::ZeroRotator,
		SpawnParams);

	if (Rig)
	{
		Rig->InitializeRig(Resolution, InPreviewMaterial, InCatalog);
	}

	return Rig;
}

void AOnsetAppearancePreviewRig::InitializeRig(
	const FIntPoint& Resolution,
	TSoftObjectPtr<UMaterialInterface> InPreviewMaterial,
	UOnsetAppearanceCatalog* InCatalog)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	Catalog = InCatalog ? InCatalog : UOnsetCharacterAppearanceComponent::LoadDefaultCatalog();

	// 1. Render target
	if (!PreviewRenderTarget)
	{
		PreviewRenderTarget = NewObject<UTextureRenderTarget2D>(this);
		PreviewRenderTarget->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8_SRGB;
		PreviewRenderTarget->ClearColor = FLinearColor(0.f, 0.f, 0.f, 0.f);
		PreviewRenderTarget->bAutoGenerateMips = false;
		PreviewRenderTarget->InitAutoFormat(Resolution.X, Resolution.Y);
		PreviewRenderTarget->UpdateResourceImmediate(true);
	}

	if (PreviewCaptureComponent)
	{
		PreviewCaptureComponent->TextureTarget = PreviewRenderTarget;
	}

	// 2. Preview actor
	if (!PreviewActor)
	{
		FActorSpawnParameters ActorParams;
		ActorParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ActorParams.bNoFail = true;

		PreviewActor = World->SpawnActor<AOnsetAppearancePreviewActor>(
			AOnsetAppearancePreviewActor::StaticClass(),
			GetActorLocation(),
			FRotator(0.f, -90.f, 0.f),
			ActorParams);

		if (PreviewActor)
		{
			PreviewActor->SetActorEnableCollision(false);
			PreviewActor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
			PreviewActor->SetActorRelativeLocation(FVector::ZeroVector);
			PreviewActor->SetActorRelativeRotation(FRotator(0.f, -90.f, 0.f));
			PreviewActor->Use = EOnsetAppearanceUse::Player;
			PreviewActor->Catalog = Catalog;

			if (PreviewCaptureComponent)
			{
				PreviewCaptureComponent->ClearShowOnlyComponents();
				PreviewCaptureComponent->ShowOnlyActors.Reset();
				PreviewCaptureComponent->ShowOnlyActors.Add(PreviewActor);
			}
		}
	}

	// 3. Dynamic material
	if (!PreviewMaterialInstance)
	{
		UMaterialInterface* BaseMat = InPreviewMaterial.IsValid() ? InPreviewMaterial.Get() : InPreviewMaterial.LoadSynchronous();
		if (!BaseMat)
		{
			BaseMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_CharacterPreview.M_CharacterPreview"));
		}

		if (BaseMat)
		{
			PreviewMaterialInstance = UMaterialInstanceDynamic::Create(BaseMat, this);
		}
	}

	if (PreviewMaterialInstance && PreviewRenderTarget)
	{
		PreviewMaterialInstance->SetTextureParameterValue(TEXT("PreviewTexture"), PreviewRenderTarget);
	}
}

bool AOnsetAppearancePreviewRig::AttachToCanvas(UCanvasPanel* TargetCanvas, UWidgetTree* InWidgetTree)
{
	if (!TargetCanvas)
	{
		return false;
	}

	UObject* BrushResource = PreviewMaterialInstance ? static_cast<UObject*>(PreviewMaterialInstance.Get()) : static_cast<UObject*>(PreviewRenderTarget.Get());

	if (!PreviewImage)
	{
		if (InWidgetTree)
		{
			PreviewImage = InWidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("OnsetAppearancePreviewImage"));
		}
		else
		{
			PreviewImage = NewObject<UImage>(TargetCanvas, TEXT("OnsetAppearancePreviewImage"));
		}

		if (!PreviewImage)
		{
			return false;
		}

		PreviewImage->SetBrushResourceObject(BrushResource);
		PreviewImage->SetVisibility(bIsActive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

		UCanvasPanelSlot* ImageSlot = TargetCanvas->AddChildToCanvas(PreviewImage);
		if (ImageSlot)
		{
			ImageSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			ImageSlot->SetOffsets(FMargin(0.f));
			ImageSlot->SetZOrder(-1);
		}
	}
	else
	{
		PreviewImage->SetBrushResourceObject(BrushResource);
		PreviewImage->SetVisibility(bIsActive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

		if (PreviewImage->GetParent() != TargetCanvas)
		{
			UCanvasPanelSlot* ImageSlot = TargetCanvas->AddChildToCanvas(PreviewImage);
			if (ImageSlot)
			{
				ImageSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
				ImageSlot->SetOffsets(FMargin(0.f));
				ImageSlot->SetZOrder(-1);
			}
		}
	}

	return true;
}

bool AOnsetAppearancePreviewRig::ApplyRecipe(const FOnsetAppearanceRecipe& InRecipe, EOnsetAppearanceUse InUse)
{
	if (!PreviewActor)
	{
		return false;
	}

	PreviewActor->Recipe = InRecipe;
	PreviewActor->Use = InUse;
	return PreviewActor->RebuildPreview();
}

bool AOnsetAppearancePreviewRig::ApplyAppearanceFromJSON(const FString& InJSON, EOnsetCharacterClass FallbackClass)
{
	FOnsetAppearanceRecipe Recipe;
	TArray<FOnsetAppearanceIssue> Issues;
	bool bValid = false;

	if (!InJSON.IsEmpty())
	{
		bValid = UOnsetCharacterAppearanceComponent::ResolveStoredAppearance(InJSON, Recipe, Issues);
	}

	if (!bValid)
	{
		const int32 PresetIndex = (FallbackClass == EOnsetCharacterClass::Tank || FallbackClass == EOnsetCharacterClass::DPS) ? 1 : 0;
		bValid = UOnsetCharacterAppearanceComponent::GetPlayerPreset(PresetIndex, Recipe);
	}

	if (bValid)
	{
		return ApplyRecipe(Recipe);
	}

	return false;
}

void AOnsetAppearancePreviewRig::SetAnimation(UAnimSequence* InAnimation, bool bAutoPlay)
{
	if (!PreviewActor)
	{
		return;
	}

	PreviewActor->PreviewAnimation = InAnimation;
	PreviewActor->bAutoPlay = bAutoPlay;
	if (bAutoPlay && InAnimation)
	{
		PreviewActor->PlayPreview();
	}
}

void AOnsetAppearancePreviewRig::SetActive(bool bInActive)
{
	bIsActive = bInActive;

	if (PreviewCaptureComponent)
	{
		PreviewCaptureComponent->bCaptureEveryFrame = bInActive;
	}

	if (PreviewImage)
	{
		PreviewImage->SetVisibility(bInActive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (PreviewActor)
	{
		PreviewActor->SetActorHiddenInGame(!bInActive);
	}
}

void AOnsetAppearancePreviewRig::SetCameraSettings(const FVector& InRelativeLocation, const FRotator& InRelativeRotation, float InFOV)
{
	if (PreviewCaptureComponent)
	{
		PreviewCaptureComponent->SetRelativeLocationAndRotation(InRelativeLocation, InRelativeRotation);
		PreviewCaptureComponent->FOVAngle = InFOV;
	}
}

void AOnsetAppearancePreviewRig::SetCharacterRotation(const FRotator& InRotation)
{
	if (PreviewActor)
	{
		PreviewActor->SetActorRelativeRotation(InRotation);
	}
}

void AOnsetAppearancePreviewRig::AddCharacterYaw(float DeltaYaw)
{
	if (PreviewActor)
	{
		PreviewActor->AddActorLocalRotation(FRotator(0.f, DeltaYaw, 0.f));
	}
}

void AOnsetAppearancePreviewRig::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (PreviewActor)
	{
		PreviewActor->Destroy();
		PreviewActor = nullptr;
	}

	if (PreviewImage)
	{
		PreviewImage->RemoveFromParent();
		PreviewImage = nullptr;
	}

	PreviewCaptureComponent = nullptr;
	PreviewRenderTarget = nullptr;
	PreviewMaterialInstance = nullptr;

	Super::EndPlay(EndPlayReason);
}
