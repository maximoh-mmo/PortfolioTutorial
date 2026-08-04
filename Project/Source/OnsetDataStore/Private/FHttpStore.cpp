#include "FHttpStore.h"
#include "OnsetDataStoreModule.h"

#ifndef ONSETDATASTORE_CLIENT_ONLY

#include "HttpManager.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/Base64.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/DateTime.h"
#include "SHA256.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

DEFINE_LOG_CATEGORY_STATIC(LogHttpStore, Log, All);

FHttpStore::~FHttpStore()
{
}

bool FHttpStore::Initialize(const FString& ConnectionString)
{
	BaseURL = ConnectionString;
	GConfig->GetString(TEXT("Onset.DataStore"), TEXT("APIKey"), APIKey, GEngineIni);

	FString CmdLineURL;
	if (FParse::Value(FCommandLine::Get(), TEXT("OnsetDataStoreURL="), CmdLineURL))
	{
		BaseURL = CmdLineURL;
	}
	FString CmdLineAPIKey;
	if (FParse::Value(FCommandLine::Get(), TEXT("OnsetDataStoreAPIKey="), CmdLineAPIKey))
	{
		APIKey = CmdLineAPIKey;
	}

	if (BaseURL.IsEmpty())
	{
		UE_LOG(LogHttpStore, Error, TEXT("FHttpStore: ConnectionString (BaseURL) is empty"));
		return false;
	}

	if (!BaseURL.StartsWith(TEXT("https://")) && !BaseURL.StartsWith(TEXT("http://")))
	{
		BaseURL = TEXT("https://") + BaseURL;
	}
	if (APIKey.IsEmpty())
	{
		UE_LOG(LogHttpStore, Error, TEXT("FHttpStore: APIKey not set in [Onset.DataStore] config"));
		return false;
	}

	GConfig->GetString(TEXT("Onset.Auth"), TEXT("AuthTokenSecret"), AuthTokenSecret, GEngineIni);
	FString CmdLineTokenSecret;
	if (FParse::Value(FCommandLine::Get(), TEXT("OnsetAuthTokenSecret="), CmdLineTokenSecret))
	{
		AuthTokenSecret = CmdLineTokenSecret;
	}
	if (AuthTokenSecret.IsEmpty())
	{
		AuthTokenSecret = TEXT("default-dev-secret-change-me");
		UE_LOG(LogHttpStore, Warning, TEXT("FHttpStore: AuthTokenSecret not configured — using dev default"));
	}

	int32 Lifetime = 0;
	GConfig->GetInt(TEXT("Onset.Auth"), TEXT("AuthTokenLifetimeSeconds"), Lifetime, GEngineIni);
	if (Lifetime >= 30)
	{
		TokenLifetimeSeconds = Lifetime;
	}

	UE_LOG(LogHttpStore, Log, TEXT("FHttpStore initialized: BaseURL=%s"), *BaseURL);
	return true;
}

FString FHttpStore::BuildSignedToken(const FString& Platform, const FString& PlatformID, int32 SlotIndex) const
{
	if (AuthTokenSecret.IsEmpty()) return {};

	const int64 NowUnix = FDateTime::UtcNow().ToUnixTimestamp();
	const int64 ExpiryUnix = NowUnix + TokenLifetimeSeconds;

	const FString PayloadStr = FString::Printf(TEXT("%s|%s|%d|%lld"), *PlatformID, *Platform, SlotIndex, ExpiryUnix);
	FTCHARToUTF8 PayloadConv(*PayloadStr);
	TArray<uint8> PayloadBytes;
	PayloadBytes.Append(reinterpret_cast<const uint8*>(PayloadConv.Get()), PayloadConv.Length());
	const FString PayloadB64 = FBase64::Encode(PayloadBytes);

	FTCHARToUTF8 KeyConv(*AuthTokenSecret);
	TArray<uint8> KeyBytes;
	KeyBytes.Append(reinterpret_cast<const uint8*>(KeyConv.Get()), KeyConv.Length());

	const TArray<uint8> SigBytes = FSHA256::HmacSha256(KeyBytes, PayloadBytes);
	const FString SigB64 = FBase64::Encode(SigBytes);

	return PayloadB64 + TEXT(".") + SigB64;
}

