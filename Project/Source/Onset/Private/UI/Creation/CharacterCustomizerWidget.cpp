// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Creation/CharacterCustomizerWidget.h"
#include "UI/Creation/AppearanceOptionEntryWidget.h"
#include "UI/Creation/AppearanceColorSwatchWidget.h"
#include "UI/Creation/AppearanceSliderWidget.h"
#include "UI/Creation/AppearanceColorPickerWidget.h"
#include "OnsetAppearanceCatalog.h"
#include "Player/OnsetCharacterAppearanceComponent.h"
#include "CommonTabListWidgetBase.h"
#include "CommonAnimatedSwitcher.h"
#include "CommonTextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Components/ScrollBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"

static void AddCustomizerHeader(UPanelWidget* Parent, UWidgetTree* Tree, const FText& Text)
{
	if (!Parent) return;

	UCommonTextBlock* Label = nullptr;
	if (Tree)
	{
		Label = Tree->ConstructWidget<UCommonTextBlock>();
	}
	else
	{
		Label = NewObject<UCommonTextBlock>(Parent);
	}

	if (!Label) return;

	static UClass* SubtextStyleClass = LoadClass<UCommonTextStyle>(nullptr, TEXT("/Game/UI/Styles/SubtextStyle.SubtextStyle_C"));
	if (SubtextStyleClass)
	{
		Label->SetStyle(SubtextStyleClass);
	}
	Label->SetText(Text);
	Label->SetAutoWrapText(true);

	Parent->AddChild(Label);
	if (UVerticalBoxSlot* BoxSlot = Cast<UVerticalBoxSlot>(Label->Slot))
	{
		BoxSlot->SetPadding(FMargin(4.f, 10.f, 4.f, 4.f));
	}
}

void UCharacterCustomizerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CategoryTabList && CategorySwitcher)
	{
		if (UCommonAnimatedSwitcher* AnimatedSwitcher = Cast<UCommonAnimatedSwitcher>(CategorySwitcher))
		{
			CategoryTabList->SetLinkedSwitcher(AnimatedSwitcher);
		}
	}

	if (!bIsInitialized)
	{
		if (!Catalog)
		{
			Catalog = UOnsetCharacterAppearanceComponent::LoadDefaultCatalog();
		}

		if (DraftRecipe.BodyId.IsNone())
		{
			FOnsetAppearanceRecipe DefaultPreset;
			if (UOnsetCharacterAppearanceComponent::GetPlayerPreset(0, DefaultPreset))
			{
				DraftRecipe = DefaultPreset;
			}
		}

		if (Catalog)
		{
			InitializeCustomizer(Catalog, DraftRecipe);
		}
	}
}

void UCharacterCustomizerWidget::InitializeCustomizer(UOnsetAppearanceCatalog* InCatalog, const FOnsetAppearanceRecipe& InInitialRecipe)
{
	Catalog = InCatalog;
	DraftRecipe = InInitialRecipe;
	bIsInitialized = true;

	RebuildAllCategories();
}

void UCharacterCustomizerWidget::SetDraftRecipe(const FOnsetAppearanceRecipe& InRecipe)
{
	DraftRecipe = InRecipe;
	RebuildAllCategories();
	OnAppearanceDraftChanged.Broadcast(DraftRecipe);
}

void UCharacterCustomizerWidget::SwitchToCategory(int32 CategoryIndex)
{
	if (CategorySwitcher)
	{
		CategorySwitcher->SetActiveWidgetIndex(CategoryIndex);
	}

	// Categories: 0 = Body, 1 = Face, 2 = Hair, 3 = Outfit, 4 = Color
	const bool bFaceFocus = (CategoryIndex == 1 || CategoryIndex == 2);
	OnCameraFramingRequested.Broadcast(bFaceFocus);
}

void UCharacterCustomizerWidget::CommitDraft(const FOnsetAppearanceRecipe& Candidate)
{
	if (!Catalog) return;

	TArray<FOnsetAppearanceIssue> Issues;
	if (!Catalog->ValidateRecipe(Candidate, EOnsetAppearanceUse::Player, Issues))
	{
		UE_LOG(LogTemp, Warning, TEXT("Character customizer: Rejected invalid appearance candidate: %s"),
			Issues.IsEmpty() ? TEXT("Unknown issue") : *Issues[0].Message);
		return;
	}

	DraftRecipe = Candidate;
	OnAppearanceDraftChanged.Broadcast(DraftRecipe);
}

