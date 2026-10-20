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
#include "UI/CharacterCreationScreen.h"
#include "UI/MHTerminalTextInputController.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "CommonButtonBase.h"
#include "UI/OnsetRootLayout.h"
#include "UI/MainMenuScreen.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableText.h"
#include "Components/Image.h"

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
    if (!OriginalAnimClass)
    {
        OriginalAnimClass = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"));
    }
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
    // Audit Terminal Text Input and Cursor Blink jitter-free behavior on WBP_CharacterCreation
    auto* CreationClass = LoadClass<UCharacterCreationScreen>(nullptr, TEXT("/Game/UI/Screens/WBP_CharacterCreation.WBP_CharacterCreation_C"));
    Check(CreationClass != nullptr, TEXT("load WBP_CharacterCreation class"));
    if (CreationClass)
    {
        auto* CreationScreen = CreateWidget<UCharacterCreationScreen>(World, CreationClass);
        Check(CreationScreen != nullptr, TEXT("instantiate WBP_CharacterCreation"));
        if (CreationScreen)
        {
            CreationScreen->InitializeTerminalInput();

            UMHTerminalInputController* TerminalCtrl = CreationScreen->GetTerminalInputController();
            Check(TerminalCtrl != nullptr, TEXT("terminal input controller initialized"));

            auto* DisplayText = Cast<UCommonTextBlock>(CreationScreen->GetWidgetFromName(TEXT("text")));
            auto* CursorText = Cast<UCommonTextBlock>(CreationScreen->GetWidgetFromName(TEXT("text_1")));
            auto* CursorBorder = Cast<UBorder>(CreationScreen->GetWidgetFromName(TEXT("CursorTextBorder")));
            auto* NameField = Cast<UEditableText>(CreationScreen->GetWidgetFromName(TEXT("CharacterName")));

            Check(DisplayText != nullptr, TEXT("find DisplayText (text)"));
            Check(CursorText != nullptr, TEXT("find CursorText (text_1)"));
            Check(CursorBorder != nullptr, TEXT("find CursorBorder"));
            Check(NameField != nullptr, TEXT("find CharacterName"));

            if (DisplayText && CursorText && CursorBorder && NameField && TerminalCtrl)
            {
                // Verify initial placeholder
                Check(TerminalCtrl->IsPlaceholderActive(), TEXT("initial placeholder active"));
                Check(DisplayText->GetText().ToString() == TEXT("Kael"), TEXT("initial placeholder text is Kael"));
                Check(CursorText->GetText().ToString() == TEXT("_"), TEXT("cursor glyph is underscore"));

                UCanvasPanelSlot* CursorSlot = Cast<UCanvasPanelSlot>(CursorBorder->Slot);
                Check(CursorSlot != nullptr, TEXT("cursor border has canvas slot"));
                const float InitialCursorX = CursorSlot ? CursorSlot->GetPosition().X : 0.0f;
                Report += FString::Printf(TEXT("Initial cursor X position: %.2f (for Kael)\n"), InitialCursorX);

                // Simulate typing text
                NameField->SetText(FText::FromString(TEXT("TestOperative")));
                NameField->OnTextChanged.Broadcast(FText::FromString(TEXT("TestOperative")));

                Check(!TerminalCtrl->IsPlaceholderActive(), TEXT("placeholder deactivated after text entered"));
                Check(DisplayText->GetText().ToString() == TEXT("TestOperative"), TEXT("display text updated to TestOperative"));
                Check(TerminalCtrl->GetFinalText() == TEXT("TestOperative"), TEXT("final text returns typed operative name"));

                const float TypedCursorX = CursorSlot ? CursorSlot->GetPosition().X : 0.0f;
                Report += FString::Printf(TEXT("Typed cursor X position: %.2f (for TestOperative)\n"), TypedCursorX);
                Check(TypedCursorX > InitialCursorX, TEXT("cursor moved right after typing longer string"));

                // Simulate clearing text
                NameField->SetText(FText::GetEmpty());
                NameField->OnTextChanged.Broadcast(FText::GetEmpty());

                Check(TerminalCtrl->IsPlaceholderActive(), TEXT("placeholder restored on empty text"));
                Check(DisplayText->GetText().ToString() == TEXT("Kael"), TEXT("placeholder text Kael restored"));
                Check(TerminalCtrl->GetFinalText() == TEXT("Kael"), TEXT("final text returns default Kael on empty"));

                // Verify legacy Blueprint timer is cleared
                if (FProperty* BlinkProp = CreationClass->FindPropertyByName(TEXT("BlinkTimer")))
                {
                    if (FStructProperty* StructProp = CastField<FStructProperty>(BlinkProp))
                    {
                        FTimerHandle* HandlePtr = StructProp->ContainerPtrToValuePtr<FTimerHandle>(CreationScreen);
                        Check(HandlePtr && !HandlePtr->IsValid(), TEXT("legacy blueprint BlinkTimer is cleared"));
                    }
                }
            }

            CreationScreen->RemoveFromParent();
        }
    }
    // Audit WBP_RootLayout and WBP_MainMenu hierarchy and focus
    auto* RootLayoutClass = LoadClass<UOnsetRootLayout>(nullptr, TEXT("/Game/UI/Core/WBP_RootLayout.WBP_RootLayout_C"));
    Check(RootLayoutClass != nullptr, TEXT("load WBP_RootLayout class"));
    auto* MainMenuClass = LoadClass<UMainMenuScreen>(nullptr, TEXT("/Game/UI/Screens/WBP_MainMenu.WBP_MainMenu_C"));
    Check(MainMenuClass != nullptr, TEXT("load WBP_MainMenu class"));
    if (RootLayoutClass && MainMenuClass)
    {
        auto* RootLayout = CreateWidget<UOnsetRootLayout>(World, RootLayoutClass);
        Check(RootLayout != nullptr, TEXT("instantiate WBP_RootLayout"));
        if (RootLayout)
        {
            auto* GameStack = RootLayout->GetStackForLayer(EOnsetUILayer::Game);
            Check(GameStack != nullptr, TEXT("GameLayerStack bound on WBP_RootLayout"));
            auto* MenuStack = RootLayout->GetStackForLayer(EOnsetUILayer::Menu);
            Check(MenuStack != nullptr, TEXT("MenuLayerStack bound on WBP_RootLayout"));
            auto* ModalStack = RootLayout->GetStackForLayer(EOnsetUILayer::Modal);
            Check(ModalStack != nullptr, TEXT("ModalLayerStack bound on WBP_RootLayout"));

            if (RootLayout->WidgetTree)
            {
                TArray<UWidget*> RootWidgets;
                RootLayout->WidgetTree->GetAllWidgets(RootWidgets);
                Report += FString::Printf(TEXT("RootLayout total widgets: %d\n"), RootWidgets.Num());
                for (UWidget* W : RootWidgets)
                {
                    Report += FString::Printf(TEXT("  RootLayout Widget: %s (%s) Vis=%d Enabled=%d\n"),
                        *W->GetName(), *W->GetClass()->GetName(), (int32)W->GetVisibility(), W->GetIsEnabled());
                }
            }

            auto* MainMenu = CreateWidget<UMainMenuScreen>(World, MainMenuClass);
            Check(MainMenu != nullptr, TEXT("instantiate WBP_MainMenu"));
            if (MainMenu)
            {
                Report += FString::Printf(TEXT("MainMenu widget tree root: %s (%s)\n"),
                    MainMenu->WidgetTree && MainMenu->WidgetTree->RootWidget ? *MainMenu->WidgetTree->RootWidget->GetName() : TEXT("null"),
                    MainMenu->WidgetTree && MainMenu->WidgetTree->RootWidget ? *MainMenu->WidgetTree->RootWidget->GetClass()->GetName() : TEXT("null"));

                if (MainMenu->WidgetTree)
                {
                    TArray<UWidget*> AllWidgets;
                    MainMenu->WidgetTree->GetAllWidgets(AllWidgets);
                    Report += FString::Printf(TEXT("MainMenu total widgets: %d\n"), AllWidgets.Num());
                    for (UWidget* W : AllWidgets)
                    {
                        UCommonButtonBase* CommonBtn = Cast<UCommonButtonBase>(W);
                        Report += FString::Printf(TEXT("  Widget: %s (%s) Vis=%d Enabled=%d CommonBtn=%d\n"),
                            *W->GetName(), *W->GetClass()->GetName(), (int32)W->GetVisibility(), W->GetIsEnabled(), CommonBtn != nullptr);
                    }
                }

                auto* FocusTarget = MainMenu->GetDesiredFocusTarget();
                Report += FString::Printf(TEXT("MainMenu GetDesiredFocusTarget: %s\n"),
                    FocusTarget ? *FocusTarget->GetName() : TEXT("null"));

                UWidget* ConnectWidget = MainMenu->GetWidgetFromName(TEXT("ConnectButton"));
                UCommonButtonBase* ConnectBtn = Cast<UCommonButtonBase>(ConnectWidget);
                Report += FString::Printf(TEXT("ConnectButton resolved: Widget=%s CommonBtn=%s\n"),
                    ConnectWidget ? *ConnectWidget->GetName() : TEXT("null"),
                    ConnectBtn ? *ConnectBtn->GetName() : TEXT("null"));
                if (ConnectWidget)
                {
                    Report += FString::Printf(TEXT("  ConnectWidget: Vis=%d Enabled=%d IsVisible=%d\n"),
                        (int32)ConnectWidget->GetVisibility(), ConnectWidget->GetIsEnabled(), ConnectWidget->IsVisible());
                    if (UUserWidget* UserBtn = Cast<UUserWidget>(ConnectWidget))
                    {
                        if (UserBtn->WidgetTree)
                        {
                            TArray<UWidget*> BtnWidgets;
                            UserBtn->WidgetTree->GetAllWidgets(BtnWidgets);
                            Report += FString::Printf(TEXT("  ConnectButton child widgets: %d\n"), BtnWidgets.Num());
                            for (UWidget* BW : BtnWidgets)
                            {
                                Report += FString::Printf(TEXT("    Child: %s (%s) Vis=%d Enabled=%d\n"),
                                    *BW->GetName(), *BW->GetClass()->GetName(), (int32)BW->GetVisibility(), BW->GetIsEnabled());
                            }
                        }
                    }
                }

                for (const TCHAR* ImgName : { TEXT("Image_206"), TEXT("Image_96"), TEXT("Image_561") })
                {
                    if (UImage* Img = Cast<UImage>(MainMenu->GetWidgetFromName(ImgName)))
                    {
                        UObject* Res = Img->GetBrush().GetResourceObject();
                        Report += FString::Printf(TEXT("  %s: Res=%s Vis=%d\n"),
                            ImgName, Res ? *Res->GetPathName() : TEXT("null"), (int32)Img->GetVisibility());
                    }
                }

                MainMenu->RemoveFromParent();
            }

            // Test pushing MainMenu onto GameLayerStack
            auto* PushedScreen = Cast<UMainMenuScreen>(GameStack->AddWidget(MainMenuClass));
            Check(PushedScreen != nullptr, TEXT("GameStack->AddWidget(MainMenuClass) succeeds"));
            if (PushedScreen)
            {
                Report += FString::Printf(TEXT("PushedScreen: %s IsActive=%d\n"),
                    *PushedScreen->GetName(), PushedScreen->IsActivated());
                auto* ActiveTarget = PushedScreen->GetDesiredFocusTarget();
                Report += FString::Printf(TEXT("PushedScreen GetDesiredFocusTarget: %s\n"),
                    ActiveTarget ? *ActiveTarget->GetName() : TEXT("null"));
            }

            RootLayout->RemoveFromParent();
        }
    }
    if (Controller) Controller->Destroy();
    FFileHelper::SaveStringToFile(Report, *(FPaths::ProjectSavedDir()/TEXT("OnsetAppearanceIntegrationAudit.txt")));
    UE_LOG(LogTemp, Display, TEXT("ONSET APPEARANCE INTEGRATION AUDIT: failures=%d\n%s"), Failures, *Report);
    World->DestroyWorld(false); return Failures ? 1 : 0;
}
