#include "OnsetAppearanceIntegrationAuditCommandlet.h"
#include "OnsetAppearanceCatalog.h"
#include "OnsetAppearancePreset.h"
#include "OnsetAppearanceComponent.h"
#include "OnsetAppearanceLibrary.h"
#include "Player/OnsetCharacterAppearanceComponent.h"
#include "Subsystem/OnsetPlayerDataSubsystem.h"
#include "DataStoreFactory.h"
#include "Animation/AnimInstance.h"
#include "Tickable.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "OnsetAppearancePreviewActor.h"
#include "Player/OnsetPlayerCharacter.h"
#include "Player/OnsetPlayerController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Animation/Skeleton.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

UOnsetAppearanceIntegrationAuditCommandlet::UOnsetAppearanceIntegrationAuditCommandlet()
{ IsClient = false; IsServer = false; IsEditor = true; LogToConsole = true; }
int32 UOnsetAppearanceIntegrationAuditCommandlet::Main(const FString& Params)
{
    int32 Failures = 0; FString Report;
    auto Check = [&Failures](bool bOK, const TCHAR* Label) { if (!bOK) { ++Failures; UE_LOG(LogTemp, Error, TEXT("Appearance integration: %s"), Label); } };
    auto* Catalog = LoadObject<UOnsetAppearanceCatalog>(nullptr, TEXT("/Game/OnsetCharacterCreation/Catalog/DA_OnsetAppearanceCatalog.DA_OnsetAppearanceCatalog"));
    Check(Catalog != nullptr, TEXT("catalog load")); if (!Catalog) return 1;
    auto* Epic = Catalog->Bodies.FindChecked(TEXT("epic.adult.male")).Mesh.LoadSynchronous();
    if (FParse::Param(*Params, TEXT("RepairPreviewReference")))
    {
        Epic->GetSkeleton()->SetPreviewMesh(Epic);
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
        UPackage* Package = Epic->GetSkeleton()->GetOutermost();
        Check(UPackage::SavePackage(Package, Epic->GetSkeleton(), *FPackageName::LongPackageNameToFilename(Package->GetName(), TEXT(".uasset")), Args), TEXT("repair skeleton preview metadata"));
        auto* KidMesh = Catalog->Bodies.FindChecked(TEXT("human.kid.authored")).Mesh.LoadSynchronous();
        Check(KidMesh && KidMesh->GetSkeleton(), TEXT("load child skeleton for preview repair"));
        if (KidMesh && KidMesh->GetSkeleton())
        {
            KidMesh->GetSkeleton()->SetPreviewMesh(KidMesh);
            UPackage* KidPackage = KidMesh->GetSkeleton()->GetOutermost();
            Check(UPackage::SavePackage(KidPackage, KidMesh->GetSkeleton(), *FPackageName::LongPackageNameToFilename(KidPackage->GetName(), TEXT(".uasset")), Args), TEXT("repair child skeleton preview metadata"));
        }
    }
    UWorld* World = UWorld::CreateWorld(EWorldType::Editor, false);
    auto* CharacterClass = LoadClass<AOnsetPlayerCharacter>(nullptr, TEXT("/Game/Characters/Player/PlayerCharacter.PlayerCharacter_C"));
    auto* ControllerClass = LoadClass<AOnsetPlayerController>(nullptr, TEXT("/Game/Input/MyOnsetPlayerController.MyOnsetPlayerController_C"));
    Check(CharacterClass && ControllerClass, TEXT("Blueprint classes load"));
    if (!CharacterClass || !ControllerClass) { World->DestroyWorld(false); return 1; }
    auto* Character = World->SpawnActor<AOnsetPlayerCharacter>(CharacterClass);
    auto* Controller = World->SpawnActor<AOnsetPlayerController>(ControllerClass);
    for (int32 Index : {0,1,2,3,255})
    {
        struct { AOnsetPlayerCharacter* PlayerChar; int32 PresetIndex; } Arguments{Character,Index};
        Controller->ProcessEvent(Controller->FindFunctionChecked(TEXT("BP_ApplyAppearancePreset")), &Arguments);
        Report += FString::Printf(TEXT("legacy %d -> %s\n"), Index, *GetPathNameSafe(Character->GetMesh()->GetSkeletalMeshAsset()));
    }
    auto* Driver = Character->GetMesh()->GetSkeletalMeshAsset();
    for (const TCHAR* Name : {TEXT("DA_EpicMale"),TEXT("DA_EpicFemale"),TEXT("DA_HumanKidMaleNPC"),TEXT("DA_HumanKidFemaleNPC")})
    {
        auto* Preset = LoadObject<UOnsetAppearancePreset>(nullptr, *FString::Printf(TEXT("/Game/OnsetCharacterCreation/Presets/%s.%s"),Name,Name));
        Check(Preset != nullptr,TEXT("preset load")); if (!Preset) continue;
        TArray<FOnsetAppearanceIssue> Issues;
        Check(Catalog->ValidateRecipe(Preset->Recipe,Preset->Use,Issues),TEXT("preset validates"));
        const bool bChild = Preset->Recipe.BodyId == TEXT("human.kid.authored");
        Check(Catalog->ValidateRecipe(Preset->Recipe,EOnsetAppearanceUse::Player,Issues) != bChild,TEXT("player body restriction"));
        auto* Preview = World->SpawnActor<AOnsetAppearancePreviewActor>();
        Preview->Recipe = Preset->Recipe;
        Preview->Use = Preset->Use;
        const bool bRebuildOK = Preview->RebuildPreview();
        Check(bRebuildOK, TEXT("preset assembly"));
        auto* Body = Preview->Appearance ? Preview->Appearance->GetBodyMesh() : nullptr;
        if (!bChild && Body && Body->GetSkeletalMeshAsset() && Driver)
        {
            const auto& Ref = Body->GetSkeletalMeshAsset()->GetRefSkeleton();
            const auto& Source = Driver->GetRefSkeleton();
            int32 Common = 0, Missing = 0, Different = 0;
            float MaxPosition = 0.f;
            for (int32 Bone = 0; Bone < Ref.GetNum(); ++Bone)
            {
                const int32 SourceBone = Source.FindBoneIndex(Ref.GetBoneName(Bone));
                if (SourceBone == INDEX_NONE) { ++Missing; continue; }
                ++Common;
                const auto& A = Ref.GetRefBonePose()[Bone];
                const auto& B = Source.GetRefBonePose()[SourceBone];
                if (!A.Equals(B, 0.01f)) ++Different;
                MaxPosition = FMath::Max(MaxPosition, static_cast<float>(FVector::Distance(A.GetLocation(), B.GetLocation())));
            }
            Report += FString::Printf(TEXT("%s driver=%s common=%d missing=%d different=%d maxLocalPositionDelta=%.4f cm\n"),
                Name, *Driver->GetPathName(), Common, Missing, Different, MaxPosition);
        }
        else if (!bChild)
        {
            Report += FString::Printf(TEXT("%s assembly failed (body=%s, mesh=%s, driver=%s)\n"),
                Name,
                Body ? *Body->GetName() : TEXT("null"),
                (Body && Body->GetSkeletalMeshAsset()) ? *Body->GetSkeletalMeshAsset()->GetPathName() : TEXT("null"),
                Driver ? *Driver->GetPathName() : TEXT("null"));
        }
        Preview->Destroy();
    }
    const auto OriginalTransform = Character->GetMesh()->GetRelativeTransform();
    auto* OriginalComponent = Character->GetMesh();
    UClass* OriginalAnimClass = Character->GetMesh()->GetAnimClass();
    FOnsetAppearanceRecipe Female, Male; TArray<FOnsetAppearanceIssue> Issues;
    Check(UOnsetCharacterAppearanceComponent::GetPlayerPreset(0, Female), TEXT("female menu preset"));
    Check(UOnsetCharacterAppearanceComponent::GetPlayerPreset(1, Male), TEXT("male menu preset"));
    FString LongAliasJSON;
    UOnsetAppearanceLibrary::EncodeRecipe(Female, LongAliasJSON);
    LongAliasJSON.TrimEndInline();
    LongAliasJSON.LeftChopInline(1);
    LongAliasJSON += FString::Printf(TEXT(",\"BodyId\":\"%s\"}"), *FString::ChrN(129, TEXT('x')));
    FOnsetAppearanceRecipe RejectedRecipe;
    Check(!UOnsetAppearanceLibrary::DecodeAndValidateRecipe(LongAliasJSON, Catalog, EOnsetAppearanceUse::Player, RejectedRecipe, Issues)
        && !Issues.IsEmpty() && Issues[0].Code == TEXT("InvalidIdentifier"), TEXT("bound alternate-case body identifier"));
    const bool bFemaleOK = Character->CharacterAppearance->SetAppearanceRecipe(Female, Issues);
    Check(bFemaleOK, TEXT("player native female"));
    Check(Character->GetMesh() == OriginalComponent && Character->CharacterAppearance->GetBodyMesh() == OriginalComponent, TEXT("reuse ACharacter animation mesh"));
    Check(Character->GetMesh()->GetRelativeTransform().Equals(OriginalTransform), TEXT("preserve character mesh transform"));
    Check(Character->GetMesh()->GetAnimClass() == OriginalAnimClass && Character->GetMesh()->GetAnimInstance(), TEXT("retain gameplay Animation Blueprint"));
    Female.BodyMorphs.Add(TEXT("Fat"), 0.65f);
    const bool bFemaleEditOK = Character->CharacterAppearance->SetAppearanceRecipe(Female, Issues);
    Check(bFemaleEditOK, TEXT("player parameter edit"));
    Character->GetMesh()->TickAnimation(0.1f, false);
    Character->GetMesh()->RefreshBoneTransforms();
    Character->CharacterAppearance->RefreshPreviewPose();
    Check(FMath::IsNearlyEqual(Character->GetMesh()->GetMorphTarget(TEXT("Fat")), 0.65f), TEXT("morph survives gameplay evaluation"));
    auto* HeadComp = Character->CharacterAppearance->GetHeadMesh();
    Check(HeadComp != nullptr, TEXT("player native head component"));
    Report += FString::Printf(TEXT("native player head world location=%s anim=%s\n"),
        HeadComp ? *HeadComp->GetComponentLocation().ToString() : TEXT("None"),
        *GetPathNameSafe(Character->GetMesh()->GetAnimClass()));
    const bool bMaleOK = Character->CharacterAppearance->SetAppearanceRecipe(Male, Issues);
    Check(bMaleOK, TEXT("switch player body"));
    auto* Child = LoadObject<UOnsetAppearancePreset>(nullptr, TEXT("/Game/OnsetCharacterCreation/Presets/DA_HumanKidMaleNPC.DA_HumanKidMaleNPC"));
    Check(Child && !Character->CharacterAppearance->SetAppearanceRecipe(Child->Recipe, Issues) && Character->CharacterAppearance->AppliedRecipe.BodyId == Male.BodyId, TEXT("reject player child and preserve appearance"));
    FOnsetAppearanceRecipe Legacy;
    Check(UOnsetCharacterAppearanceComponent::ResolveStoredAppearance(TEXT("{\"PresetIndex\":255}"), Legacy, Issues) && Legacy.BodyId == TEXT("epic.adult.female"), TEXT("legacy default female compatibility"));
    Check(!UOnsetCharacterAppearanceComponent::ResolveStoredAppearance(TEXT("{\"PresetIndex\":0.5}"), Legacy, Issues), TEXT("reject malformed legacy index"));
    auto* NPC = World->SpawnActor<AOnsetBaseCharacter>();
    Check(NPC && Child && NPC->CharacterAppearance->SetAppearanceRecipe(Child->Recipe, Issues), TEXT("authored child NPC assembly"));
    Check(NPC && NPC->GetMesh() && NPC->GetMesh()->GetAnimationMode() == EAnimationMode::AnimationSingleNode, TEXT("child uses its own animation rig"));
    auto* Replica = World->SpawnActor<AOnsetPlayerCharacter>(CharacterClass);
    if (Replica && Replica->CharacterAppearance)
    {
        Replica->CharacterAppearance->AppearanceJSON = Character->CharacterAppearance->AppearanceJSON;
        Replica->CharacterAppearance->ProcessEvent(Replica->CharacterAppearance->FindFunctionChecked(TEXT("OnRep_AppearanceJSON")), nullptr);
        FlushAsyncLoading();
        for (int32 Step = 0; Step < 10; ++Step) FTickableGameObject::TickObjects(nullptr, LEVELTICK_All, false, 0.01f);
        Check(Replica->CharacterAppearance->AppliedRecipe.BodyId == Male.BodyId && Replica->CharacterAppearance->GetBodyMesh() == Replica->GetMesh(), TEXT("replicated recipe reconstruction"));
        Replica->CharacterAppearance->DestroyComponent();
        Check(IsValid(Replica->GetMesh()) && Replica->GetMesh()->IsRegistered(), TEXT("borrowed mesh survives component teardown"));
    }
    else
    {
        Check(false, TEXT("spawn player replica"));
    }

    // An isolated SQLite fixture exercises the existing adapter and identity cache, never the live account database.
    const FString Fixture = FPaths::ProjectSavedDir()/TEXT("AppearanceQA")/(FGuid::NewGuid().ToString()+TEXT(".sqlite"));
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Fixture), true);
    bool bStoreReady = false;
    auto* Data = NewObject<UOnsetPlayerDataSubsystem>(World);
    Data->Store = CreateDataStore(TEXT("SQLite"), Fixture, bStoreReady);
    Check(bStoreReady && Data->CreateAccount(TEXT("audit"), TEXT("appearance")), TEXT("isolated persistence account"));
    FOnsetFullCharacterData Record; Record.SlotIndex = 0; Record.CharacterName = TEXT("AppearanceAudit"); Record.CharacterClass = EOnsetCharacterClass::DPS;
    UOnsetAppearanceLibrary::EncodeRecipe(Female, Record.AppearanceJSON); Record.Level = 3;
    Check(Data->SaveCharacter(TEXT("audit"), TEXT("appearance"), Record), TEXT("save native appearance"));
    FOnsetFullCharacterData Loaded;
    Check(Data->LoadCharacter(TEXT("audit"), TEXT("appearance"), 0, Loaded) && Loaded.AppearanceJSON == Record.AppearanceJSON, TEXT("native recipe persistence round trip"));
    UOnsetAppearanceLibrary::EncodeRecipe(Male, Loaded.AppearanceJSON);
    Check(Data->SaveCharacter(TEXT("audit"), TEXT("appearance"), Loaded), TEXT("save changed appearance"));
    FOnsetFullCharacterData Autosave; Autosave.SlotIndex = 0;
    Check(Data->SaveCharacterPreservingIdentity(TEXT("audit"), TEXT("appearance"), Autosave) && Autosave.AppearanceJSON == Loaded.AppearanceJSON, TEXT("autosave retains changed appearance cache"));
    FOnsetFullCharacterData Final;
    Check(Data->LoadCharacter(TEXT("audit"), TEXT("appearance"), 0, Final) && Final.AppearanceJSON == Loaded.AppearanceJSON, TEXT("changed appearance survives reload"));
    Data->Store.Reset();
    if (NPC) NPC->Destroy();
    if (Replica) Replica->Destroy();
    if (Character) Character->Destroy();
    if (Controller) Controller->Destroy();
    FFileHelper::SaveStringToFile(Report, *(FPaths::ProjectSavedDir()/TEXT("OnsetAppearanceIntegrationAudit.txt")));
    UE_LOG(LogTemp, Display, TEXT("ONSET APPEARANCE INTEGRATION AUDIT: failures=%d\n%s"), Failures, *Report);
    World->DestroyWorld(false); return Failures ? 1 : 0;
}
