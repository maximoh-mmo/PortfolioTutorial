#include "UI/CharacterCreationScreen.h"

#include "OnsetAppearanceCatalog.h"
#include "OnsetAppearanceComponent.h"
#include "OnsetAppearanceLibrary.h"
#include "OnsetAppearancePreviewActor.h"
#include "Player/OnsetCharacterAppearanceComponent.h"
#include "Player/OnsetPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "CommonButtonBase.h"
#include "CommonTextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Viewport.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Components/EditableText.h"
#include "UObject/UnrealType.h"

void UCharacterCreationScreen::SetPlayerController(AOnsetPlayerController* Controller)
{
    if (CachedPlayerController == Controller) return;
    if (CachedPlayerController) CachedPlayerController->OnCharacterAppearanceResult.RemoveDynamic(this, &UCharacterCreationScreen::HandleAppearanceResult);
    CachedPlayerController = Controller;
    if (Controller) Controller->OnCharacterAppearanceResult.AddDynamic(this, &UCharacterCreationScreen::HandleAppearanceResult);
}

void UCharacterCreationScreen::SetSlotIndex(int32 Index) { SlotIndex = Index; }

void UCharacterCreationScreen::NativeOnActivated()
{
    Super::NativeOnActivated();
    if (!CachedPlayerController) SetPlayerController(GetOwningPlayer<AOnsetPlayerController>());
    EnsureDraft();
    BuildAppearancePanel();
    EnsureCreateButton();
    RefreshPreview();
}

void UCharacterCreationScreen::NativeOnDeactivated()
{
    if (PreviewActor) { PreviewActor->Destroy(); PreviewActor = nullptr; }
    Super::NativeOnDeactivated();
}

void UCharacterCreationScreen::NativeDestruct()
{
    SetPlayerController(nullptr);
    Super::NativeDestruct();
}

void UCharacterCreationScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (bDraftReady && PreviewViewport && !PreviewActor)
    {
        EnsurePreview();
        if (PreviewActor) RefreshPreview();
    }
}

void UCharacterCreationScreen::EnsureDraft()
{
    if (bDraftReady) return;
    AppearanceCatalog = UOnsetCharacterAppearanceComponent::LoadDefaultCatalog();
    FOnsetAppearanceRecipe Preset;
    TArray<FOnsetAppearanceIssue> Issues;
    if (AppearanceCatalog && UOnsetCharacterAppearanceComponent::GetPlayerPreset(0, Preset)
        && AppearanceCatalog->ValidateRecipe(Preset, EOnsetAppearanceUse::Player, Issues))
    { DraftAppearance = MoveTemp(Preset); bDraftReady = true; }
    else UE_LOG(LogTemp, Error, TEXT("Character creation: EpicHuman default preset is unavailable or invalid: %s"),
        Issues.IsEmpty() ? TEXT("catalog/preset load failed") : *Issues[0].Message);
}

UCommonTextBlock* UCharacterCreationScreen::AddLabel(UPanelWidget* Parent, const FText& Value)
{
    if (!Parent || !WidgetTree) return nullptr;
    auto* Label = WidgetTree->ConstructWidget<UCommonTextBlock>();
    static UClass* Style = LoadClass<UCommonTextStyle>(nullptr, TEXT("/Game/UI/Styles/SubtextStyle.SubtextStyle_C"));
    if (Style) Label->SetStyle(Style);
    Label->SetText(Value);
    Label->SetAutoWrapText(true);
    Parent->AddChild(Label);
    if (auto* BoxSlot = Cast<UVerticalBoxSlot>(Label->Slot)) BoxSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 3.f));
    return Label;
}