void UCharacterCustomizerWidget::RebuildAllCategories()
{
	BuildBodyCategory();
	BuildFaceCategory();
	BuildHairCategory();
	BuildOutfitCategory();
	BuildColorCategory();
}

void UCharacterCustomizerWidget::BuildBodyCategory()
{
	if (!BodyOptionsScroll || !Catalog) return;

	BodyOptionsScroll->ClearChildren();

	AddCustomizerHeader(BodyOptionsScroll, WidgetTree, NSLOCTEXT("OnsetCreation", "BodyTypeHeader", "BODY MODEL"));

	if (OptionButtonClass)
	{
		const FName FemaleBody = TEXT("epic.adult.female");
		const FName MaleBody = TEXT("epic.adult.male");

		UAppearanceOptionEntryWidget* FemaleBtn = CreateWidget<UAppearanceOptionEntryWidget>(GetOwningPlayer(), OptionButtonClass);
		if (FemaleBtn)
		{
			FemaleBtn->SetupOption(FemaleBody, NSLOCTEXT("OnsetCreation", "BodyFemale", "Female"), DraftRecipe.BodyId == FemaleBody);
			FemaleBtn->OnOptionSelected.AddDynamic(this, &ThisClass::HandleBodySelected);
			BodyOptionsScroll->AddChild(FemaleBtn);
		}

		UAppearanceOptionEntryWidget* MaleBtn = CreateWidget<UAppearanceOptionEntryWidget>(GetOwningPlayer(), OptionButtonClass);
		if (MaleBtn)
		{
			MaleBtn->SetupOption(MaleBody, NSLOCTEXT("OnsetCreation", "BodyMale", "Male"), DraftRecipe.BodyId == MaleBody);
			MaleBtn->OnOptionSelected.AddDynamic(this, &ThisClass::HandleBodySelected);
			BodyOptionsScroll->AddChild(MaleBtn);
		}
	}

	if (const FOnsetBodyDefinition* BodyDef = Catalog->Bodies.Find(DraftRecipe.BodyId))
	{
		if (BodyDef->SupportedMorphs.Num() > 0 && SliderRowClass)
		{
			AddCustomizerHeader(BodyOptionsScroll, WidgetTree, NSLOCTEXT("OnsetCreation", "BodyShapeHeader", "BODY MORPHS"));

			for (FName MorphId : BodyDef->SupportedMorphs)
			{
				UAppearanceSliderWidget* SliderWidget = CreateWidget<UAppearanceSliderWidget>(GetOwningPlayer(), SliderRowClass);
				if (SliderWidget)
				{
					const float CurrentVal = DraftRecipe.BodyMorphs.FindRef(MorphId);
					SliderWidget->SetupMorph(MorphId, FText::FromName(MorphId), CurrentVal);
					SliderWidget->OnMorphAdjusted.AddDynamic(this, &ThisClass::HandleBodyMorphAdjusted);
					BodyOptionsScroll->AddChild(SliderWidget);
				}
			}
		}
	}
}

void UCharacterCustomizerWidget::BuildFaceCategory()
{
	if (!FaceOptionsScroll || !Catalog) return;

	FaceOptionsScroll->ClearChildren();

	AddCustomizerHeader(FaceOptionsScroll, WidgetTree, NSLOCTEXT("OnsetCreation", "FacePresetHeader", "FACE PRESETS"));

	if (OptionButtonClass)
	{
		TArray<FName> Heads;
		for (const auto& Pair : Catalog->Heads)
		{
			if (Pair.Value.CompatibleBodies.Contains(DraftRecipe.BodyId))
			{
				Heads.Add(Pair.Key);
			}
		}
		Heads.Sort([](FName A, FName B) { return A.ToString() < B.ToString(); });

		for (int32 Index = 0; Index < Heads.Num(); ++Index)
		{
			const FName HeadId = Heads[Index];
			const FText Title = FText::Format(NSLOCTEXT("OnsetCreation", "FaceFormat", "Face {0}"), FText::AsNumber(Index + 1));

			UAppearanceOptionEntryWidget* FaceBtn = CreateWidget<UAppearanceOptionEntryWidget>(GetOwningPlayer(), OptionButtonClass);
			if (FaceBtn)
			{
				FaceBtn->SetupOption(HeadId, Title, DraftRecipe.HeadId == HeadId);
				FaceBtn->OnOptionSelected.AddDynamic(this, &ThisClass::HandleHeadSelected);
				FaceOptionsScroll->AddChild(FaceBtn);
			}
		}
	}

	if (const FOnsetHeadDefinition* HeadDef = Catalog->Heads.Find(DraftRecipe.HeadId))
	{
		if (HeadDef->SupportedMorphs.Num() > 0 && SliderRowClass)
		{
			AddCustomizerHeader(FaceOptionsScroll, WidgetTree, NSLOCTEXT("OnsetCreation", "FaceShapeHeader", "FACIAL MORPHS"));

			for (FName MorphId : HeadDef->SupportedMorphs)
			{
				UAppearanceSliderWidget* SliderWidget = CreateWidget<UAppearanceSliderWidget>(GetOwningPlayer(), SliderRowClass);
				if (SliderWidget)
				{
					const float CurrentVal = DraftRecipe.FaceMorphs.FindRef(MorphId);
					SliderWidget->SetupMorph(MorphId, FText::FromName(MorphId), CurrentVal);
					SliderWidget->OnMorphAdjusted.AddDynamic(this, &ThisClass::HandleFaceMorphAdjusted);
					FaceOptionsScroll->AddChild(SliderWidget);
				}
			}
		}
	}
}

