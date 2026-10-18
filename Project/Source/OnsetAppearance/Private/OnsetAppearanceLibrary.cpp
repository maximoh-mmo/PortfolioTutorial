#include "OnsetAppearanceLibrary.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool UOnsetAppearanceLibrary::EncodeRecipe(const FOnsetAppearanceRecipe& Recipe, FString& Json)
{ return FJsonObjectConverter::UStructToJsonObjectString(Recipe, Json); }

bool UOnsetAppearanceLibrary::DecodeAndValidateRecipe(const FString& Json, const UOnsetAppearanceCatalog* Catalog,
    EOnsetAppearanceUse Use, FOnsetAppearanceRecipe& Recipe, TArray<FOnsetAppearanceIssue>& Issues)
{
    Issues.Reset();
    auto Fail = [&Issues](FName Code, const FString& Message)
    { auto& I = Issues.AddDefaulted_GetRef(); I.Code = Code; I.Field = TEXT("AppearanceJSON"); I.Message = Message; return false; };
    if (!Catalog) return Fail(TEXT("MissingCatalog"), TEXT("Appearance catalog is unavailable."));
    if (Json.Len() > 65536) return Fail(TEXT("PayloadTooLarge"), TEXT("Appearance payload exceeds 64 KiB."));
    TSharedPtr<FJsonObject> Object;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Object) || !Object.IsValid())
        return Fail(TEXT("InvalidJSON"), TEXT("Appearance must be a JSON object."));
    double Version = 0; FString Body, Head;
    if (!Object->TryGetNumberField(TEXT("schemaVersion"), Version) || Version != 1.0
        || !Object->TryGetStringField(TEXT("bodyId"), Body) || Body.IsEmpty()
        || !Object->TryGetStringField(TEXT("headId"), Head) || Head.IsEmpty())
        return Fail(TEXT("InvalidSchema"), TEXT("Schema version, body ID and head ID are required."));
    // Bound all client-provided FName strings before the converter creates names.
    if (Body.Len()>128 || Head.Len()>128) return Fail(TEXT("InvalidIdentifier"),TEXT("Appearance identifiers are too long."));
    for (const TCHAR* Field : {TEXT("BodyId"), TEXT("HeadId")})
    {
        FString Alias;
        if (Object->TryGetStringField(Field, Alias) && Alias.Len() > 128)
            return Fail(TEXT("InvalidIdentifier"), TEXT("Appearance identifiers are too long."));
    }
    for (const TCHAR* Field : {TEXT("bodyMorphs"),TEXT("BodyMorphs"),TEXT("faceMorphs"),TEXT("FaceMorphs")})
    {
        const TSharedPtr<FJsonObject>* Map=nullptr;
        if (Object->TryGetObjectField(Field,Map) && Map && Map->IsValid())
        {
            if ((*Map)->Values.Num()>64) return Fail(TEXT("TooManyMorphs"),TEXT("Too many morph parameters."));
            for (const auto& Pair : (*Map)->Values) if (Pair.Key.Len()>128) return Fail(TEXT("InvalidIdentifier"),TEXT("Morph identifiers are too long."));
        }
    }
    for (const TCHAR* Field : {TEXT("parts"),TEXT("Parts")})
    {
        const TArray<TSharedPtr<FJsonValue>>* Parts=nullptr;
        if (Object->TryGetArrayField(Field,Parts) && Parts)
        {
            if (Parts->Num()>24) return Fail(TEXT("TooManyParts"),TEXT("Too many clothing selections."));
            for (const auto& Value : *Parts)
            {
                const TSharedPtr<FJsonObject>* Part=nullptr;
                if (Value->TryGetObject(Part) && Part && Part->IsValid())
                    for (const TCHAR* Key : {TEXT("partId"),TEXT("PartId")}) { FString Id; if ((*Part)->TryGetStringField(Key,Id) && Id.Len()>128) return Fail(TEXT("InvalidIdentifier"),TEXT("Part identifiers are too long.")); }
            }
        }
    }
    FOnsetAppearanceRecipe Candidate;
    if (!FJsonObjectConverter::JsonObjectToUStruct(Object.ToSharedRef(), &Candidate, 0, 0))
        return Fail(TEXT("InvalidSchema"), TEXT("Appearance fields have invalid types."));
    if (!Catalog->ValidateRecipe(Candidate, Use, Issues)) return false;
    Recipe = MoveTemp(Candidate);
    return true;
}