UCommonButtonBase* UCharacterCreationScreen::AddOption(UPanelWidget* Parent, const FText& Value, bool bSelected)
{
    if (!Parent || !GetOwningPlayer()) return nullptr;
    static UClass* ButtonClass = LoadClass<UCommonButtonBase>(nullptr, TEXT("/Game/UI/Components/WBP_ButtonBase.WBP_ButtonBase_C"));
    if (!ButtonClass) return nullptr;
    auto* Button = CreateWidget<UCommonButtonBase>(GetOwningPlayer(), ButtonClass);
    Parent->AddChild(Button);
    if (auto* Text = Cast<UCommonTextBlock>(Button->GetWidgetFromName(TEXT("ButtonText"))))
        Text->SetText(bSelected ? FText::Format(NSLOCTEXT("OnsetCreation", "Selected", "> {0}"), Value) : Value);
    if (auto* RowSlot = Cast<UHorizontalBoxSlot>(Button->Slot))
    { RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill)); RowSlot->SetPadding(FMargin(2.f, 0.f)); }
    else if (auto* ColumnSlot = Cast<UVerticalBoxSlot>(Button->Slot)) ColumnSlot->SetPadding(FMargin(0.f, 2.f));
    return Button;
}

void UCharacterCreationScreen::SetFeedback(const FText& Value)
{
    if (FeedbackText) FeedbackText->SetText(Value);
}