void UCharacterCustomizerWidget::BuildHairCategory()
{
	if (!HairOptionsScroll || !Catalog || !OptionButtonClass) return;

	HairOptionsScroll->ClearChildren();
	HairColorPicker = nullptr;

	AddCustomizerHeader(HairOptionsScroll, WidgetTree, NSLOCTEXT("OnsetCreation", "HairHeader", "HAIRSTYLES"));

	TArray<FName> HairParts;
	for (const auto& Pair : Catalog->Parts)
	{
		if (Pair.Value.CompatibleBodies.Contains(DraftRecipe.BodyId))
		{
			if (Pair.Value.bHair || Pair.Value.Slot == TEXT("Hair") || Pair.Value.Slot.ToString().StartsWith(TEXT("Hair")))
			{
				HairParts.Add(Pair.Key);
			}
		}
	}
	HairParts.Sort([](FName A, FName B) { return A.ToString() < B.ToString(); });

	const FOnsetPartSelection* CurrentHair = DraftRecipe.Parts.FindByPredicate([&](const FOnsetPartSelection& P)
	{
		const FOnsetPartDefinition* Def = Catalog->Parts.Find(P.PartId);
		return Def && (Def->bHair || Def->Slot == TEXT("Hair") || Def->Slot.ToString().StartsWith(TEXT("Hair")));
	});

	UAppearanceOptionEntryWidget* NoneBtn = CreateWidget<UAppearanceOptionEntryWidget>(GetOwningPlayer(), OptionButtonClass);
	if (NoneBtn)
	{
		NoneBtn->SetupOption(TEXT("None.Hair"), NSLOCTEXT("OnsetCreation", "HairNone", "None (Bald)"), CurrentHair == nullptr);
		NoneBtn->OnOptionSelected.AddDynamic(this, &ThisClass::HandlePartOptionSelected);
		HairOptionsScroll->AddChild(NoneBtn);
	}

	for (int32 Index = 0; Index < HairParts.Num(); ++Index)
	{
		const FName PartId = HairParts[Index];
		const FText Title = FText::Format(NSLOCTEXT("OnsetCreation", "HairStyleFormat", "Style {0}"), FText::AsNumber(Index + 1));

		UAppearanceOptionEntryWidget* HairBtn = CreateWidget<UAppearanceOptionEntryWidget>(GetOwningPlayer(), OptionButtonClass);
		if (HairBtn)
		{
			HairBtn->SetupOption(PartId, Title, CurrentHair && CurrentHair->PartId == PartId);
			HairBtn->OnOptionSelected.AddDynamic(this, &ThisClass::HandlePartOptionSelected);
			HairOptionsScroll->AddChild(HairBtn);
		}
	}

	// Hair Dye / Color
	AddCustomizerHeader(HairOptionsScroll, WidgetTree, NSLOCTEXT("OnsetCreation", "HairColorHeader", "HAIR COLOR / DYE"));

	FLinearColor CurrentHairColor = FLinearColor(0.04f, 0.04f, 0.04f, 1.0f);
	if (CurrentHair && CurrentHair->bOverrideColors)
	{
		CurrentHairColor = CurrentHair->ColorA;
	}

	if (ColorPickerClass)
	{
		HairColorPicker = CreateWidget<UAppearanceColorPickerWidget>(GetOwningPlayer(), ColorPickerClass);
		if (HairColorPicker)
		{
			HairColorPicker->SetupPicker(CurrentHairColor, NSLOCTEXT("OnsetCreation", "HairPickerLabel", "Hair Dye"));
			HairColorPicker->OnColorChanged.AddDynamic(this, &ThisClass::HandleHairColorSelected);
			HairOptionsScroll->AddChild(HairColorPicker);
		}
	}

	if (ColorSwatchClass)
	{
		const FLinearColor HairPresets[] = {
			FLinearColor(0.04f, 0.04f, 0.04f), // Black
			FLinearColor(0.20f, 0.12f, 0.06f), // Dark Brown
			FLinearColor(0.48f, 0.32f, 0.18f), // Chestnut Brown
			FLinearColor(0.85f, 0.72f, 0.45f), // Golden Blonde
			FLinearColor(0.60f, 0.15f, 0.08f), // Auburn / Red
			FLinearColor(0.72f, 0.72f, 0.75f), // Silver / Grey
			FLinearColor(0.12f, 0.40f, 0.85f), // Neon Blue
			FLinearColor(0.82f, 0.15f, 0.52f)  // Magenta
		};

		UHorizontalBox* HairSwatchBox = WidgetTree ? WidgetTree->ConstructWidget<UHorizontalBox>() : NewObject<UHorizontalBox>(HairOptionsScroll);
		HairOptionsScroll->AddChild(HairSwatchBox);

		for (int32 Index = 0; Index < UE_ARRAY_COUNT(HairPresets); ++Index)
		{
			UAppearanceColorSwatchWidget* Swatch = CreateWidget<UAppearanceColorSwatchWidget>(GetOwningPlayer(), ColorSwatchClass);
			if (Swatch)
			{
				Swatch->SetupSwatch(HairPresets[Index], CurrentHairColor.Equals(HairPresets[Index], 0.02f));
				Swatch->OnColorSelected.AddDynamic(this, &ThisClass::HandleHairColorSelected);
				HairSwatchBox->AddChild(Swatch);
			}
		}
	}
}

