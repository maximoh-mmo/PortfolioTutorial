#include "UI/CharacterCreationScreen.h"
#include "UI/MHTerminalTextInputController.h"

#include "OnsetAppearanceCatalog.h"
#include "OnsetAppearanceComponent.h"
#include "OnsetAppearanceLibrary.h"
#include "OnsetAppearancePreviewActor.h"
#include "UI/OnsetAppearancePreviewRig.h"
#include "Player/OnsetCharacterAppearanceComponent.h"
#include "Player/OnsetPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetNavigation.h"
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
#include "Components/Image.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Components/EditableText.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
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
    InitializeTerminalInput();
    SetupNavigationRules();
    RefreshPreview();
}

void UCharacterCreationScreen::NativeOnDeactivated()
{
    if (TerminalInputController) { TerminalInputController->Shutdown(); }
    if (PreviewActor) { PreviewActor->Destroy(); PreviewActor = nullptr; }
    if (PreviewRigActor) { PreviewRigActor->Destroy(); PreviewRigActor = nullptr; }
    PreviewCaptureComponent = nullptr;
    PreviewRenderTarget = nullptr;
    PreviewMaterialInstance = nullptr;
    PreviewImage = nullptr;
    bIsDraggingPreview = false;
    Super::NativeOnDeactivated();
}

void UCharacterCreationScreen::NativeDestruct()
{
    if (TerminalInputController) { TerminalInputController->Shutdown(); }
    if (PreviewActor) { PreviewActor->Destroy(); PreviewActor = nullptr; }
    if (PreviewRigActor) { PreviewRigActor->Destroy(); PreviewRigActor = nullptr; }
    PreviewCaptureComponent = nullptr;
    PreviewRenderTarget = nullptr;
    PreviewMaterialInstance = nullptr;
    PreviewImage = nullptr;
    bIsDraggingPreview = false;
    SetPlayerController(nullptr);
    Super::NativeDestruct();
}

void UCharacterCreationScreen::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (bDraftReady && !PreviewActor)
    {
        EnsurePreview();
        if (PreviewActor) RefreshPreview();
    }

    APlayerController* PC = CachedPlayerController ? CachedPlayerController.Get() : GetOwningPlayer();
    if (PC && PreviewActor)
    {
        const float RightX = PC->GetInputAnalogKeyState(EKeys::Gamepad_RightX);
        if (FMath::Abs(RightX) > 0.15f)
        {
            const float YawDelta = -RightX * 120.f * InDeltaTime;
            PreviewActor->AddActorLocalRotation(FRotator(0.f, YawDelta, 0.f));
        }
    }
}

FReply UCharacterCreationScreen::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        const auto* Center = Cast<UCanvasPanel>(GetWidgetFromName(TEXT("Preview")));
        const bool bOverImage = PreviewImage && PreviewImage->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition());
        const bool bOverCanvas = Center && Center->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition());

        if (bOverImage || bOverCanvas)
        {
            bIsDraggingPreview = true;
            return FReply::Handled().CaptureMouse(TakeWidget());
        }
    }
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UCharacterCreationScreen::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsDraggingPreview)
    {
        bIsDraggingPreview = false;
        return FReply::Handled().ReleaseMouseCapture();
    }
    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UCharacterCreationScreen::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (bIsDraggingPreview && PreviewActor)
    {
        const float DeltaX = InMouseEvent.GetCursorDelta().X;
        if (FMath::Abs(DeltaX) > KINDA_SMALL_NUMBER)
        {
            const float YawDelta = -DeltaX * 0.4f;
            PreviewActor->AddActorLocalRotation(FRotator(0.f, YawDelta, 0.f));
            return FReply::Handled();
        }
    }
    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UCharacterCreationScreen::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
    bIsDraggingPreview = false;
    Super::NativeOnMouseCaptureLost(CaptureLostEvent);
}