void UCharacterCreationScreen::BuildAppearancePanel()
{
    if (!bDraftReady || !WidgetTree || !AppearanceCatalog) return;
    auto* Panel = Cast<UVerticalBox>(GetWidgetFromName(TEXT("VerticalBox_3")));
    if (!Panel) return;
    if (!OptionsScroll)
    {
        if (auto* LegacyGrid = GetWidgetFromName(TEXT("ScaleBox_0"))) LegacyGrid->SetVisibility(ESlateVisibility::Collapsed);
        OptionsScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("OnsetAppearanceOptions"));
        OptionsScroll->SetScrollBarVisibility(ESlateVisibility::Visible);
        auto* ScrollSlot = Panel->AddChildToVerticalBox(OptionsScroll);
        ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ScrollSlot->SetPadding(FMargin(6.f, 0.f, 6.f, 4.f));
        OptionsList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("OnsetAppearanceOptionList"));
        OptionsScroll->AddChild(OptionsList);
        FeedbackText = AddLabel(Panel, FText::GetEmpty());
    }
    const float OldScroll = OptionsScroll->GetScrollOffset();
    OptionsList->ClearChildren();
    AddLabel(OptionsList, NSLOCTEXT("OnsetCreation", "Body", "BODY"));
    for (int32 Index = 0; Index < 2; ++Index)
    {
        const FName Body = Index == 0 ? TEXT("epic.adult.female") : TEXT("epic.adult.male");
        const FText Title = Index == 0 ? NSLOCTEXT("OnsetCreation", "Female", "Female") : NSLOCTEXT("OnsetCreation", "Male", "Male");
        if (auto* Button = AddOption(OptionsList, Title, DraftAppearance.BodyId == Body))
            Button->OnClicked().AddWeakLambda(this, [this, Index]() { SelectBodyPreset(Index); });
    }
    AddLabel(OptionsList, NSLOCTEXT("OnsetCreation", "Face", "FACE"));
    TArray<FName> Heads;
    for (const auto& Pair : AppearanceCatalog->Heads)
        if (Pair.Value.CompatibleBodies.Contains(DraftAppearance.BodyId)) Heads.Add(Pair.Key);
    Heads.Sort([](FName A, FName B) { return A.ToString() < B.ToString(); });
    for (int32 Index = 0; Index < Heads.Num(); ++Index)
    {
        const FName Id = Heads[Index];
        const FText Title = FText::Format(NSLOCTEXT("OnsetCreation", "FaceStyle", "Face {0}"), FText::AsNumber(Index + 1));
        if (auto* Button = AddOption(OptionsList, Title, DraftAppearance.HeadId == Id))
            Button->OnClicked().AddWeakLambda(this, [this, Id]() { SelectHead(Id); });
    }
    TMap<FName, TArray<FName>> BySlot;
    for (const auto& Pair : AppearanceCatalog->Parts)
        if (Pair.Value.CompatibleBodies.Contains(DraftAppearance.BodyId)) BySlot.FindOrAdd(Pair.Value.Slot).Add(Pair.Key);
    TArray<FName> Slots;
    BySlot.GetKeys(Slots);
    Slots.Sort([](FName A, FName B) { return A.ToString() < B.ToString(); });
    for (FName SlotId : Slots)
    {
        AddLabel(OptionsList, FText::FromName(SlotId));
        auto& Options = BySlot.FindChecked(SlotId);
        Options.Sort([](FName A, FName B) { return A.ToString() < B.ToString(); });
        const auto* Current = DraftAppearance.Parts.FindByPredicate([&](const FOnsetPartSelection& Part)
        { const auto* Definition = AppearanceCatalog->Parts.Find(Part.PartId); return Definition && Definition->Slot == SlotId; });
        if (SlotId.ToString().StartsWith(TEXT("Hair")) || SlotId == TEXT("HeadTop"))
            if (auto* Button = AddOption(OptionsList, NSLOCTEXT("OnsetCreation", "None", "None"), !Current))
                Button->OnClicked().AddWeakLambda(this, [this, SlotId]() { SelectPart(SlotId, NAME_None); });
        for (int32 Index = 0; Index < Options.Num(); ++Index)
        {
            const FName Id = Options[Index];
            const FText Title = FText::Format(NSLOCTEXT("OnsetCreation", "Style", "Style {0}"), FText::AsNumber(Index + 1));
            if (auto* Button = AddOption(OptionsList, Title, Current && Current->PartId == Id))
                Button->OnClicked().AddWeakLambda(this, [this, SlotId, Id]() { SelectPart(SlotId, Id); });
        }
    }
    auto MorphGroup = [this](const TArray<FName>& Morphs, bool bFace)
    {
        for (FName Id : Morphs)
        {
            const float Value = (bFace ? DraftAppearance.FaceMorphs : DraftAppearance.BodyMorphs).FindRef(Id);
            AddLabel(OptionsList, FText::Format(NSLOCTEXT("OnsetCreation", "Morph", "{0}: {1}%"), FText::FromName(Id), FText::AsNumber(FMath::RoundToInt(Value * 100.f))));
            auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
            OptionsList->AddChild(Row);
            if (auto* Minus = AddOption(Row, FText::FromString(TEXT("-"))))
                Minus->OnClicked().AddWeakLambda(this, [this, Id, bFace]() { AdjustMorph(Id, bFace, -0.25f); });
            if (auto* Plus = AddOption(Row, FText::FromString(TEXT("+"))))
                Plus->OnClicked().AddWeakLambda(this, [this, Id, bFace]() { AdjustMorph(Id, bFace, 0.25f); });
        }
    };
    if (const auto* Body = AppearanceCatalog->Bodies.Find(DraftAppearance.BodyId))
    { AddLabel(OptionsList, NSLOCTEXT("OnsetCreation", "BodyShape", "BODY SHAPE")); MorphGroup(Body->SupportedMorphs, false); }
    if (const auto* Head = AppearanceCatalog->Heads.Find(DraftAppearance.HeadId))
    { AddLabel(OptionsList, NSLOCTEXT("OnsetCreation", "FaceShape", "FACE SHAPE")); MorphGroup(Head->SupportedMorphs, true); }
    AddLabel(OptionsList, NSLOCTEXT("OnsetCreation", "Skin", "SKIN TONE"));
    const FLinearColor Skin[] = { {0.88f,0.69f,0.52f}, {0.75f,0.55f,0.38f}, {0.57f,0.38f,0.25f}, {0.35f,0.20f,0.13f} };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Skin); ++Index)
        if (auto* Button = AddOption(OptionsList, FText::Format(NSLOCTEXT("OnsetCreation", "Tone", "Tone {0}"), FText::AsNumber(Index + 1)), DraftAppearance.SkinColor.Equals(Skin[Index], 0.01f)))
        {
            if (auto* Swatch = Cast<UBorder>(Button->GetWidgetFromName(TEXT("AccentBar")))) Swatch->SetBrushColor(Skin[Index]);
            Button->OnClicked().AddWeakLambda(this, [this, Color = Skin[Index]]() { SetSkinColor(Color); });
        }
    AddLabel(OptionsList, NSLOCTEXT("OnsetCreation", "Eyes", "EYE COLOR"));
    const FLinearColor Eyes[] = { {0.12f,0.30f,0.50f}, {0.28f,0.20f,0.12f}, {0.14f,0.35f,0.20f}, {0.45f,0.40f,0.32f} };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Eyes); ++Index)
        if (auto* Button = AddOption(OptionsList, FText::Format(NSLOCTEXT("OnsetCreation", "Eye", "Eyes {0}"), FText::AsNumber(Index + 1)), DraftAppearance.EyeColor.Equals(Eyes[Index], 0.01f)))
        {
            if (auto* Swatch = Cast<UBorder>(Button->GetWidgetFromName(TEXT("AccentBar")))) Swatch->SetBrushColor(Eyes[Index]);
            Button->OnClicked().AddWeakLambda(this, [this, Color = Eyes[Index]]() { SetEyeColor(Color); });
        }
    const FLinearColor OutfitColors[] = {
        FLinearColor(0.13f, 0.24f, 0.33f), FLinearColor(0.38f, 0.12f, 0.14f),
        FLinearColor(0.22f, 0.29f, 0.17f), FLinearColor(0.55f, 0.46f, 0.32f)
    };
    for (int32 Channel = 0; Channel < 3; ++Channel)
    {
        const FText ChannelName = Channel == 0 ? NSLOCTEXT("OnsetCreation", "Primary", "OUTFIT PRIMARY")
            : Channel == 1 ? NSLOCTEXT("OnsetCreation", "Secondary", "OUTFIT SECONDARY")
            : NSLOCTEXT("OnsetCreation", "Accent", "OUTFIT ACCENT");
        AddLabel(OptionsList, ChannelName);
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(OutfitColors); ++Index)
        {
            const FLinearColor Color = OutfitColors[Index];
            if (auto* Button = AddOption(OptionsList, FText::Format(NSLOCTEXT("OnsetCreation", "Palette", "Palette {0}"), FText::AsNumber(Index + 1))))
            {
                if (auto* Swatch = Cast<UBorder>(Button->GetWidgetFromName(TEXT("AccentBar")))) Swatch->SetBrushColor(Color);
                Button->OnClicked().AddWeakLambda(this, [this, Channel, Color]() { SetOutfitColor(Channel, Color); });
            }
        }
    }
    OptionsScroll->SetScrollOffset(OldScroll);
}

