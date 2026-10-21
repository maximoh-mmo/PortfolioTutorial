#include "Player/OnsetCharacterAppearanceComponent.h"
#include "Player/OnsetPlayerCharacter.h"
#include "OnsetAppearanceLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Animation/AnimInstance.h"
#include "Net/UnrealNetwork.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

UOnsetCharacterAppearanceComponent::UOnsetCharacterAppearanceComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = true;
    bTickInEditor = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
    OnAppearanceReady.AddUniqueDynamic(this, &UOnsetCharacterAppearanceComponent::HandleReady);
}
UOnsetAppearanceCatalog* UOnsetCharacterAppearanceComponent::LoadDefaultCatalog()
{ return LoadObject<UOnsetAppearanceCatalog>(nullptr,TEXT("/Game/OnsetCharacterCreation/Catalog/DA_OnsetAppearanceCatalog.DA_OnsetAppearanceCatalog")); }
bool UOnsetCharacterAppearanceComponent::GetPlayerPreset(int32 Index,FOnsetAppearanceRecipe& Recipe)
{
    // Explicit Onset menu order. Historical preset indices did not change the pawn mesh.
    if (Index < 0 || Index > 1) return false;
    const FString Name=Index==0 ? TEXT("DA_EpicFemale") : TEXT("DA_EpicMale");
    auto* Preset=LoadObject<UOnsetAppearancePreset>(nullptr,*FString::Printf(TEXT("/Game/OnsetCharacterCreation/Presets/%s.%s"),*Name,*Name));
    if (!Preset) return false;
    Recipe=Preset->Recipe;return true;
}
bool UOnsetCharacterAppearanceComponent::ResolveStoredAppearance(const FString& JSON,FOnsetAppearanceRecipe& Recipe,TArray<FOnsetAppearanceIssue>& Issues)
{
    Issues.Reset();
    if (JSON.Len() <= 65536)
    {
        TSharedPtr<FJsonObject> Object;
        if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(JSON),Object) && Object.IsValid() && !Object->HasField(TEXT("schemaVersion")))
        {
            double Index=0;
            if (Object->TryGetNumberField(TEXT("PresetIndex"),Index) && FMath::IsFinite(Index) && Index >= 0 && Index <= 255 && FMath::FloorToDouble(Index)==Index)
            {
                FOnsetAppearanceRecipe Candidate;
                auto* DefaultCatalog=LoadDefaultCatalog();
                if (DefaultCatalog && GetPlayerPreset(0,Candidate) && DefaultCatalog->ValidateRecipe(Candidate,EOnsetAppearanceUse::Player,Issues)) { Recipe=MoveTemp(Candidate);return true; }
            }
        }
    }
    return UOnsetAppearanceLibrary::DecodeAndValidateRecipe(JSON,LoadDefaultCatalog(),EOnsetAppearanceUse::Player,Recipe,Issues);
}
EOnsetAppearanceUse UOnsetCharacterAppearanceComponent::GetUse() const
{ return GetOwner() && GetOwner()->IsA<AOnsetPlayerCharacter>() ? EOnsetAppearanceUse::Player : EOnsetAppearanceUse::AuthoredCharacter; }
bool UOnsetCharacterAppearanceComponent::BindCharacterMesh()
{
    auto* Character=Cast<ACharacter>(GetOwner());
    if (!IsRegistered() || !Character || !Character->GetMesh()) return false;
    if (!Catalog) Catalog=LoadDefaultCatalog();
    if (Character->GetMesh()->GetRelativeRotation().IsNearlyZero())
        Character->GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
    if (!Character->GetMesh()->GetSkeletalMeshAsset())
        Character->GetMesh()->SetRelativeLocation(FVector(0.f,0.f,-Character->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
    BindExternalBodyMesh(Character->GetMesh());
    return Catalog != nullptr;
}
bool UOnsetCharacterAppearanceComponent::SetAppearanceRecipe(const FOnsetAppearanceRecipe& Recipe,TArray<FOnsetAppearanceIssue>& Issues)
{
    Issues.Reset();
    if (!GetOwner() || !GetOwner()->HasAuthority()) { auto& I=Issues.AddDefaulted_GetRef(); I.Code=TEXT("AuthorityRequired");I.Message=TEXT("Appearance changes require server authority.");return false; }
    if (!BindCharacterMesh() || !Catalog->ValidateRecipe(Recipe,GetUse(),Issues)) return false;
    FString Canonical;
    if (!UOnsetAppearanceLibrary::EncodeRecipe(Recipe,Canonical) || !ApplyRecipe(Recipe,GetUse())) { Issues=LastIssues;return false; }
    AppearanceJSON=MoveTemp(Canonical);GetOwner()->ForceNetUpdate();return true;
}
void UOnsetCharacterAppearanceComponent::OnRep_AppearanceJSON()
{
    UE_LOG(LogTemp, Log, TEXT("OnRep_AppearanceJSON: actor=%s len=%d"), *GetNameSafe(GetOwner()), AppearanceJSON.Len());
    if (AppearanceJSON.IsEmpty()) return;
    if (!BindCharacterMesh()) { bPendingAppearance=true;return; }
    bPendingAppearance=false;
    FOnsetAppearanceRecipe Recipe;TArray<FOnsetAppearanceIssue> Issues;
    if (UOnsetAppearanceLibrary::DecodeAndValidateRecipe(AppearanceJSON,Catalog,GetUse(),Recipe,Issues))
    {
        UE_LOG(LogTemp, Log, TEXT("OnRep_AppearanceJSON: applying appearance body=%s head=%s parts=%d"), *Recipe.BodyId.ToString(), *Recipe.HeadId.ToString(), Recipe.Parts.Num());
        if (GetWorld() && GetWorld()->WorldType == EWorldType::Editor) ApplyRecipe(Recipe, GetUse());
        else ApplyRecipeAsync(Recipe, GetUse());
    }
    else UE_LOG(LogTemp,Warning,TEXT("Rejected replicated appearance for %s"),*GetNameSafe(GetOwner()));
}
void UOnsetCharacterAppearanceComponent::BeginPlay()
{
    Super::BeginPlay();BindCharacterMesh();
    if (GetOwner()->HasAuthority() && AppearanceJSON.IsEmpty() && !AuthoredPreset.IsNull() && GetUse()==EOnsetAppearanceUse::AuthoredCharacter)
    { if (auto* Preset=AuthoredPreset.LoadSynchronous()) { TArray<FOnsetAppearanceIssue> Issues;SetAppearanceRecipe(Preset->Recipe,Issues); } }
    else OnRep_AppearanceJSON();
}
void UOnsetCharacterAppearanceComponent::EndPlay(const EEndPlayReason::Type Reason)
{ CancelPendingBuild();Super::EndPlay(Reason); }
void UOnsetCharacterAppearanceComponent::HandleReady()
{
    UE_LOG(LogTemp, Log, TEXT("HandleReady: appearance applied successfully on %s (Body=%s Head=%s)"),
        *GetNameSafe(GetOwner()), *AppliedRecipe.BodyId.ToString(), *AppliedRecipe.HeadId.ToString());
    if (AppliedRecipe.BodyId != TEXT("human.kid.authored"))
    {
        if (GetBodyMesh() && !GetBodyMesh()->GetAnimClass())
            if (auto* Class=LoadClass<UAnimInstance>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C")))
                GetBodyMesh()->SetAnimInstanceClass(Class);
        return;
    }
    const FString Base=TEXT("/Game/Characters/Humanoid/Kid/Animation/Retarget/");
    auto Load=[&Base](const TCHAR* Name)
    {
        UAnimSequence* Anim = LoadObject<UAnimSequence>(nullptr,*(Base+Name+TEXT(".")+Name));
        if (!Anim)
        {
            const FString AltBase = TEXT("/Game/Characters/Humanoid/Kid/Animation/Retarget/");
            Anim = LoadObject<UAnimSequence>(nullptr,*(AltBase+Name+TEXT(".")+Name));
        }
        return Anim;
    };
    ChildIdle=Load(TEXT("A_ACC_Hmn_HC_Wait_01"));ChildRun=Load(TEXT("A_ACC_Hmn_HC_Grd_Run_F"));ChildAir=Load(TEXT("A_ACC_Hmn_HC_Jump_Air"));
    if (GetBodyMesh())
    {
        GetBodyMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
        if (ChildIdle) GetBodyMesh()->PlayAnimation(ChildIdle,true);
    }
}
void UOnsetCharacterAppearanceComponent::TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime,TickType,ThisTickFunction);
    if (bPendingAppearance) OnRep_AppearanceJSON();
    if (GetBodyMesh()) RefreshPreviewPose();
    if (AppliedRecipe.BodyId==TEXT("human.kid.authored"))
    {
        auto* Character=Cast<ACharacter>(GetOwner());auto* Body=GetBodyMesh();
        if (Character && Body)
        {
            UAnimSequence* Clip=Character->GetCharacterMovement()->IsFalling() ? ChildAir.Get() : (Character->GetVelocity().SizeSquared2D()>25.f ? ChildRun.Get() : ChildIdle.Get());
            if (Clip && Body->GetAnimationMode()==EAnimationMode::AnimationSingleNode && Body->AnimationData.AnimToPlay != Clip) Body->PlayAnimation(Clip,true);
        }
    }
}
void UOnsetCharacterAppearanceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{ Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(UOnsetCharacterAppearanceComponent,AppearanceJSON); }

void UOnsetCharacterAppearanceComponent::ReapplyAppearance() { OnRep_AppearanceJSON(); }
void UOnsetCharacterAppearanceComponent::OnRegister()
{
    Super::OnRegister();
    if (!GetOwner() || IsTemplate()) return;
    if (!AppearanceJSON.IsEmpty()) OnRep_AppearanceJSON();
}
#if WITH_EDITOR
void UOnsetCharacterAppearanceComponent::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
    Super::PostEditChangeProperty(Event);
    if (!IsTemplate() && GetWorld() && GetWorld()->WorldType==EWorldType::Editor && Event.GetPropertyName()==GET_MEMBER_NAME_CHECKED(UOnsetCharacterAppearanceComponent,AuthoredPreset))
        if (auto* Preset=AuthoredPreset.LoadSynchronous()) { TArray<FOnsetAppearanceIssue> Issues; SetAppearanceRecipe(Preset->Recipe,Issues); }
}
#endif