UWidget* UCharacterCreationScreen::NativeGetDesiredFocusTarget() const
{
    // 1. First preference: Selected class button on the left panel (or first available class button)
    if (UWidget* ClassTarget = GetSelectedClassButton())
    {
        if (ClassTarget->IsVisible() && ClassTarget->GetIsEnabled())
        {
            return ClassTarget;
        }
    }

    // 2. Second preference: Callsign input field in the center panel
    if (UWidget* NameInput = Cast<UWidget>(GetWidgetFromName(TEXT("CharacterName"))))
    {
        if (NameInput->IsVisible() && NameInput->GetIsEnabled())
        {
            return NameInput;
        }
    }

    // 3. Fall back to first appearance option
    if (UWidget* AppTarget = GetActiveAppearanceOption())
    {
        return AppTarget;
    }

    return Super::NativeGetDesiredFocusTarget();
}

TArray<UWidget*> UCharacterCreationScreen::GetClassButtons() const
{
    TArray<UWidget*> Buttons;
    const TCHAR* ClassButtonNames[] = { TEXT("Tank"), TEXT("DPS"), TEXT("Ranged"), TEXT("Support") };
    for (const TCHAR* Name : ClassButtonNames)
    {
        if (UWidget* Btn = Cast<UWidget>(GetWidgetFromName(Name)))
        {
            if (Btn->IsVisible() && Btn->GetIsEnabled())
            {
                Buttons.Add(Btn);
            }
        }
    }
    return Buttons;
}

UWidget* UCharacterCreationScreen::GetSelectedClassButton() const
{
    const TCHAR* ClassButtonNames[] = { TEXT("Tank"), TEXT("DPS"), TEXT("Ranged"), TEXT("Support") };
    for (const TCHAR* Name : ClassButtonNames)
    {
        if (UCommonButtonBase* Btn = Cast<UCommonButtonBase>(GetWidgetFromName(Name)))
        {
            if (Btn->IsVisible() && Btn->GetIsEnabled() && Btn->GetSelected())
            {
                return Btn;
            }
        }
    }
    for (const TCHAR* Name : ClassButtonNames)
    {
        if (UWidget* Btn = Cast<UWidget>(GetWidgetFromName(Name)))
        {
            if (Btn->IsVisible() && Btn->GetIsEnabled())
            {
                return Btn;
            }
        }
    }
    return nullptr;
}

UWidget* UCharacterCreationScreen::GetActiveAppearanceOption() const
{
    if (LastFocusedAppearanceWidget && LastFocusedAppearanceWidget->IsVisible() && LastFocusedAppearanceWidget->GetIsEnabled())
    {
        return LastFocusedAppearanceWidget.Get();
    }
    if (OptionsList)
    {
        for (int32 Index = 0; Index < OptionsList->GetChildrenCount(); ++Index)
        {
            UWidget* Child = OptionsList->GetChildAt(Index);
            if (UCommonButtonBase* Btn = Cast<UCommonButtonBase>(Child))
            {
                if (Btn->IsVisible() && Btn->GetIsEnabled() && Btn->GetIsFocusable())
                {
                    return Btn;
                }
            }
            else if (UHorizontalBox* Row = Cast<UHorizontalBox>(Child))
            {
                for (int32 Col = 0; Col < Row->GetChildrenCount(); ++Col)
                {
                    if (UCommonButtonBase* RowBtn = Cast<UCommonButtonBase>(Row->GetChildAt(Col)))
                    {
                        if (RowBtn->IsVisible() && RowBtn->GetIsEnabled() && RowBtn->GetIsFocusable())
                        {
                            return RowBtn;
                        }
                    }
                }
            }
        }
    }
    return nullptr;
}