void UCharacterCreationScreen::EnsurePreview()
{
    if (!bDraftReady || !WidgetTree) return;
    if (!PreviewViewport)
    {
        auto* Center = Cast<UCanvasPanel>(GetWidgetFromName(TEXT("CanvasPanel_0")));
        if (!Center) return;
        if (auto* OldImage = GetWidgetFromName(TEXT("ScaleBox_1"))) OldImage->SetVisibility(ESlateVisibility::Collapsed);
        PreviewViewport = WidgetTree->ConstructWidget<UViewport>(UViewport::StaticClass(), TEXT("OnsetAppearancePreview"));
        PreviewViewport->SetBackgroundColor(FLinearColor(0.025f, 0.045f, 0.07f, 1.f));
        auto* PreviewSlot = Center->AddChildToCanvas(PreviewViewport);
        PreviewSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
        PreviewSlot->SetOffsets(FMargin(0.f));
        PreviewSlot->SetZOrder(-1);
    }
    if (!PreviewActor && PreviewViewport->GetViewportWorld())
    {
        // The lighting setters access UViewport's Slate widget directly. Wait until
        // the widget has been constructed, which may be a tick after activation.
        PreviewViewport->SetLightIntensity(3.f);
        PreviewViewport->SetSkyIntensity(1.f);
        PreviewViewport->SetViewLocation(FVector(240.f, 0.f, 95.f));
        PreviewViewport->SetViewRotation(FRotator(0.f, 180.f, 0.f));
        PreviewActor = Cast<AOnsetAppearancePreviewActor>(PreviewViewport->Spawn(AOnsetAppearancePreviewActor::StaticClass()));
        if (PreviewActor) { PreviewActor->Use = EOnsetAppearanceUse::Player; PreviewActor->Catalog = AppearanceCatalog; }
    }
}

