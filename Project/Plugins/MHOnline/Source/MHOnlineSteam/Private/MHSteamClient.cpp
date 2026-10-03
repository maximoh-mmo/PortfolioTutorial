#include "MHSteamClient.h"

#if MH_STEAM_NATIVE
#include "OnlineSubsystem.h"
#include "SteamSharedModule.h"
#include "Containers/Queue.h"
THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
THIRD_PARTY_INCLUDES_END

class FMHNativeSteamClient final : public IMHSteamClient
{
    TSharedPtr<FSteamClientInstanceHandler> Instance;
    TMap<uint32, TArray<uint8>> Owned;
    TQueue<FMHSteamEvent, EQueueMode::Mpsc> Events;
    CCallbackManual<FMHNativeSteamClient, GetAuthSessionTicketResponse_t> SessionCallback;
    CCallbackManual<FMHNativeSteamClient, GetTicketForWebApiResponse_t> WebCallback;
    void OnSession(GetAuthSessionTicketResponse_t* Response)
    {
        Events.Enqueue({Response->m_hAuthTicket, Response->m_eResult == k_EResultOK, {}});
    }
    void OnWeb(GetTicketForWebApiResponse_t* Response)
    {
        FMHSteamEvent Event{Response->m_hAuthTicket, Response->m_eResult == k_EResultOK, {}};
        if (Event.bSuccess && Response->m_cubTicket > 0 && Response->m_cubTicket <= Response->k_nCubTicketMaxLength)
        { Event.Bytes.Append(Response->m_rgubTicket, Response->m_cubTicket); }
        Events.Enqueue(MoveTemp(Event));
    }
public:
    FMHNativeSteamClient()
    {
        // OSS owns the callback pump. Never initialize a second independent Steam API.
        if (IsRunningDedicatedServer() || !IOnlineSubsystem::Get(FName(TEXT("Steam")))) { return; }
        Instance = FSteamSharedModule::Get().ObtainSteamClientInstanceHandle();
        if (IsAvailable())
        {
            SessionCallback.Register(this, &FMHNativeSteamClient::OnSession);
            WebCallback.Register(this, &FMHNativeSteamClient::OnWeb);
        }
    }
    ~FMHNativeSteamClient() override
    {
        SessionCallback.Unregister();
        WebCallback.Unregister();
        TArray<uint32> Handles;
        Owned.GetKeys(Handles);
        for (uint32 Handle : Handles) { Cancel(Handle); }
    }
    bool IsAvailable() const override { return Instance && Instance->IsInitialized() && SteamUser(); }
    FString Account() const override
    {
        return IsAvailable() && SteamUser()->BLoggedOn()
            ? FString::Printf(TEXT("%llu"), SteamUser()->GetSteamID().ConvertToUint64()) : FString();
    }
    uint32 Begin(const FString& Audience) override
    {
        if (!IsAvailable()) { return 0; }
        TArray<uint8> Bytes;
        HAuthTicket Handle = k_HAuthTicketInvalid;
        if (Audience.StartsWith(TEXT("webapi:")))
        {
            Handle = SteamUser()->GetAuthTicketForWebApi(TCHAR_TO_UTF8(*Audience.Mid(7)));
        }
        else
        {
            SteamNetworkingIdentity Target;
            Target.Clear();
            Target.SetSteamID64(FCString::Strtoui64(*Audience.Mid(8), nullptr, 10));
            Bytes.SetNumUninitialized(2560);
            uint32 Size = 0;
            Handle = SteamUser()->GetAuthSessionTicket(Bytes.GetData(), Bytes.Num(), &Size, &Target);
            if (Handle != k_HAuthTicketInvalid && (Size == 0 || Size > uint32(Bytes.Num())))
            { SteamUser()->CancelAuthTicket(Handle); Handle = k_HAuthTicketInvalid; }
            Bytes.SetNum(FMath::Min(Size, uint32(Bytes.Num())));
        }
        if (Handle != k_HAuthTicketInvalid) { Owned.Add(Handle, MoveTemp(Bytes)); }
        return Handle;
    }
    void Cancel(uint32 Handle) override
    {
        if (auto* Bytes = Owned.Find(Handle))
        {
            if (!Bytes->IsEmpty()) { FMemory::Memzero(Bytes->GetData(), Bytes->Num()); }
            Owned.Remove(Handle);
            if (IsAvailable()) { SteamUser()->CancelAuthTicket(Handle); }
        }
    }
    bool Poll(FMHSteamEvent& Event) override
    {
        while (Events.Dequeue(Event))
        {
            if (auto* Bytes = Owned.Find(Event.Handle))
            {
                if (Event.Bytes.IsEmpty()) { Event.Bytes = MoveTemp(*Bytes); }
                return true;
            }
            if (!Event.Bytes.IsEmpty()) { FMemory::Memzero(Event.Bytes.GetData(), Event.Bytes.Num()); Event.Bytes.Empty(); }
        }
        return false;
    }
};
#else
class FMHNativeSteamClient final : public IMHSteamClient
{
public:
    bool IsAvailable() const override { return false; }
    FString Account() const override { return {}; }
    uint32 Begin(const FString&) override { return 0; }
    void Cancel(uint32) override {}
    bool Poll(FMHSteamEvent&) override { return false; }
};
#endif

TSharedRef<IMHSteamClient> MHCreateSteamClient() { return MakeShared<FMHNativeSteamClient>(); }