void UCharacterCreationScreen::SetupNavigationRules()
{
    UWidget* TankBtn = Cast<UWidget>(GetWidgetFromName(TEXT("Tank")));
    UWidget* DpsBtn = Cast<UWidget>(GetWidgetFromName(TEXT("DPS")));
    UWidget* RangedBtn = Cast<UWidget>(GetWidgetFromName(TEXT("Ranged")));
    UWidget* SupportBtn = Cast<UWidget>(GetWidgetFromName(TEXT("Support")));
    UWidget* NameInput = Cast<UWidget>(GetWidgetFromName(TEXT("CharacterName")));
    UWidget* BackBtn = Cast<UWidget>(GetWidgetFromName(TEXT("BackButton")));
    UWidget* CreateBtn = NativeCreateButton ? Cast<UWidget>(NativeCreateButton) : Cast<UWidget>(GetWidgetFromName(TEXT("CreateCharacterButton")));
    UWidget* FirstAppBtn = GetActiveAppearanceOption();

    // 1. Left panel: Class buttons (Tank -> DPS -> Ranged -> Support)
    if (TankBtn)
    {
        if (DpsBtn) TankBtn->SetNavigationRuleExplicit(EUINavigation::Down, DpsBtn);
        if (NameInput) TankBtn->SetNavigationRuleExplicit(EUINavigation::Right, NameInput);
        TankBtn->SetNavigationRuleBase(EUINavigation::Up, EUINavigationRule::Stop);
        TankBtn->SetNavigationRuleBase(EUINavigation::Left, EUINavigationRule::Stop);
    }
    if (DpsBtn)
    {
        if (TankBtn) DpsBtn->SetNavigationRuleExplicit(EUINavigation::Up, TankBtn);
        if (RangedBtn) DpsBtn->SetNavigationRuleExplicit(EUINavigation::Down, RangedBtn);
        if (NameInput) DpsBtn->SetNavigationRuleExplicit(EUINavigation::Right, NameInput);
        DpsBtn->SetNavigationRuleBase(EUINavigation::Left, EUINavigationRule::Stop);
    }
    if (RangedBtn)
    {
        if (DpsBtn) RangedBtn->SetNavigationRuleExplicit(EUINavigation::Up, DpsBtn);
        if (SupportBtn) RangedBtn->SetNavigationRuleExplicit(EUINavigation::Down, SupportBtn);
        if (NameInput) RangedBtn->SetNavigationRuleExplicit(EUINavigation::Right, NameInput);
        RangedBtn->SetNavigationRuleBase(EUINavigation::Left, EUINavigationRule::Stop);
    }
    if (SupportBtn)
    {
        if (RangedBtn) SupportBtn->SetNavigationRuleExplicit(EUINavigation::Up, RangedBtn);
        if (BackBtn) SupportBtn->SetNavigationRuleExplicit(EUINavigation::Down, BackBtn);
        if (NameInput) SupportBtn->SetNavigationRuleExplicit(EUINavigation::Right, NameInput);
        SupportBtn->SetNavigationRuleBase(EUINavigation::Left, EUINavigationRule::Stop);
    }

    // 2. Center panel: Callsign input
    if (NameInput)
    {
        UWidget* SelectedClass = GetSelectedClassButton();
        if (!SelectedClass) SelectedClass = TankBtn;
        if (SelectedClass)
        {
            NameInput->SetNavigationRuleExplicit(EUINavigation::Left, SelectedClass);
            NameInput->SetNavigationRuleExplicit(EUINavigation::Up, SelectedClass);
        }
        if (FirstAppBtn) NameInput->SetNavigationRuleExplicit(EUINavigation::Right, FirstAppBtn);
        if (CreateBtn) NameInput->SetNavigationRuleExplicit(EUINavigation::Down, CreateBtn);
    }

    // 3. Bottom panel: Footer buttons
    if (BackBtn)
    {
        if (SupportBtn) BackBtn->SetNavigationRuleExplicit(EUINavigation::Up, SupportBtn);
        if (CreateBtn) BackBtn->SetNavigationRuleExplicit(EUINavigation::Right, CreateBtn);
        BackBtn->SetNavigationRuleBase(EUINavigation::Down, EUINavigationRule::Stop);
        BackBtn->SetNavigationRuleBase(EUINavigation::Left, EUINavigationRule::Stop);
    }
    if (CreateBtn)
    {
        if (BackBtn) CreateBtn->SetNavigationRuleExplicit(EUINavigation::Left, BackBtn);
        if (NameInput) CreateBtn->SetNavigationRuleExplicit(EUINavigation::Up, NameInput);
        CreateBtn->SetNavigationRuleBase(EUINavigation::Down, EUINavigationRule::Stop);
        CreateBtn->SetNavigationRuleBase(EUINavigation::Right, EUINavigationRule::Stop);
    }

    // 4. Right panel: Appearance options
    if (OptionsList)
    {
        const int32 ChildCount = OptionsList->GetChildrenCount();
        UWidget* LastButton = nullptr;
        for (int32 Index = 0; Index < ChildCount; ++Index)
        {
            UWidget* Child = OptionsList->GetChildAt(Index);
            if (UCommonButtonBase* Btn = Cast<UCommonButtonBase>(Child))
            {
                if (NameInput) Btn->SetNavigationRuleExplicit(EUINavigation::Left, NameInput);
                LastButton = Btn;
            }
            else if (UHorizontalBox* Row = Cast<UHorizontalBox>(Child))
            {
                if (Row->GetChildrenCount() > 0)
                {
                    if (UWidget* FirstInRow = Row->GetChildAt(0))
                    {
                        if (NameInput) FirstInRow->SetNavigationRuleExplicit(EUINavigation::Left, NameInput);
                    }
                    LastButton = Row->GetChildAt(Row->GetChildrenCount() - 1);
                }
            }
        }
        if (LastButton && CreateBtn)
        {
            LastButton->SetNavigationRuleExplicit(EUINavigation::Down, CreateBtn);
        }
    }
}