void UCharacterCreationScreen::RefreshPreview()
{
    EnsurePreview();
    if (!PreviewActor) return;
    PreviewActor->Recipe = DraftAppearance;
    if (!PreviewActor->RebuildPreview())
    {
        const TArray<FOnsetAppearanceIssue>& Issues = PreviewActor->Appearance->LastIssues;
        const FString Reason = Issues.IsEmpty() ? TEXT("unknown build failure") : Issues[0].Message;
        UE_LOG(LogTemp, Error, TEXT("Character creation preview failed: body=%s reason=%s"),
            *DraftAppearance.BodyId.ToString(), *Reason);
        SetFeedback(FText::FromString(FString::Printf(TEXT("Appearance preview unavailable: %s"), *Reason)));
        return;
    }
    const USkeletalMeshComponent* Body = PreviewActor->Appearance->GetBodyMesh();
    UE_LOG(LogTemp, Display, TEXT("Character creation preview: body=%s mesh=%s"),
        *DraftAppearance.BodyId.ToString(), *GetPathNameSafe(Body ? Body->GetSkeletalMeshAsset() : nullptr));
}

void UCharacterCreationScreen::EnsureCreateButton()
{
    if (NativeCreateButton || !GetOwningPlayer()) return;
    auto* LegacyButton = Cast<UCommonButtonBase>(GetWidgetFromName(TEXT("CreateCharacterButton")));
    auto* Footer = LegacyButton ? LegacyButton->GetParent() : nullptr;
    if (!Footer) return;

    // The Blueprint click graph still requires a selected legacy preset and deactivates
    // the menu immediately. Keep its visual style, but own the new transaction in C++.
    LegacyButton->SetVisibility(ESlateVisibility::Collapsed);
    NativeCreateButton = CreateWidget<UCommonButtonBase>(GetOwningPlayer(), LegacyButton->GetClass(), TEXT("OnsetNativeCreateButton"));
    if (!NativeCreateButton) return;
    Footer->AddChild(NativeCreateButton);
    if (auto* Label = Cast<UCommonTextBlock>(NativeCreateButton->GetWidgetFromName(TEXT("ButtonText"))))
        Label->SetText(NSLOCTEXT("OnsetCreation", "CreateOperative", "CREATE OPERATIVE"));
    NativeCreateButton->OnClicked().AddWeakLambda(this, [this]() { HandleNativeCreateClicked(); });
}

void UCharacterCreationScreen::HandleNativeCreateClicked()
{
    auto* NameField = Cast<UEditableText>(GetWidgetFromName(TEXT("CharacterName")));
    if (!NameField) { SetFeedback(NSLOCTEXT("OnsetCreation", "MissingNameField", "Callsign control is unavailable.")); return; }

    EOnsetCharacterClass ChosenClass = EOnsetCharacterClass::Tank;
    bool bFoundClass = false;
    for (TFieldIterator<FProperty> It(GetClass(), EFieldIterationFlags::IncludeSuper); It; ++It)
    {
        FProperty* Property = *It;
        if (const auto* EnumProperty = CastField<FEnumProperty>(Property))
        {
            if (EnumProperty->GetEnum() == StaticEnum<EOnsetCharacterClass>())
            {
                ChosenClass = static_cast<EOnsetCharacterClass>(EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(EnumProperty->ContainerPtrToValuePtr<void>(this)));
                bFoundClass = true;
                break;
            }
        }
        else if (const auto* ByteProperty = CastField<FByteProperty>(Property))
        {
            if (ByteProperty->Enum == StaticEnum<EOnsetCharacterClass>())
            {
                ChosenClass = static_cast<EOnsetCharacterClass>(ByteProperty->GetPropertyValue_InContainer(this));
                bFoundClass = true;
                break;
            }
        }
    }
    if (!bFoundClass) { SetFeedback(NSLOCTEXT("OnsetCreation", "MissingClassField", "Choose a class.")); return; }
    CreateCharacter(NameField->GetText().ToString(), ChosenClass, 0);
}