void UCharacterCustomizerWidget::BuildOutfitCategory()
{
	if (!OutfitOptionsScroll || !Catalog || !OptionButtonClass) return;

	OutfitOptionsScroll->ClearChildren();

	TMap<FName, TArray<FName>> BySlot;
	for (const auto& Pair : Catalog->Parts)
	{
		if (Pair.Value.CompatibleBodies.Contains(DraftRecipe.BodyId))
		{
			if (!Pair.Value.bHair && Pair.Value.Slot != TEXT("Hair") && !Pair.Value.Slot.ToString().StartsWith(TEXT("Hair")))
			{
				BySlot.FindOrAdd(Pair.Value.Slot).Add(Pair.Key);
			}
		}
	}

	TArray<FName> Slots;
	BySlot.GetKeys(Slots);
	Slots.Sort([](FName A, FName B) { return A.ToString() < B.ToString(); });

	for (FName SlotId : Slots)
	{
		AddCustomizerHeader(OutfitOptionsScroll, WidgetTree, FText::FromName(SlotId));

		auto& PartList = BySlot.FindChecked(SlotId);
		PartList.Sort([](FName A, FName B) { return A.ToString() < B.ToString(); });

		const FOnsetPartSelection* CurrentPart = DraftRecipe.Parts.FindByPredicate([&](const FOnsetPartSelection& P)
		{
			const FOnsetPartDefinition* Def = Catalog->Parts.Find(P.PartId);
			return Def && Def->Slot == SlotId;
		});

		if (SlotId == TEXT("HeadTop") || SlotId == TEXT("Gloves") || SlotId == TEXT("Accessory"))
		{
			UAppearanceOptionEntryWidget* NoneBtn = CreateWidget<UAppearanceOptionEntryWidget>(GetOwningPlayer(), OptionButtonClass);
			if (NoneBtn)
			{
				NoneBtn->SetupOption(FName(*FString::Printf(TEXT("None.%s"), *SlotId.ToString())), NSLOCTEXT("OnsetCreation", "PartNone", "None"), CurrentPart == nullptr);
				NoneBtn->OnOptionSelected.AddDynamic(this, &ThisClass::HandlePartOptionSelected);
				OutfitOptionsScroll->AddChild(NoneBtn);
			}
		}

		for (int32 Index = 0; Index < PartList.Num(); ++Index)
		{
			const FName PartId = PartList[Index];
			const FText Title = FText::Format(NSLOCTEXT("OnsetCreation", "OutfitStyleFormat", "Style {0}"), FText::AsNumber(Index + 1));

			UAppearanceOptionEntryWidget* PartBtn = CreateWidget<UAppearanceOptionEntryWidget>(GetOwningPlayer(), OptionButtonClass);
			if (PartBtn)
			{
				PartBtn->SetupOption(PartId, Title, CurrentPart && CurrentPart->PartId == PartId);
				PartBtn->OnOptionSelected.AddDynamic(this, &ThisClass::HandlePartOptionSelected);
				OutfitOptionsScroll->AddChild(PartBtn);
			}
		}
	}
}