void UCharacterCreationScreen::CyclePanelFocus(bool bForward)
{
    // Panels arranged Left to Right, matching screen layout:
    // 0: Class selection (Left panel: Tank, DPS, Ranged, Support)
    // 1: Callsign input (Center panel: CharacterName)
    // 2: Appearance customizer (Right panel: OptionsList in OptionsScroll)
    // 3: Footer actions (Bottom panel: Create Operative / Back)

    enum ECreationPanel : int32
    {
        Panel_Class = 0,
        Panel_Callsign = 1,
        Panel_Appearance = 2,
        Panel_Footer = 3,
        Panel_MAX = 4
    };

    ECreationPanel CurrentPanel = Panel_Class;
    APlayerController* PC = GetOwningPlayer();

    TArray<UWidget*> ClassButtons = GetClassButtons();
    bool bFoundCurrent = false;

    for (UWidget* Btn : ClassButtons)
    {
        if (Btn && Btn->HasUserFocus(PC))
        {
            CurrentPanel = Panel_Class;
            bFoundCurrent = true;
            break;
        }
    }

    if (!bFoundCurrent)
    {
        if (UWidget* NameInput = Cast<UWidget>(GetWidgetFromName(TEXT("CharacterName"))))
        {
            if (NameInput->HasUserFocus(PC))
            {
                CurrentPanel = Panel_Callsign;
                bFoundCurrent = true;
            }
        }
    }

    if (!bFoundCurrent && OptionsScroll)
    {
        if (OptionsScroll->HasUserFocusedDescendants(PC))
        {
            CurrentPanel = Panel_Appearance;
            bFoundCurrent = true;
        }
    }

    if (!bFoundCurrent)
    {
        UWidget* BackBtn = Cast<UWidget>(GetWidgetFromName(TEXT("BackButton")));
        if ((NativeCreateButton && NativeCreateButton->HasUserFocus(PC)) ||
            (BackBtn && BackBtn->HasUserFocus(PC)))
        {
            CurrentPanel = Panel_Footer;
            bFoundCurrent = true;
        }
    }

    int32 NextPanelIdx = 0;
    if (bFoundCurrent)
    {
        NextPanelIdx = bForward
            ? ((static_cast<int32>(CurrentPanel) + 1) % Panel_MAX)
            : ((static_cast<int32>(CurrentPanel) - 1 + Panel_MAX) % Panel_MAX);
    }
    else
    {
        NextPanelIdx = bForward ? Panel_Callsign : Panel_Footer;
    }

    switch (static_cast<ECreationPanel>(NextPanelIdx))
    {
    case Panel_Class:
        if (UWidget* ClassTarget = GetSelectedClassButton())
        {
            ClassTarget->SetFocus();
        }
        break;

    case Panel_Callsign:
        if (UWidget* NameInput = Cast<UWidget>(GetWidgetFromName(TEXT("CharacterName"))))
        {
            NameInput->SetFocus();
        }
        break;

    case Panel_Appearance:
        if (UWidget* AppTarget = GetActiveAppearanceOption())
        {
            AppTarget->SetFocus();
            if (OptionsScroll)
            {
                OptionsScroll->ScrollWidgetIntoView(AppTarget, true, EDescendantScrollDestination::IntoView);
            }
        }
        break;

    case Panel_Footer:
        if (bForward)
        {
            if (NativeCreateButton && NativeCreateButton->IsVisible() && NativeCreateButton->GetIsEnabled())
            {
                NativeCreateButton->SetFocus();
            }
            else if (UWidget* BackBtn = Cast<UWidget>(GetWidgetFromName(TEXT("BackButton"))))
            {
                BackBtn->SetFocus();
            }
        }
        else
        {
            if (UWidget* BackBtn = Cast<UWidget>(GetWidgetFromName(TEXT("BackButton"))))
            {
                if (BackBtn->IsVisible() && BackBtn->GetIsEnabled())
                {
                    BackBtn->SetFocus();
                }
                else if (NativeCreateButton)
                {
                    NativeCreateButton->SetFocus();
                }
            }
        }
        break;

    default:
        break;
    }
}