void UCharacterCreationScreen::CommitDraft(FOnsetAppearanceRecipe Candidate)
{
    if (!AppearanceCatalog) return;
    TArray<FOnsetAppearanceIssue> Issues;
    if (!AppearanceCatalog->ValidateRecipe(Candidate, EOnsetAppearanceUse::Player, Issues))
    { SetFeedback(Issues.IsEmpty() ? NSLOCTEXT("OnsetCreation", "Invalid", "Invalid appearance.") : FText::FromString(Issues[0].Message)); return; }
    DraftAppearance = MoveTemp(Candidate);
    SetFeedback(FText::GetEmpty());
    BuildAppearancePanel();
    RefreshPreview();
}

void UCharacterCreationScreen::SelectBodyPreset(int32 Index)
{
    FOnsetAppearanceRecipe Candidate;
    if (!UOnsetCharacterAppearanceComponent::GetPlayerPreset(Index, Candidate)) return;
    Candidate.SkinColor = DraftAppearance.SkinColor;
    Candidate.EyeColor = DraftAppearance.EyeColor;
    CommitDraft(MoveTemp(Candidate));
}

void UCharacterCreationScreen::SelectHead(FName Id)
{
    if (!AppearanceCatalog) return;
    const auto* Head = AppearanceCatalog->Heads.Find(Id);
    if (!Head || !Head->CompatibleBodies.Contains(DraftAppearance.BodyId)) return;
    FOnsetAppearanceRecipe Candidate = DraftAppearance;
    Candidate.HeadId = Id;
    Candidate.FaceMorphs.Reset();
    CommitDraft(MoveTemp(Candidate));
}

void UCharacterCreationScreen::SelectPart(FName PartSlot, FName Id)
{
    if (!AppearanceCatalog) return;
    if (!Id.IsNone())
    {
        const auto* Definition = AppearanceCatalog->Parts.Find(Id);
        if (!Definition || Definition->Slot != PartSlot || !Definition->CompatibleBodies.Contains(DraftAppearance.BodyId)) return;
    }
    FOnsetAppearanceRecipe Candidate = DraftAppearance;
    FOnsetPartSelection Previous;
    bool bHadPrevious = false;
    if (const FOnsetPartSelection* Existing = Candidate.Parts.FindByPredicate([&](const FOnsetPartSelection& Part)
        { const auto* Definition = AppearanceCatalog->Parts.Find(Part.PartId); return Definition && Definition->Slot == PartSlot; }))
    { Previous = *Existing; bHadPrevious = true; }
    Candidate.Parts.RemoveAll([&](const FOnsetPartSelection& Part)
    { const auto* Definition = AppearanceCatalog->Parts.Find(Part.PartId); return Definition && Definition->Slot == PartSlot; });
    if (!Id.IsNone())
    {
        FOnsetPartSelection Selection = bHadPrevious ? Previous : FOnsetPartSelection();
        Selection.PartId = Id;
        Candidate.Parts.Add(Selection);
    }
    CommitDraft(MoveTemp(Candidate));
}

void UCharacterCreationScreen::AdjustMorph(FName Id, bool bFace, float Delta)
{
    if (!AppearanceCatalog || !FMath::IsFinite(Delta)) return;
    const TArray<FName>* Supported = nullptr;
    if (bFace) { if (const auto* Head = AppearanceCatalog->Heads.Find(DraftAppearance.HeadId)) Supported = &Head->SupportedMorphs; }
    else if (const auto* Body = AppearanceCatalog->Bodies.Find(DraftAppearance.BodyId)) Supported = &Body->SupportedMorphs;
    if (!Supported || !Supported->Contains(Id)) return;
    FOnsetAppearanceRecipe Candidate = DraftAppearance;
    auto& Values = bFace ? Candidate.FaceMorphs : Candidate.BodyMorphs;
    const float Value = FMath::Clamp(Values.FindRef(Id) + Delta, 0.f, 1.f);
    if (Value <= KINDA_SMALL_NUMBER) Values.Remove(Id); else Values.Add(Id, Value);
    CommitDraft(MoveTemp(Candidate));
}

