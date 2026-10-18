#include "OnsetAppearanceComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimClassInterface.h"
#include "Animation/Skeleton.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
void Color(USkeletalMeshComponent* Mesh, FName Parameter, const FLinearColor& Value)
{ if (Mesh && !Parameter.IsNone()) Mesh->SetVectorParameterValueOnMaterials(Parameter, FVector(Value.R, Value.G, Value.B)); }
void Skin(USkeletalMeshComponent* Mesh, FName Slot, FName Parameter, const FLinearColor& Value)
{
    if (!Mesh || Slot.IsNone() || Parameter.IsNone()) return;
    const int32 Index = Mesh->GetMaterialIndex(Slot);
    if (Index != INDEX_NONE)
        if (auto* MID = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(Index))) MID->SetVectorParameterValue(Parameter, Value);
}
void Texture(USkeletalMeshComponent* Mesh, FName Parameter, UTexture* Value)
{
    for (int32 I = 0; Mesh && I < Mesh->GetNumMaterials(); ++I)
        if (auto* MID = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(I))) MID->SetTextureParameterValue(Parameter, Value);
}
void Morphs(USkeletalMeshComponent* Mesh, const TMap<FName, float>& Values)
{
    if (!Mesh || !Mesh->GetSkeletalMeshAsset()) return;
    for (const auto& P : Values) if (Mesh->GetSkeletalMeshAsset()->FindMorphTarget(P.Key)) Mesh->SetMorphTarget(P.Key, P.Value);
}
}

UOnsetAppearanceComponent::UOnsetAppearanceComponent() { PrimaryComponentTick.bCanEverTick = false; }

void UOnsetAppearanceComponent::CancelPendingBuild()
{
    ++BuildRevision;
    if (PendingLoad) { PendingLoad->CancelHandle(); PendingLoad.Reset(); }
}

bool UOnsetAppearanceComponent::ApplyRecipe(const FOnsetAppearanceRecipe& Recipe, EOnsetAppearanceUse Use)
{
    CancelPendingBuild();
    LastIssues.Reset();
    if (!Catalog)
    { auto& I = LastIssues.AddDefaulted_GetRef(); I.Code = TEXT("MissingCatalog"); I.Field = TEXT("Catalog"); I.Message = TEXT("Appearance catalog is unavailable."); return false; }
    if (!Catalog->ValidateRecipe(Recipe, Use, LastIssues)) return false;
    TArray<FSoftObjectPath> Paths; Catalog->GetRecipeAssets(Recipe, Paths);
    for (const FSoftObjectPath& Path : Paths) if (!Path.TryLoad())
    {
        auto& I = LastIssues.AddDefaulted_GetRef(); I.Code = TEXT("MissingAsset"); I.Field = Path.ToString(); I.Message = TEXT("Appearance asset could not be loaded.");
        return false;
    }
    return BuildLoaded(Recipe);
}

bool UOnsetAppearanceComponent::ApplyRecipeAsync(const FOnsetAppearanceRecipe& Recipe, EOnsetAppearanceUse Use)
{
    CancelPendingBuild(); LastIssues.Reset();
    if (!Catalog)
    { auto& I = LastIssues.AddDefaulted_GetRef(); I.Code = TEXT("MissingCatalog"); I.Field = TEXT("Catalog"); I.Message = TEXT("Appearance catalog is unavailable."); return false; }
    if (!Catalog->ValidateRecipe(Recipe, Use, LastIssues)) return false;
    TArray<FSoftObjectPath> Paths; Catalog->GetRecipeAssets(Recipe, Paths);
    const uint64 Revision = BuildRevision;
    PendingLoad = UAssetManager::GetStreamableManager().RequestAsyncLoad(Paths,
        FStreamableDelegate::CreateWeakLambda(this, [this, Recipe, Use, Revision]()
        {
            if (Revision != BuildRevision) return;
            // Revalidate against the current catalog; ApplyRecipe also detects failed loads.
            ApplyRecipe(Recipe, Use);
        }));
    return PendingLoad.IsValid();
}

USkeletalMeshComponent* UOnsetAppearanceComponent::MakeMesh(USkeletalMesh* Mesh, USceneComponent* Parent, FName Socket, const FTransform& Relative)
{
    auto* C = NewObject<USkeletalMeshComponent>(GetOwner(), NAME_None, RF_Transient);
    C->SetMobility(EComponentMobility::Movable);
    C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    C->SetSkeletalMesh(Mesh);
    C->SetupAttachment(Parent, Socket);
    C->SetRelativeTransform(Relative);
    C->RegisterComponent();
    C->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    for (int32 I = 0; I < C->GetNumMaterials(); ++I) C->CreateDynamicMaterialInstance(I);
    BuiltMeshes.Add(C);
    return C;
}