void UCharacterCustomizerWidget::BuildColorCategory()
{
	if (!ColorOptionsScroll) return;

	ColorOptionsScroll->ClearChildren();
	SkinColorPicker = nullptr;
	EyeColorPicker = nullptr;
	OutfitPrimaryColorPicker = nullptr;
	OutfitSecondaryColorPicker = nullptr;
	OutfitAccentColorPicker = nullptr;

	// 1. Skin Tone
	AddCustomizerHeader(ColorOptionsScroll, WidgetTree, NSLOCTEXT("OnsetCreation", "SkinToneHeader", "SKIN TONE"));
	if (ColorPickerClass)
	{
		SkinColorPicker = CreateWidget<UAppearanceColorPickerWidget>(GetOwningPlayer(), ColorPickerClass);
		if (SkinColorPicker)
		{
			SkinColorPicker->SetupPicker(DraftRecipe.SkinColor, NSLOCTEXT("OnsetCreation", "SkinColorLabel", "Custom Skin Color"));
			SkinColorPicker->OnColorChanged.AddDynamic(this, &ThisClass::HandleSkinColorSelected);
			ColorOptionsScroll->AddChild(SkinColorPicker);
		}
	}

	if (ColorSwatchClass)
	{
		UHorizontalBox* SkinBox = WidgetTree ? WidgetTree->ConstructWidget<UHorizontalBox>() : NewObject<UHorizontalBox>(ColorOptionsScroll);
		ColorOptionsScroll->AddChild(SkinBox);

		const FLinearColor SkinTones[] = {
			{0.88f, 0.69f, 0.52f},
			{0.75f, 0.55f, 0.38f},
			{0.57f, 0.38f, 0.25f},
			{0.35f, 0.20f, 0.13f}
		};

		for (int32 Index = 0; Index < UE_ARRAY_COUNT(SkinTones); ++Index)
		{
			UAppearanceColorSwatchWidget* Swatch = CreateWidget<UAppearanceColorSwatchWidget>(GetOwningPlayer(), ColorSwatchClass);
			if (Swatch)
			{
				Swatch->SetupSwatch(SkinTones[Index], DraftRecipe.SkinColor.Equals(SkinTones[Index], 0.02f));
				Swatch->OnColorSelected.AddDynamic(this, &ThisClass::HandleSkinColorSelected);
				SkinBox->AddChild(Swatch);
			}
		}
	}

	// 2. Eye Color
	AddCustomizerHeader(ColorOptionsScroll, WidgetTree, NSLOCTEXT("OnsetCreation", "EyeColorHeader", "EYE COLOR"));
	if (ColorPickerClass)
	{
		EyeColorPicker = CreateWidget<UAppearanceColorPickerWidget>(GetOwningPlayer(), ColorPickerClass);
		if (EyeColorPicker)
		{
			EyeColorPicker->SetupPicker(DraftRecipe.EyeColor, NSLOCTEXT("OnsetCreation", "EyeColorLabel", "Custom Eye Color"));
			EyeColorPicker->OnColorChanged.AddDynamic(this, &ThisClass::HandleEyeColorSelected);
			ColorOptionsScroll->AddChild(EyeColorPicker);
		}
	}

	if (ColorSwatchClass)
	{
		UHorizontalBox* EyeBox = WidgetTree ? WidgetTree->ConstructWidget<UHorizontalBox>() : NewObject<UHorizontalBox>(ColorOptionsScroll);
		ColorOptionsScroll->AddChild(EyeBox);

		const FLinearColor EyeColors[] = {
			{0.12f, 0.30f, 0.50f},
			{0.28f, 0.20f, 0.12f},
			{0.14f, 0.35f, 0.20f},
			{0.45f, 0.40f, 0.32f}
		};

		for (int32 Index = 0; Index < UE_ARRAY_COUNT(EyeColors); ++Index)
		{
			UAppearanceColorSwatchWidget* Swatch = CreateWidget<UAppearanceColorSwatchWidget>(GetOwningPlayer(), ColorSwatchClass);
			if (Swatch)
			{
				Swatch->SetupSwatch(EyeColors[Index], DraftRecipe.EyeColor.Equals(EyeColors[Index], 0.02f));
				Swatch->OnColorSelected.AddDynamic(this, &ThisClass::HandleEyeColorSelected);
				EyeBox->AddChild(Swatch);
			}
		}
	}

	// 3. Outfit Colors (Primary, Secondary, Accent)
	const FLinearColor OutfitPalettes[] = {
		FLinearColor(0.13f, 0.24f, 0.33f),
		FLinearColor(0.38f, 0.12f, 0.14f),
		FLinearColor(0.22f, 0.29f, 0.17f),
		FLinearColor(0.55f, 0.46f, 0.32f),
		FLinearColor(0.05f, 0.05f, 0.05f),
		FLinearColor(0.85f, 0.85f, 0.85f)
	};

	const FText ChannelLabels[] = {
		NSLOCTEXT("OnsetCreation", "ChannelPrimary", "OUTFIT PRIMARY"),
		NSLOCTEXT("OnsetCreation", "ChannelSecondary", "OUTFIT SECONDARY"),
		NSLOCTEXT("OnsetCreation", "ChannelAccent", "OUTFIT ACCENT")
	};

	for (int32 Channel = 0; Channel < 3; ++Channel)
	{
		AddCustomizerHeader(ColorOptionsScroll, WidgetTree, ChannelLabels[Channel]);

		FLinearColor CurrentColor = (Channel == 0) ? FLinearColor(0.13f, 0.24f, 0.33f) :
									(Channel == 1) ? FLinearColor(0.38f, 0.12f, 0.14f) :
													 FLinearColor(0.22f, 0.29f, 0.17f);
		if (Catalog)
		{
			for (const FOnsetPartSelection& Part : DraftRecipe.Parts)
			{
				const FOnsetPartDefinition* Def = Catalog->Parts.Find(Part.PartId);
				if (Def && !Def->bHair && Part.bOverrideColors)
				{
					CurrentColor = (Channel == 0) ? Part.ColorA :
								   (Channel == 1) ? Part.ColorB :
													Part.ColorC;
					break;
				}
			}
		}

		if (ColorPickerClass)
		{
			UAppearanceColorPickerWidget* Picker = CreateWidget<UAppearanceColorPickerWidget>(GetOwningPlayer(), ColorPickerClass);
			if (Picker)
			{
				Picker->SetupPicker(CurrentColor, ChannelLabels[Channel]);
				if (Channel == 0)
				{
					OutfitPrimaryColorPicker = Picker;
					Picker->OnColorChanged.AddDynamic(this, &ThisClass::HandleOutfitPrimarySelected);
				}
				else if (Channel == 1)
				{
					OutfitSecondaryColorPicker = Picker;
					Picker->OnColorChanged.AddDynamic(this, &ThisClass::HandleOutfitSecondarySelected);
				}
				else
				{
					OutfitAccentColorPicker = Picker;
					Picker->OnColorChanged.AddDynamic(this, &ThisClass::HandleOutfitAccentSelected);
				}
				ColorOptionsScroll->AddChild(Picker);
			}
		}

		if (ColorSwatchClass)
		{
			UHorizontalBox* ChannelBox = WidgetTree ? WidgetTree->ConstructWidget<UHorizontalBox>() : NewObject<UHorizontalBox>(ColorOptionsScroll);
			ColorOptionsScroll->AddChild(ChannelBox);

			for (int32 Index = 0; Index < UE_ARRAY_COUNT(OutfitPalettes); ++Index)
			{
				UAppearanceColorSwatchWidget* Swatch = CreateWidget<UAppearanceColorSwatchWidget>(GetOwningPlayer(), ColorSwatchClass);
				if (Swatch)
				{
					Swatch->SetupSwatch(OutfitPalettes[Index], CurrentColor.Equals(OutfitPalettes[Index], 0.02f));
					if (Channel == 0)
					{
						Swatch->OnColorSelected.AddDynamic(this, &ThisClass::HandleOutfitPrimarySelected);
					}
					else if (Channel == 1)
					{
						Swatch->OnColorSelected.AddDynamic(this, &ThisClass::HandleOutfitSecondarySelected);
					}
					else
					{
						Swatch->OnColorSelected.AddDynamic(this, &ThisClass::HandleOutfitAccentSelected);
					}
					ChannelBox->AddChild(Swatch);
				}
			}
		}
	}
}