bool FHttpStore::SendRequest(const FString& Verb, const FString& Path, const FString& Body, const FString& StoreToken, FString& OutBody, int32& OutStatusCode)
{
	FHttpModule& Http = FHttpModule::Get();
	TSharedRef<IHttpRequest> Request = Http.CreateRequest();

	FString FullURL = BaseURL + Path;
	Request->SetURL(FullURL);
	Request->SetVerb(Verb);
	Request->SetHeader(TEXT("X-API-Key"), APIKey);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	if (!StoreToken.IsEmpty())
	{
		Request->SetHeader(TEXT("X-Store-Token"), StoreToken);
	}

	if (!Body.IsEmpty())
	{
		Request->SetContentAsString(Body);
	}

	FHttpResponsePtr Response = nullptr;
	bool bCompleted = false;

	Request->OnProcessRequestComplete().BindLambda([&](FHttpRequestPtr, FHttpResponsePtr Resp, bool)
	{
		Response = Resp;
		bCompleted = true;
	});

	if (!Request->ProcessRequest())
	{
		UE_LOG(LogHttpStore, Error, TEXT("Failed to process request: %s %s"), *Verb, *Path);
		return false;
	}

	double Timeout = 10.0;
	double StartTime = FPlatformTime::Seconds();

	while (!bCompleted)
	{
		FHttpModule::Get().GetHttpManager().Tick(0.f);
		if (FPlatformTime::Seconds() - StartTime > Timeout)
		{
			Request->CancelRequest();
			UE_LOG(LogHttpStore, Error, TEXT("Request timed out: %s %s"), *Verb, *Path);
			return false;
		}
		FPlatformProcess::Sleep(0.01f);
	}

	if (!Response.IsValid())
	{
		UE_LOG(LogHttpStore, Error, TEXT("No response: %s %s"), *Verb, *Path);
		return false;
	}

	OutStatusCode = Response->GetResponseCode();
	OutBody = Response->GetContentAsString();

	if (OutStatusCode < 200 || OutStatusCode >= 300)
	{
		UE_LOG(LogHttpStore, Warning, TEXT("Request returned %d: %s %s — %s"),
			OutStatusCode, *Verb, *Path, *OutBody);
		return false;
	}

	return true;
}

bool FHttpStore::LoadAccount(const FString& Platform, const FString& PlatformID, FOnsetAccountData& OutAccount)
{
	FString Path = FString::Printf(TEXT("/account/%s/%s"), *Platform, *PlatformID);
	FString ResponseBody;
	int32 StatusCode = 0;

	if (!SendRequest(TEXT("GET"), Path, TEXT(""), BuildSignedToken(Platform, PlatformID, -1), ResponseBody, StatusCode))
	{
		return false;
	}

	TSharedPtr<FJsonObject> Json;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
	if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
	{
		UE_LOG(LogHttpStore, Error, TEXT("Failed to parse JSON response"));
		return false;
	}

	OutAccount.Platform = Json->GetStringField(TEXT("platform"));
	OutAccount.PlatformID = Json->GetStringField(TEXT("platformId"));

	const TArray<TSharedPtr<FJsonValue>>* SlotsArray = nullptr;
	if (Json->TryGetArrayField(TEXT("slots"), SlotsArray))
	{
		for (const TSharedPtr<FJsonValue>& SlotVal : *SlotsArray)
		{
			TSharedPtr<FJsonObject> SlotObj = SlotVal->AsObject();
			if (!SlotObj)
				continue;

			FOnsetCharacterSlotData Slot;
			Slot.SlotIndex = SlotObj->GetIntegerField(TEXT("slotIndex"));
			Slot.CharacterName = SlotObj->GetStringField(TEXT("characterName"));
			Slot.Level = SlotObj->GetIntegerField(TEXT("level"));
			Slot.CharacterClass = static_cast<EOnsetCharacterClass>(SlotObj->GetIntegerField(TEXT("characterClass")));
			Slot.bOccupied = SlotObj->GetBoolField(TEXT("bOccupied"));
			OutAccount.Slots.Add(Slot);
		}
	}

	return true;
}

bool FHttpStore::CreateAccount(const FString& Platform, const FString& PlatformID)
{
	FString Path = FString::Printf(TEXT("/account/%s/%s"), *Platform, *PlatformID);
	FString ResponseBody;
	int32 StatusCode = 0;

	return SendRequest(TEXT("POST"), Path, TEXT(""), BuildSignedToken(Platform, PlatformID, -1), ResponseBody, StatusCode);
}