void UCharacterCreationScreen::SetSkinColor(FLinearColor Color)
{ FOnsetAppearanceRecipe Candidate = DraftAppearance; Candidate.SkinColor = Color; CommitDraft(MoveTemp(Candidate)); }

void UCharacterCreationScreen::SetEyeColor(FLinearColor Color)
{ FOnsetAppearanceRecipe Candidate = DraftAppearance; Candidate.EyeColor = Color; CommitDraft(MoveTemp(Candidate)); }

void UCharacterCreationScreen::SetOutfitColor(int32 Channel, FLinearColor Color)
{
    if (Channel < 0 || Channel > 2 || !AppearanceCatalog) return;
    FOnsetAppearanceRecipe Candidate = DraftAppearance;
    for (FOnsetPartSelection& Part : Candidate.Parts)
    {
        const FOnsetPartDefinition* Definition = AppearanceCatalog->Parts.Find(Part.PartId);
        if (!Definition || Definition->bHair) continue;
        Part.bOverrideColors = true;
        if (Channel == 0) Part.ColorA = Color;
        else if (Channel == 1) Part.ColorB = Color;
        else Part.ColorC = Color;
    }
    CommitDraft(MoveTemp(Candidate));
}

void UCharacterCreationScreen::CreateCharacter(const FString& Name, EOnsetCharacterClass Class, int32 LegacyPresetIndex)
{
    // The existing Blueprint passes the old grid index; the validated draft now owns the appearance.
    EnsureDraft();
    FString JSON;
    if (!bDraftReady || !UOnsetAppearanceLibrary::EncodeRecipe(DraftAppearance, JSON))
    { SetFeedback(NSLOCTEXT("OnsetCreation", "NoAppearance", "Appearance options are unavailable.")); return; }
    CreateCharacterWithAppearance(Name, Class, JSON);
}

void UCharacterCreationScreen::CreateCharacterWithAppearance(const FString& Name, EOnsetCharacterClass Class, const FString& JSON)
{
    if (bRequestPending || !CachedPlayerController || SlotIndex < 0) return;
    if (Name.TrimStartAndEnd().IsEmpty()) { SetFeedback(NSLOCTEXT("OnsetCreation", "NameRequired", "Enter a callsign.")); return; }
    if (!StaticEnum<EOnsetCharacterClass>()->IsValidEnumValue(static_cast<int64>(Class)))
    { SetFeedback(NSLOCTEXT("OnsetCreation", "ClassRequired", "Choose a class.")); return; }
    FOnsetAppearanceRecipe Validated;
    TArray<FOnsetAppearanceIssue> Issues;
    if (!UOnsetAppearanceLibrary::DecodeAndValidateRecipe(JSON, AppearanceCatalog, EOnsetAppearanceUse::Player, Validated, Issues))
    { SetFeedback(Issues.IsEmpty() ? NSLOCTEXT("OnsetCreation", "Invalid", "Invalid appearance.") : FText::FromString(Issues[0].Message)); return; }
    bRequestPending = true;
    if (NativeCreateButton) NativeCreateButton->SetIsEnabled(false);
    SetFeedback(NSLOCTEXT("OnsetCreation", "Creating", "Creating operative..."));
    CachedPlayerController->Server_CreateCharacterWithAppearance(SlotIndex, Name, Class, JSON);
}

void UCharacterCreationScreen::HandleAppearanceResult(bool bSuccess, const TArray<FOnsetAppearanceIssue>& Issues)
{
    if (!bRequestPending) return;
    bRequestPending = false;
    if (NativeCreateButton) NativeCreateButton->SetIsEnabled(true);
    SetFeedback(bSuccess ? NSLOCTEXT("OnsetCreation", "Created", "Operative created.")
        : Issues.IsEmpty() ? NSLOCTEXT("OnsetCreation", "Failed", "Character creation failed.") : FText::FromString(Issues[0].Message));
}