FReply UCharacterCreationScreen::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();

    // 1. Shoulder bumpers: Cycle horizontally across screen panels (LB / RB)
    if (Key == EKeys::Gamepad_RightShoulder || Key == EKeys::Gamepad_LeftShoulder)
    {
        CyclePanelFocus(Key == EKeys::Gamepad_RightShoulder);
        return FReply::Handled();
    }

    // 2. Triggers: Fast page scroll for appearance options (LT / RT)
    if (Key == EKeys::Gamepad_RightTrigger || Key == EKeys::Gamepad_RightTriggerAxis)
    {
        if (OptionsScroll)
        {
            OptionsScroll->SetScrollOffset(OptionsScroll->GetScrollOffset() + 250.f);
        }
        return FReply::Handled();
    }
    if (Key == EKeys::Gamepad_LeftTrigger || Key == EKeys::Gamepad_LeftTriggerAxis)
    {
        if (OptionsScroll)
        {
            OptionsScroll->SetScrollOffset(FMath::Max(0.f, OptionsScroll->GetScrollOffset() - 250.f));
        }
        return FReply::Handled();
    }

    // 3. Direct shortcut to Create Operative: Face Button Left (X) or Special Right (Menu/Start)
    if (Key == EKeys::Gamepad_FaceButton_Left || Key == EKeys::Gamepad_Special_Right)
    {
        HandleNativeCreateClicked();
        return FReply::Handled();
    }

    // 4. R3 Click: Reset 3D preview turntable rotation
    if (Key == EKeys::Gamepad_RightThumbstick && PreviewActor)
    {
        PreviewActor->SetActorRotation(FRotator(0.f, 0.f, 0.f));
        return FReply::Handled();
    }

    // 5. Gamepad Accept (A) on Callsign input: Open Virtual Keyboard
    if (Key == EKeys::Gamepad_FaceButton_Bottom)
    {
        APlayerController* PC = GetOwningPlayer();
        UWidget* NameInput = Cast<UWidget>(GetWidgetFromName(TEXT("CharacterName")));
        if (NameInput && NameInput->HasUserFocus(PC))
        {
            if (TerminalInputController)
            {
                TerminalInputController->OpenVirtualKeyboard();
                return FReply::Handled();
            }
        }
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
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

    if (OptionsScroll)
    {
        Button->OnFocusReceived().AddWeakLambda(this, [this, Button]()
        {
            LastFocusedAppearanceWidget = Button;
            if (OptionsScroll)
            {
                OptionsScroll->ScrollWidgetIntoView(Button, true, EDescendantScrollDestination::IntoView);
            }
        });
    }

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
    SetupNavigationRules();
    if (IsActivated()) RequestRefreshFocus();
}

