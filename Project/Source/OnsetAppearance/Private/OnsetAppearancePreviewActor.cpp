#include "OnsetAppearancePreviewActor.h"
#include "OnsetAppearanceComponent.h"
#include "OnsetAppearancePreset.h"
#include "OnsetAuthoredAppearance.h"
#include "OnsetAnimationProfile.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

AOnsetAppearancePreviewActor::AOnsetAppearancePreviewActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    PrimaryActorTick.bTickEvenWhenPaused = true;
    Appearance = CreateDefaultSubobject<UOnsetAppearanceComponent>(TEXT("Appearance"));
    SetRootComponent(Appearance);
    Appearance->OnAppearanceReady.AddUniqueDynamic(this, &AOnsetAppearancePreviewActor::HandleAppearanceReady);
    Catalog = TSoftObjectPtr<UOnsetAppearanceCatalog>(FSoftObjectPath(TEXT("/Game/OnsetCharacterCreation/Catalog/DA_OnsetAppearanceCatalog.DA_OnsetAppearanceCatalog")));
}

bool AOnsetAppearancePreviewActor::RebuildPreview()
{
    if (!Appearance) return false;
    Appearance->Catalog = Catalog.LoadSynchronous();
    if (!Appearance->IsRegistered() && GetWorld()) Appearance->RegisterComponentWithWorld(GetWorld());
    return Appearance->ApplyRecipe(Recipe, Use);
}

bool AOnsetAppearancePreviewActor::LoadPreset()
{
    const auto* Loaded = Preset.LoadSynchronous();
    if (!Loaded) return false;
    Recipe = Loaded->Recipe; Use = Loaded->Use;
    return RebuildPreview();
}

bool AOnsetAppearancePreviewActor::LoadAuthoredAppearance(UOnsetAuthoredAppearance* InAppearance)
{
    if (!InAppearance)
    {
        InAppearance = AuthoredAppearance.LoadSynchronous();
    }
    if (!InAppearance) return false;

    AuthoredAppearance = InAppearance;
    Recipe = InAppearance->Recipe;
    Use = EOnsetAppearanceUse::AuthoredCharacter;

    if (!InAppearance->AnimationProfile.IsNull())
    {
        LoadAnimationProfile(InAppearance->AnimationProfile.LoadSynchronous());
    }

    return RebuildPreview();
}

bool AOnsetAppearancePreviewActor::LoadAnimationProfile(UOnsetAnimationProfile* InProfile)
{
    if (!InProfile)
    {
        InProfile = ActiveAnimationProfile.LoadSynchronous();
    }
    if (!InProfile) return false;

    ActiveAnimationProfile = InProfile;
    SampleAnimations = InProfile->SamplePreviewAnimations;

    if (UAnimSequence* IdleSeq = Cast<UAnimSequence>(InProfile->DefaultIdleAnim.LoadSynchronous()))
    {
        PreviewAnimation = IdleSeq;
    }
    else if (!InProfile->SamplePreviewAnimations.IsEmpty())
    {
        PreviewAnimation = InProfile->SamplePreviewAnimations[0];
    }

    if (bPreviewInitialized)
    {
        ConfigureAnimation();
        if (bAutoPlay) PlayPreview();
    }
    return true;
}

void AOnsetAppearancePreviewActor::RotatePreview(float DeltaYawDegrees)
{
    AddActorLocalRotation(FRotator(0.f, DeltaYawDegrees, 0.f));
}

bool AOnsetAppearancePreviewActor::ConfigureAnimation()
{
    auto* Body = Appearance->GetBodyMesh();
    if (!Body || !Body->GetSkeletalMeshAsset()) { AnimationStatus = TEXT("Build a valid appearance first."); return false; }
    UAnimSequence* Candidate = PreviewAnimation.LoadSynchronous();
    if (!Candidate) { AnimationStatus = TEXT("Select an animation sequence."); return false; }
    if (Candidate->GetSkeleton() != Body->GetSkeletalMeshAsset()->GetSkeleton())
    { AnimationStatus = TEXT("Skeleton mismatch: select a clip for this body or retarget it first."); return false; }
    const float Length = Candidate->GetPlayLength();
    if (!FMath::IsFinite(Length) || Length <= KINDA_SMALL_NUMBER)
    { AnimationStatus = TEXT("Selected animation has no playable duration."); return false; }
    if (ActiveBody.Get() != Body || ActiveAnimation != Candidate || !Body->GetSingleNodeInstance())
    {
        Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        Body->SetAnimation(Candidate);
        Body->SetComponentTickEnabled(false);
        ActiveBody = Body; ActiveAnimation = Candidate;
    }
    if (auto* Instance = Body->GetSingleNodeInstance()) Instance->SetPlaying(false);
    else { AnimationStatus = TEXT("Could not create a single-node animation instance."); return false; }
    AnimationLength = Length;
    PreviewTime = FMath::Clamp(FMath::IsFinite(PreviewTime) ? PreviewTime : 0.f, 0.f, Length);
    AnimationStatus = FString::Printf(TEXT("%s (%.2f seconds)"), *Candidate->GetName(), Length);
    return true;
}

void AOnsetAppearancePreviewActor::EvaluatePreviewPose()
{
    auto* Body = ActiveBody.Get();
    if (!Body || !ActiveAnimation || Body != Appearance->GetBodyMesh()) return;
    Body->SetPosition(PreviewTime, false);
    Body->TickAnimation(0.f, false);
    Body->RefreshBoneTransforms();
    Appearance->RefreshPreviewPose();
}