bool UOnsetAppearanceComponent::BuildLoaded(const FOnsetAppearanceRecipe& R)
{
    auto Fail = [this](const FString& Field, const FString& Message)
    { auto& I = LastIssues.AddDefaulted_GetRef(); I.Code = TEXT("AssemblyFailed"); I.Field = Field; I.Message = Message; return false; };
    if (!GetOwner() || !IsRegistered() || !Catalog) return Fail(TEXT("Component"), TEXT("Appearance component must be registered on an actor."));
    const auto* B = Catalog->Bodies.Find(R.BodyId); const auto* H = Catalog->Heads.Find(R.HeadId);
    if (!B || !H || !B->Mesh.Get() || !H->Mesh.Get()) return Fail(TEXT("BodyId"), TEXT("Body or head asset is unavailable."));
    // Validate attachment sockets before replacing a previously valid preview.
    auto SocketExists = [](USkeletalMesh* Mesh, FName Socket)
    { return Socket.IsNone() || Mesh->FindSocket(Socket) || Mesh->GetRefSkeleton().FindBoneIndex(Socket) != INDEX_NONE; };
    if (!SocketExists(B->Mesh.Get(), B->HeadSocket)) return Fail(TEXT("HeadSocket"), TEXT("Body has no configured head attachment socket."));
    for (const auto& S : R.Parts)
    {
        const auto* P = Catalog->Parts.Find(S.PartId);
        if (!P || !P->Mesh.Get()) return Fail(S.PartId.ToString(), TEXT("Part mesh is unavailable."));
        const FName Socket = P->bHair ? B->HairSocket : P->Socket;
        if (!SocketExists(P->bAttachToHead ? H->Mesh.Get() : B->Mesh.Get(), Socket)) return Fail(S.PartId.ToString(), TEXT("Configured attachment socket is missing."));
    }
    bool bReuse = BodyMesh && HeadMesh && BodyMesh->GetSkeletalMeshAsset() == B->Mesh.Get()
        && HeadMesh->GetSkeletalMeshAsset() == H->Mesh.Get() && AppliedRecipe.BodyId == R.BodyId
        && AppliedRecipe.HeadId == R.HeadId && AppliedRecipe.Parts.Num() == R.Parts.Num();
    for (int32 Index = 0; bReuse && Index < R.Parts.Num(); ++Index)
        bReuse = AppliedRecipe.Parts[Index].PartId == R.Parts[Index].PartId;
    if (bReuse)
    { UpdateParameters(R); AppliedRecipe = R; OnAppearanceReady.Broadcast(); return true; }
    ClearMeshes();
    if (ExternalBodyMesh.IsValid())
    {
        BodyMesh = ExternalBodyMesh.Get();
        BodyMesh->EmptyOverrideMaterials();
        BodyMesh->SetSkeletalMesh(B->Mesh.Get());
        if (ExternalAnimationClass)
        {
            const auto* AnimInterface = IAnimClassInterface::GetFromClass(ExternalAnimationClass);
            if (AnimInterface && AnimInterface->GetTargetSkeleton() && AnimInterface->GetTargetSkeleton()->IsCompatibleMesh(B->Mesh.Get(), false))
            { BodyMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint); BodyMesh->SetAnimInstanceClass(ExternalAnimationClass); }
        }
        for (int32 Index = 0; Index < BodyMesh->GetNumMaterials(); ++Index) BodyMesh->CreateDynamicMaterialInstance(Index);
    }
    else BodyMesh = MakeMesh(B->Mesh.Get(), this, NAME_None, FTransform::Identity);
    HeadMesh = MakeMesh(H->Mesh.Get(), BodyMesh, B->HeadSocket, FTransform::Identity);
    TSet<FName> Suppressed; bool bHideHair = false;
    for (const auto& S : R.Parts) if (const auto* P = Catalog->Parts.Find(S.PartId))
    { for (FName Slot : P->SuppressedSlots) Suppressed.Add(Slot); bHideHair |= P->bHideHair; }
    for (const auto& S : R.Parts)
    {
        const auto& P = Catalog->Parts.FindChecked(S.PartId);
        if (Suppressed.Contains(P.Slot) || (P.bHair && bHideHair)) continue;
        auto* C = MakeMesh(P.Mesh.Get(), P.bAttachToHead ? HeadMesh.Get() : BodyMesh.Get(), P.bHair ? B->HairSocket : P.Socket, P.RelativeTransform);
        if (P.bShareBodyPose) C->SetLeaderPoseComponent(BodyMesh);
        PartMeshes.Add(S.PartId, C);
    }
    UpdateParameters(R);
    AppliedRecipe = R; OnAppearanceReady.Broadcast();
    return true;
}