void UCharacterCustomizerWidget::HandleBodySelected(FName BodyId)
{
	if (DraftRecipe.BodyId == BodyId) return;

	const int32 PresetIndex = (BodyId == TEXT("epic.adult.male")) ? 1 : 0;
	FOnsetAppearanceRecipe Candidate;
	if (UOnsetCharacterAppearanceComponent::GetPlayerPreset(PresetIndex, Candidate))
	{
		Candidate.SkinColor = DraftRecipe.SkinColor;
		Candidate.EyeColor = DraftRecipe.EyeColor;
		CommitDraft(Candidate);
		RebuildAllCategories();
		OnCameraFramingRequested.Broadcast(false);
	}
}

void UCharacterCustomizerWidget::HandleHeadSelected(FName HeadId)
{
	if (!Catalog || DraftRecipe.HeadId == HeadId) return;

	const FOnsetHeadDefinition* HeadDef = Catalog->Heads.Find(HeadId);
	if (!HeadDef || !HeadDef->CompatibleBodies.Contains(DraftRecipe.BodyId)) return;

	FOnsetAppearanceRecipe Candidate = DraftRecipe;
	Candidate.HeadId = HeadId;
	Candidate.FaceMorphs.Reset();

	CommitDraft(Candidate);
	BuildFaceCategory();
	OnCameraFramingRequested.Broadcast(true);
}

