#include "OnsetAppearanceCatalog.h"

namespace
{
bool ValidColor(const FLinearColor& C)
{
    return FMath::IsFinite(C.R) && FMath::IsFinite(C.G) && FMath::IsFinite(C.B) && FMath::IsFinite(C.A)
        && C.R >= 0.f && C.R <= 1.f && C.G >= 0.f && C.G <= 1.f && C.B >= 0.f && C.B <= 1.f && C.A >= 0.f && C.A <= 1.f;
}
}

bool UOnsetAppearanceCatalog::ValidateRecipe(const FOnsetAppearanceRecipe& R, EOnsetAppearanceUse Use, TArray<FOnsetAppearanceIssue>& Issues) const
{
    Issues.Reset();
    auto Fail = [&Issues](FName Code, const FString& Field, const FString& Message)
    { FOnsetAppearanceIssue& I = Issues.AddDefaulted_GetRef(); I.Code = Code; I.Field = Field; I.Message = Message; };
    if (R.SchemaVersion != 1) Fail(TEXT("UnsupportedVersion"), TEXT("SchemaVersion"), TEXT("Unsupported appearance schema."));
    const FOnsetBodyDefinition* B = Bodies.Find(R.BodyId);
    if (!B || B->Mesh.IsNull()) Fail(TEXT("UnknownBody"), TEXT("BodyId"), TEXT("Body definition is missing."));
    else if (Use == EOnsetAppearanceUse::Player && !B->bPlayerSelectable)
        Fail(TEXT("AuthoredOnlyBody"), TEXT("BodyId"), TEXT("This body is available only to authored characters."));
    const FOnsetHeadDefinition* H = Heads.Find(R.HeadId);
    if (!H || H->Mesh.IsNull()) Fail(TEXT("UnknownHead"), TEXT("HeadId"), TEXT("Head definition is missing."));
    else if (!H->CompatibleBodies.Contains(R.BodyId)) Fail(TEXT("IncompatibleHead"), TEXT("HeadId"), TEXT("Head does not fit this body."));
    auto CheckMorphs = [&Fail](const TMap<FName, float>& Values, const TArray<FName>& Supported, const FString& Field)
    {
        if (Values.Num() > 64) Fail(TEXT("TooManyMorphs"), Field, TEXT("Too many morph parameters."));
        for (const auto& P : Values)
        {
            if (!Supported.Contains(P.Key)) Fail(TEXT("UnsupportedMorph"), Field + TEXT(".") + P.Key.ToString(), TEXT("Morph is not supported by this definition."));
            if (!FMath::IsFinite(P.Value) || P.Value < 0.f || P.Value > 1.f)
                Fail(TEXT("InvalidMorphValue"), Field + TEXT(".") + P.Key.ToString(), TEXT("Morph must be finite and between zero and one."));
        }
    };
    if (B) CheckMorphs(R.BodyMorphs, B->SupportedMorphs, TEXT("BodyMorphs"));
    if (H) CheckMorphs(R.FaceMorphs, H->SupportedMorphs, TEXT("FaceMorphs"));
    if (!ValidColor(R.SkinColor) || !ValidColor(R.EyeColor)) Fail(TEXT("InvalidColor"), TEXT("Colors"), TEXT("Colors must have finite channels between zero and one."));
    if (R.Parts.Num() > 24) Fail(TEXT("TooManyParts"), TEXT("Parts"), TEXT("At most 24 parts are allowed."));
    TSet<FName> Slots;
    for (int32 Index = 0; Index < R.Parts.Num(); ++Index)
    {
        const FOnsetPartSelection& S = R.Parts[Index];
        const FString Field = FString::Printf(TEXT("Parts[%d]"), Index);
        const FOnsetPartDefinition* P = Parts.Find(S.PartId);
        if (!P || P->Mesh.IsNull()) { Fail(TEXT("UnknownPart"), Field, TEXT("Part definition is missing.")); continue; }
        if (!P->CompatibleBodies.Contains(R.BodyId)) Fail(TEXT("IncompatiblePart"), Field, TEXT("Part does not fit this body."));
        if (Slots.Contains(P->Slot)) Fail(TEXT("DuplicateSlot"), Field, TEXT("Only one selection per slot is allowed."));
        Slots.Add(P->Slot);
        if (!ValidColor(S.ColorA) || !ValidColor(S.ColorB) || !ValidColor(S.ColorC)) Fail(TEXT("InvalidColor"), Field, TEXT("Part colors are invalid."));
    }
    return Issues.IsEmpty();
}

void UOnsetAppearanceCatalog::GetRecipeAssets(const FOnsetAppearanceRecipe& R, TArray<FSoftObjectPath>& Assets) const
{
    Assets.Reset();
    auto Add = [&Assets](const FSoftObjectPath& Path) { if (!Path.IsNull()) Assets.AddUnique(Path); };
    if (const auto* B = Bodies.Find(R.BodyId)) Add(B->Mesh.ToSoftObjectPath());
    if (const auto* H = Heads.Find(R.HeadId)) Add(H->Mesh.ToSoftObjectPath());
    Add(BlankBodyHideTexture.ToSoftObjectPath());
    for (const auto& S : R.Parts) if (const auto* P = Parts.Find(S.PartId))
    { Add(P->Mesh.ToSoftObjectPath()); Add(P->BodyHideTexture.ToSoftObjectPath()); }
}