void UOnsetAppearanceComponent::UpdateParameters(const FOnsetAppearanceRecipe& R)
{
    const auto& B = Catalog->Bodies.FindChecked(R.BodyId); const auto& H = Catalog->Heads.FindChecked(R.HeadId);
    BodyMesh->ClearMorphTargets(); HeadMesh->ClearMorphTargets();
    Skin(BodyMesh, B.SkinMaterialSlot, B.SkinParameter, R.SkinColor);
    Skin(HeadMesh, H.SkinMaterialSlot, H.SkinParameter, R.SkinColor);
    Color(HeadMesh, H.EyeParameter, R.EyeColor);
    BodyMesh->SetScalarParameterValueOnMaterials(TEXT("B_Muscle"), R.BodyMorphs.FindRef(TEXT("Musclar")));
    Morphs(BodyMesh, R.BodyMorphs); Morphs(HeadMesh, R.FaceMorphs);
    for (const auto& Entry : Catalog->Parts) if (!Entry.Value.BodyHideParameter.IsNone())
        Texture(BodyMesh, Entry.Value.BodyHideParameter, Catalog->BlankBodyHideTexture.Get());
    for (const auto& S : R.Parts)
    {
        auto* Found = PartMeshes.Find(S.PartId); if (!Found || !IsValid(*Found)) continue;
        auto* C = Found->Get(); const auto& P = Catalog->Parts.FindChecked(S.PartId);
        // Recreate material instances from authored defaults, so removing an override restores the original colors.
        for (int32 I = 0; I < C->GetNumMaterials(); ++I)
        {
            const auto& Materials = C->GetSkeletalMeshAsset()->GetMaterials();
            UMaterialInterface* Base = Materials.IsValidIndex(I) ? Materials[I].MaterialInterface.Get() : nullptr;
            if (auto* Existing = Cast<UMaterialInstanceDynamic>(C->GetMaterial(I)))
            { Existing->ClearParameterValues(); }
            else if (Base) C->CreateDynamicMaterialInstance(I, Base);
        }
        C->ClearMorphTargets(); Morphs(C, R.BodyMorphs); Morphs(BodyMesh, P.RequiredBodyMorphs);
        if (S.bOverrideColors)
        { Color(C, P.ColorParameterA, S.ColorA); Color(C, P.ColorParameterB, S.ColorB); Color(C, P.ColorParameterC, S.ColorC); }
        if (!P.BodyHideParameter.IsNone()) Texture(BodyMesh, P.BodyHideParameter, P.BodyHideTexture.Get());
    }
}

void UOnsetAppearanceComponent::ClearMeshes()
{
    for (int32 I = BuiltMeshes.Num() - 1; I >= 0; --I) if (IsValid(BuiltMeshes[I])) BuiltMeshes[I]->DestroyComponent();
    BuiltMeshes.Reset(); PartMeshes.Reset(); BodyMesh = nullptr; HeadMesh = nullptr;
}

void UOnsetAppearanceComponent::RefreshPreviewPose()
{
    if (!BodyMesh || !HeadMesh || !Catalog) return;
    Morphs(BodyMesh, AppliedRecipe.BodyMorphs);
    Morphs(HeadMesh, AppliedRecipe.FaceMorphs);
    for (const auto& Pair : PartMeshes)
    {
        if (!IsValid(Pair.Value)) continue;
        Morphs(Pair.Value, AppliedRecipe.BodyMorphs);
        if (const auto* Definition = Catalog->Parts.Find(Pair.Key)) Morphs(BodyMesh, Definition->RequiredBodyMorphs);
    }
    BodyMesh->SetScalarParameterValueOnMaterials(TEXT("B_Muscle"), AppliedRecipe.BodyMorphs.FindRef(TEXT("Musclar")));
    BodyMesh->RefreshFollowerComponents();
    BodyMesh->UpdateChildTransforms();
    for (const auto& Mesh : BuiltMeshes)
    {
        if (!Mesh || Mesh == BodyMesh) continue;
        Mesh->RefreshBoneTransforms();
        Mesh->UpdateChildTransforms();
        Mesh->MarkRenderDynamicDataDirty();
    }
    BodyMesh->MarkRenderDynamicDataDirty();
}

void UOnsetAppearanceComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{ CancelPendingBuild(); ClearMeshes(); Super::OnComponentDestroyed(bDestroyingHierarchy); }

void UOnsetAppearanceComponent::BindExternalBodyMesh(USkeletalMeshComponent* Mesh)
{
    if (ExternalBodyMesh.Get() == Mesh) return;
    CancelPendingBuild(); ClearMeshes(); ExternalBodyMesh=Mesh;
    ExternalAnimationClass=Mesh ? Mesh->GetAnimClass() : nullptr;
}