void UCharacterCustomizerWidget::HandlePartSelected(FName SlotId, FName PartId)
{
	if (!Catalog) return;

	if (!PartId.IsNone())
	{
		const FOnsetPartDefinition* Def = Catalog->Parts.Find(PartId);
		if (!Def || !Def->CompatibleBodies.Contains(DraftRecipe.BodyId)) return;
	}

	FOnsetAppearanceRecipe Candidate = DraftRecipe;
	FOnsetPartSelection Previous;
	bool bHadPrevious = false;

	if (const FOnsetPartSelection* Existing = Candidate.Parts.FindByPredicate([&](const FOnsetPartSelection& P)
	{
		const FOnsetPartDefinition* Def = Catalog->Parts.Find(P.PartId);
		return Def && Def->Slot == SlotId;
	}))
	{
		Previous = *Existing;
		bHadPrevious = true;
	}

	Candidate.Parts.RemoveAll([&](const FOnsetPartSelection& P)
	{
		const FOnsetPartDefinition* Def = Catalog->Parts.Find(P.PartId);
		return Def && Def->Slot == SlotId;
	});

	if (!PartId.IsNone())
	{
		FOnsetPartSelection Selection = bHadPrevious ? Previous : FOnsetPartSelection();
		Selection.PartId = PartId;
		Candidate.Parts.Add(Selection);
	}

	CommitDraft(Candidate);

	const bool bIsHeadPart = (SlotId == TEXT("Hair") || SlotId.ToString().StartsWith(TEXT("Hair")) || SlotId == TEXT("HeadTop"));
	OnCameraFramingRequested.Broadcast(bIsHeadPart);

	if (SlotId == TEXT("Hair") || SlotId.ToString().StartsWith(TEXT("Hair")))
	{
		BuildHairCategory();
	}
	else
	{
		BuildOutfitCategory();
	}
}

void UCharacterCustomizerWidget::HandleBodyMorphAdjusted(FName MorphId, float NewValue)
{
	if (!Catalog) return;

	FOnsetAppearanceRecipe Candidate = DraftRecipe;
	if (NewValue <= KINDA_SMALL_NUMBER)
	{
		Candidate.BodyMorphs.Remove(MorphId);
	}
	else
	{
		Candidate.BodyMorphs.Add(MorphId, FMath::Clamp(NewValue, 0.0f, 1.0f));
	}

	CommitDraft(Candidate);
	OnCameraFramingRequested.Broadcast(false);
}

void UCharacterCustomizerWidget::HandleFaceMorphAdjusted(FName MorphId, float NewValue)
{
	if (!Catalog) return;

	FOnsetAppearanceRecipe Candidate = DraftRecipe;
	if (NewValue <= KINDA_SMALL_NUMBER)
	{
		Candidate.FaceMorphs.Remove(MorphId);
	}
	else
	{
		Candidate.FaceMorphs.Add(MorphId, FMath::Clamp(NewValue, 0.0f, 1.0f));
	}

	CommitDraft(Candidate);
	OnCameraFramingRequested.Broadcast(true);
}