bool FHttpStore::LoadCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex, FOnsetFullCharacterData& OutData)
{
	FString Path = FString::Printf(TEXT("/account/%s/%s/character/%d"), *Platform, *PlatformID, SlotIndex);
	FString ResponseBody;
	int32 StatusCode = 0;

	if (!SendRequest(TEXT("GET"), Path, TEXT(""), BuildSignedToken(Platform, PlatformID, SlotIndex), ResponseBody, StatusCode))
	{
		return false;
	}

	TSharedPtr<FJsonObject> Json;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
	if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
	{
		UE_LOG(LogHttpStore, Error, TEXT("Failed to parse character JSON"));
		return false;
	}

	OutData.SlotIndex = Json->GetIntegerField(TEXT("slotIndex"));
	OutData.CharacterName = Json->GetStringField(TEXT("characterName"));
	OutData.Level = Json->GetIntegerField(TEXT("level"));
	OutData.Experience = Json->GetIntegerField(TEXT("experience"));
	OutData.CurrentZone = Json->GetStringField(TEXT("currentZone"));
	OutData.SavedMaxHealth = static_cast<float>(Json->GetNumberField(TEXT("savedMaxHealth")));

	const TSharedPtr<FJsonObject>* PosObj = nullptr;
	if (Json->TryGetObjectField(TEXT("savedPosition"), PosObj) && PosObj->IsValid())
	{
		OutData.SavedPosition.X = (*PosObj)->GetNumberField(TEXT("x"));
		OutData.SavedPosition.Y = (*PosObj)->GetNumberField(TEXT("y"));
		OutData.SavedPosition.Z = (*PosObj)->GetNumberField(TEXT("z"));
	}

	OutData.SavedRotationYaw = static_cast<float>(Json->GetNumberField(TEXT("savedRotationYaw")));
	OutData.InventoryJSON = Json->GetStringField(TEXT("inventoryJson"));
	OutData.EquipmentJSON = Json->GetStringField(TEXT("equipmentJson"));
	OutData.QuestsJSON = Json->GetStringField(TEXT("questsJson"));
	OutData.CharacterClass = static_cast<EOnsetCharacterClass>(Json->GetIntegerField(TEXT("characterClass")));
	OutData.AppearanceJSON = Json->GetStringField(TEXT("appearanceJson"));

	return true;
}

bool FHttpStore::SaveCharacter(const FString& Platform, const FString& PlatformID, const FOnsetFullCharacterData& Data)
{
	FString Path = FString::Printf(TEXT("/account/%s/%s/character/%d"), *Platform, *PlatformID, Data.SlotIndex);

	TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);
	Json->SetNumberField(TEXT("slotIndex"), Data.SlotIndex);
	Json->SetStringField(TEXT("characterName"), Data.CharacterName);
	Json->SetNumberField(TEXT("level"), Data.Level);
	Json->SetNumberField(TEXT("experience"), Data.Experience);
	Json->SetStringField(TEXT("currentZone"), Data.CurrentZone);
	Json->SetNumberField(TEXT("savedMaxHealth"), Data.SavedMaxHealth);

	TSharedPtr<FJsonObject> PosObj = MakeShareable(new FJsonObject);
	PosObj->SetNumberField(TEXT("x"), Data.SavedPosition.X);
	PosObj->SetNumberField(TEXT("y"), Data.SavedPosition.Y);
	PosObj->SetNumberField(TEXT("z"), Data.SavedPosition.Z);
	Json->SetObjectField(TEXT("savedPosition"), PosObj);

	Json->SetNumberField(TEXT("savedRotationYaw"), Data.SavedRotationYaw);
	Json->SetStringField(TEXT("inventoryJson"), Data.InventoryJSON);
	Json->SetStringField(TEXT("equipmentJson"), Data.EquipmentJSON);
	Json->SetStringField(TEXT("questsJson"), Data.QuestsJSON);
	Json->SetNumberField(TEXT("characterClass"), static_cast<int32>(Data.CharacterClass));
	Json->SetStringField(TEXT("appearanceJson"), Data.AppearanceJSON);

	FString Body;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Body);
	if (!FJsonSerializer::Serialize(Json.ToSharedRef(), Writer))
	{
		UE_LOG(LogHttpStore, Error, TEXT("Failed to serialize character JSON"));
		return false;
	}

	FString ResponseBody;
	int32 StatusCode = 0;
	return SendRequest(TEXT("PUT"), Path, Body, BuildSignedToken(Platform, PlatformID, Data.SlotIndex), ResponseBody, StatusCode);
}

bool FHttpStore::DeleteCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex)
{
	FString Path = FString::Printf(TEXT("/account/%s/%s/character/%d"), *Platform, *PlatformID, SlotIndex);
	FString ResponseBody;
	int32 StatusCode = 0;

	return SendRequest(TEXT("DELETE"), Path, TEXT(""), BuildSignedToken(Platform, PlatformID, SlotIndex), ResponseBody, StatusCode);
}

void FHttpStore::SaveAll()
{
}

#endif