void AOnsetAppearancePreviewActor::HandleAppearanceReady()
{
    if (PreviewAnimation.IsNull()) { ShowReferencePose(); return; }
    if (ConfigureAnimation()) { if (bAutoPlay) PlayPreview(); else EvaluatePreviewPose(); }
    else if (ActiveBody.Get() != Appearance->GetBodyMesh()) { bPreviewPlaying = false; ActiveAnimation = nullptr; ActiveBody.Reset(); AnimationLength = 0.f; }
}

bool AOnsetAppearancePreviewActor::PlayPreview()
{
    if (!ConfigureAnimation()) return false;
    if (!FMath::IsFinite(PlaybackSpeed) || FMath::Abs(PlaybackSpeed) > 4.f)
    { AnimationStatus = TEXT("Playback speed must be finite and between -4 and 4."); return false; }
    if (PlaybackSpeed >= 0.f && PreviewTime >= AnimationLength) PreviewTime = 0.f;
    else if (PlaybackSpeed < 0.f && PreviewTime <= 0.f) PreviewTime = AnimationLength;
    bPreviewPlaying = true; EvaluatePreviewPose(); return true;
}

void AOnsetAppearancePreviewActor::PausePreview() { bPreviewPlaying = false; }

bool AOnsetAppearancePreviewActor::RestartPreview()
{
    if (!ConfigureAnimation()) return false;
    PreviewTime = PlaybackSpeed < 0.f ? AnimationLength : 0.f;
    return PlayPreview();
}

void AOnsetAppearancePreviewActor::StopPreview()
{ bPreviewPlaying = false; PreviewTime = 0.f; if (ConfigureAnimation()) EvaluatePreviewPose(); }

bool AOnsetAppearancePreviewActor::ScrubPreview()
{ bPreviewPlaying = false; if (!ConfigureAnimation()) return false; EvaluatePreviewPose(); return true; }

bool AOnsetAppearancePreviewActor::SetPreviewPosition(float Seconds)
{
    if (!FMath::IsFinite(Seconds)) { AnimationStatus = TEXT("Preview time must be finite."); return false; }
    PreviewTime = Seconds; return ScrubPreview();
}

void AOnsetAppearancePreviewActor::ShowReferencePose()
{
    bPreviewPlaying = false; PreviewAnimation.Reset(); ActiveAnimation = nullptr; ActiveBody.Reset();
    AnimationLength = 0.f; PreviewTime = 0.f; AnimationStatus = TEXT("Reference pose.");
    if (auto* Body = Appearance->GetBodyMesh())
    {
        Body->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        Body->SetAnimation(nullptr); Body->SetComponentTickEnabled(false);
        Body->TickAnimation(0.f, false); Body->RefreshBoneTransforms(); Appearance->RefreshPreviewPose();
    }
}

FName AOnsetAppearancePreviewActor::GetActiveAnimationName() const
{ return ActiveAnimation ? ActiveAnimation->GetFName() : NAME_None; }

bool AOnsetAppearancePreviewActor::SelectSample(int32 Direction)
{
    if (SampleAnimations.IsEmpty()) { AnimationStatus = TEXT("No sample clips are assigned to this actor."); return false; }
    int32 Index = SampleAnimations.IndexOfByPredicate([this](const auto& Clip) { return Clip.ToSoftObjectPath() == PreviewAnimation.ToSoftObjectPath(); });
    if (Index == INDEX_NONE) Index = Direction > 0 ? -1 : 0;
    Index = (Index + Direction + SampleAnimations.Num()) % SampleAnimations.Num();
    const bool bWasPlaying = bPreviewPlaying;
    PreviewAnimation = SampleAnimations[Index]; PreviewTime = 0.f;
    return bWasPlaying ? PlayPreview() : ScrubPreview();
}
bool AOnsetAppearancePreviewActor::NextSampleAnimation() { return SelectSample(1); }
bool AOnsetAppearancePreviewActor::PreviousSampleAnimation() { return SelectSample(-1); }

void AOnsetAppearancePreviewActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bTurntableEnabled && FMath::Abs(TurntableSpeed) > 0.01f)
    {
        AddActorLocalRotation(FRotator(0.f, TurntableSpeed * DeltaSeconds, 0.f));
    }

    if (!bPreviewPlaying || !ActiveAnimation || !ActiveBody.IsValid()) return;
    if (!FMath::IsFinite(DeltaSeconds) || DeltaSeconds < 0.f || !FMath::IsFinite(PlaybackSpeed) || FMath::Abs(PlaybackSpeed) > 4.f)
    { PausePreview(); AnimationStatus = TEXT("Invalid playback time or speed."); return; }
    float Time = PreviewTime + DeltaSeconds * PlaybackSpeed;
    if (bLoopAnimation)
    { Time = FMath::Fmod(Time, AnimationLength); if (Time < 0.f) Time += AnimationLength; }
    else if (Time >= AnimationLength || Time < 0.f)
    { Time = FMath::Clamp(Time, 0.f, AnimationLength); bPreviewPlaying = false; }
    PreviewTime = Time; EvaluatePreviewPose();
}

void AOnsetAppearancePreviewActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    const bool bFirstBuild = !bPreviewInitialized;
    RebuildPreview(); bPreviewInitialized = true;
    if (bFirstBuild && bAutoPlay) PlayPreview();
}
void AOnsetAppearancePreviewActor::BeginPlay()
{ Super::BeginPlay(); RebuildPreview(); if (bAutoPlay) PlayPreview(); }