void UCharacterCustomizerWidget::HandleSkinColorSelected(const FLinearColor& Color)
{
	FOnsetAppearanceRecipe Candidate = DraftRecipe;
	Candidate.SkinColor = Color;
	CommitDraft(Candidate);
	if (SkinColorPicker && SkinColorPicker->GetColor() != Color)
	{
		SkinColorPicker->SetColor(Color);
	}
	OnCameraFramingRequested.Broadcast(false);
}

void UCharacterCustomizerWidget::HandleEyeColorSelected(const FLinearColor& Color)
{
	FOnsetAppearanceRecipe Candidate = DraftRecipe;
	Candidate.EyeColor = Color;
	CommitDraft(Candidate);
	if (EyeColorPicker && EyeColorPicker->GetColor() != Color)
	{
		EyeColorPicker->SetColor(Color);
	}
	OnCameraFramingRequested.Broadcast(true);
}

void UCharacterCustomizerWidget::HandleOutfitColorSelected(int32 Channel, const FLinearColor& Color)
{
	if (!Catalog || Channel < 0 || Channel > 2) return;

	FOnsetAppearanceRecipe Candidate = DraftRecipe;
	for (FOnsetPartSelection& Part : Candidate.Parts)
	{
		const FOnsetPartDefinition* Def = Catalog->Parts.Find(Part.PartId);
		if (!Def || Def->bHair || Def->Slot == TEXT("Hair") || Def->Slot.ToString().StartsWith(TEXT("Hair"))) continue;

		Part.bOverrideColors = true;
		if (Channel == 0) Part.ColorA = Color;
		else if (Channel == 1) Part.ColorB = Color;
		else Part.ColorC = Color;
	}

	CommitDraft(Candidate);
	OnCameraFramingRequested.Broadcast(false);
}

void UCharacterCustomizerWidget::HandlePartOptionSelected(FName OptionId)
{
	const FString OptStr = OptionId.ToString();
	if (OptStr.StartsWith(TEXT("None.")))
	{
		const FName SlotId = FName(*OptStr.Mid(5));
		HandlePartSelected(SlotId, NAME_None);
		return;
	}

	if (Catalog)
	{
		if (const FOnsetPartDefinition* Def = Catalog->Parts.Find(OptionId))
		{
			HandlePartSelected(Def->Slot, OptionId);
		}
	}
}

void UCharacterCustomizerWidget::HandleOutfitPrimarySelected(const FLinearColor& Color)
{
	HandleOutfitColorSelected(0, Color);
	if (OutfitPrimaryColorPicker && OutfitPrimaryColorPicker->GetColor() != Color)
	{
		OutfitPrimaryColorPicker->SetColor(Color);
	}
}

void UCharacterCustomizerWidget::HandleOutfitSecondarySelected(const FLinearColor& Color)
{
	HandleOutfitColorSelected(1, Color);
	if (OutfitSecondaryColorPicker && OutfitSecondaryColorPicker->GetColor() != Color)
	{
		OutfitSecondaryColorPicker->SetColor(Color);
	}
}

void UCharacterCustomizerWidget::HandleOutfitAccentSelected(const FLinearColor& Color)
{
	HandleOutfitColorSelected(2, Color);
	if (OutfitAccentColorPicker && OutfitAccentColorPicker->GetColor() != Color)
	{
		OutfitAccentColorPicker->SetColor(Color);
	}
}

void UCharacterCustomizerWidget::HandleHairColorSelected(const FLinearColor& Color)
{
	if (!Catalog) return;

	FOnsetAppearanceRecipe Candidate = DraftRecipe;
	bool bModified = false;
	for (FOnsetPartSelection& Part : Candidate.Parts)
	{
		const FOnsetPartDefinition* Def = Catalog->Parts.Find(Part.PartId);
		if (!Def) continue;
		const bool bIsHair = Def->bHair || Def->Slot == TEXT("Hair") || Def->Slot.ToString().StartsWith(TEXT("Hair"));
		if (!bIsHair) continue;

		Part.bOverrideColors = true;
		Part.ColorA = Color;
		bModified = true;
	}

	if (bModified)
	{
		CommitDraft(Candidate);
	}

	if (HairColorPicker && HairColorPicker->GetColor() != Color)
	{
		HairColorPicker->SetColor(Color);
	}
	OnCameraFramingRequested.Broadcast(true);
}