void UCharacterCreationScreen::EnsurePreview()
{
    if (!bDraftReady || !WidgetTree || !GetWorld()) return;

    auto* Center = Cast<UCanvasPanel>(GetWidgetFromName(TEXT("Preview")));
    if (!Center) return;

    if (!PreviewRigActor)
    {
        AOnsetAppearancePreviewRig* Rig = AOnsetAppearancePreviewRig::SpawnPreviewRig(
            GetWorld(),
            FVector(0.f, 0.f, -10000.f),
            FIntPoint(1024, 1280),
            PreviewMaterial,
            AppearanceCatalog
        );

        if (Rig)
        {
            Rig->AttachToCanvas(Center, WidgetTree);
            PreviewRigActor = Rig;
            PreviewCaptureComponent = Rig->GetCaptureComponent();
            PreviewActor = Rig->GetPreviewActor();
            PreviewRenderTarget = Rig->GetRenderTarget();
            PreviewMaterialInstance = Rig->GetMaterialInstance();
            PreviewImage = Rig->GetPreviewImage();
        }
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
    SetFeedback(FText::GetEmpty());

    if (PreviewCaptureComponent)
    {
        PreviewCaptureComponent->ClearShowOnlyComponents();
        PreviewCaptureComponent->ShowOnlyActors.Reset();
        if (PreviewRigActor) PreviewCaptureComponent->ShowOnlyActors.Add(PreviewRigActor);
        PreviewCaptureComponent->ShowOnlyActors.Add(PreviewActor);
        PreviewCaptureComponent->ShowOnlyActorComponents(PreviewActor, true);
    }

    const USkeletalMeshComponent* Body = PreviewActor->Appearance->GetBodyMesh();
    UE_LOG(LogTemp, Display, TEXT("Character creation preview: body=%s mesh=%s"),
        *DraftAppearance.BodyId.ToString(), *GetPathNameSafe(Body ? Body->GetSkeletalMeshAsset() : nullptr));

    if (AOnsetAppearancePreviewRig* Rig = Cast<AOnsetAppearancePreviewRig>(PreviewRigActor))
    {
        UAnimSequence* Anim = DefaultIdleAnimation.IsValid() ? DefaultIdleAnimation.Get() : DefaultIdleAnimation.LoadSynchronous();
        if (!Anim)
        {
            Anim = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/Humanoid/Adult/Anims/Unarmed/MM_Idle_F_Adult.MM_Idle_F_Adult"));
        }
        if (Anim)
        {
            Rig->SetAnimation(Anim, true);
        }
    }
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
    if (auto* NewSlot = Cast<UHorizontalBoxSlot>(NativeCreateButton->Slot))
    {
        if (auto* OldSlot = Cast<UHorizontalBoxSlot>(LegacyButton->Slot))
        {
            NewSlot->SetPadding(OldSlot->GetPadding());
            NewSlot->SetSize(OldSlot->GetSize());
            NewSlot->SetHorizontalAlignment(OldSlot->GetHorizontalAlignment());
            NewSlot->SetVerticalAlignment(OldSlot->GetVerticalAlignment());
        }
    }
    if (auto* Label = Cast<UCommonTextBlock>(NativeCreateButton->GetWidgetFromName(TEXT("ButtonText"))))
        Label->SetText(NSLOCTEXT("OnsetCreation", "CreateOperative", "CREATE OPERATIVE"));
    NativeCreateButton->OnClicked().AddWeakLambda(this, [this]() { HandleNativeCreateClicked(); });
    SetupNavigationRules();
}

void UCharacterCreationScreen::InitializeTerminalInput()
{
    // Clear old Blueprint blink timer if it was set in Blueprint EventGraph
    for (const TCHAR* PropName : {TEXT("BlinkTimer"), TEXT("Blink Timer")})
    {
        if (FProperty* Prop = GetClass()->FindPropertyByName(PropName))
        {
            if (FStructProperty* StructProp = CastField<FStructProperty>(Prop))
            {
                if (StructProp->Struct == TBaseStructure<FTimerHandle>::Get())
                {
                    FTimerHandle* HandlePtr = StructProp->ContainerPtrToValuePtr<FTimerHandle>(this);
                    if (HandlePtr && HandlePtr->IsValid() && GetWorld())
                    {
                        GetWorld()->GetTimerManager().ClearTimer(*HandlePtr);
                        *HandlePtr = FTimerHandle();
                    }
                }
            }
        }
    }

    auto* NameField = Cast<UEditableText>(GetWidgetFromName(TEXT("CharacterName")));
    auto* DefaultBorder = Cast<UBorder>(GetWidgetFromName(TEXT("DefaultNameBorder")));
    auto* DisplayText = Cast<UCommonTextBlock>(GetWidgetFromName(TEXT("text")));
    auto* CursorBorder = Cast<UBorder>(GetWidgetFromName(TEXT("CursorTextBorder")));
    auto* CursorText = Cast<UCommonTextBlock>(GetWidgetFromName(TEXT("text_1")));

    if (NameField && DisplayText)
    {
        if (!TerminalInputController)
        {
            TerminalInputController = NewObject<UMHTerminalInputController>(this);
        }

        const FString InitialPlaceholder = DefaultCharacterName.IsEmpty() ? TEXT("Kael") : DefaultCharacterName;
        TerminalInputController->Initialize(
            this,
            NameField,
            DisplayText,
            CursorText,
            DefaultBorder,
            CursorBorder,
            InitialPlaceholder,
            0.5f,
            16
        );
    }
}

void UCharacterCreationScreen::HandleNativeCreateClicked()
{
    FString ChosenName;
    if (TerminalInputController)
    {
        ChosenName = TerminalInputController->GetFinalText().TrimStartAndEnd();
    }
    else
    {
        auto* NameField = Cast<UEditableText>(GetWidgetFromName(TEXT("CharacterName")));
        if (!NameField) { SetFeedback(NSLOCTEXT("OnsetCreation", "MissingNameField", "Callsign control is unavailable.")); return; }
        ChosenName = NameField->GetText().ToString().TrimStartAndEnd();
        if (ChosenName.IsEmpty())
        {
            ChosenName = DefaultCharacterName.IsEmpty() ? TEXT("Kael") : DefaultCharacterName;
        }
    }

    if (ChosenName.IsEmpty())
    {
        ChosenName = TEXT("Kael");
    }

    EOnsetCharacterClass ChosenClass = EOnsetCharacterClass::Tank;
    bool bFoundClass = false;

    if (UCommonButtonBase* TankBtn = Cast<UCommonButtonBase>(GetWidgetFromName(TEXT("Tank"))))
    {
        if (TankBtn->GetSelected()) { ChosenClass = EOnsetCharacterClass::Tank; bFoundClass = true; }
    }
    if (UCommonButtonBase* DPSBtn = Cast<UCommonButtonBase>(GetWidgetFromName(TEXT("DPS"))))
    {
        if (DPSBtn->GetSelected()) { ChosenClass = EOnsetCharacterClass::DPS; bFoundClass = true; }
    }
    if (UCommonButtonBase* RangedBtn = Cast<UCommonButtonBase>(GetWidgetFromName(TEXT("Ranged"))))
    {
        if (RangedBtn->GetSelected()) { ChosenClass = EOnsetCharacterClass::Ranged; bFoundClass = true; }
    }
    if (UCommonButtonBase* SupportBtn = Cast<UCommonButtonBase>(GetWidgetFromName(TEXT("Support"))))
    {
        if (SupportBtn->GetSelected()) { ChosenClass = EOnsetCharacterClass::Support; bFoundClass = true; }
    }

    if (!bFoundClass)
    {
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
    }

    if (!bFoundClass) { SetFeedback(NSLOCTEXT("OnsetCreation", "MissingClassField", "Choose a class.")); return; }
    CreateCharacter(ChosenName, ChosenClass, 0);
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
