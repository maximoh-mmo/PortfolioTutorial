#include "Modules/ModuleManager.h"

#if MH_STEAM_NATIVE
#include "Containers/Ticker.h"
#include "Misc/App.h"
THIRD_PARTY_INCLUDES_START
#include "steam/steam_api.h"
#include "steam/steam_gameserver.h"
THIRD_PARTY_INCLUDES_END

DEFINE_LOG_CATEGORY_STATIC(LogMHOnlineSteam, Log, All);

/**
 * Observes Steam Game Server callbacks already pumped by OnlineSubsystemSteam.
 * It is intentionally diagnostic-only: it neither starts Steam nor accepts or
 * rejects authentication requests.
 */
class FMHSteamServerDiagnostics final
{
public:
	FMHSteamServerDiagnostics()
		: ServersConnectedCallback(this, &FMHSteamServerDiagnostics::OnServersConnected)
		, ServersDisconnectedCallback(this, &FMHSteamServerDiagnostics::OnServersDisconnected)
		, ServerConnectFailureCallback(this, &FMHSteamServerDiagnostics::OnServerConnectFailure)
		, PolicyResponseCallback(this, &FMHSteamServerDiagnostics::OnPolicyResponse)
		, ValidateAuthTicketResponseCallback(this, &FMHSteamServerDiagnostics::OnValidateAuthTicketResponse)
	{
	}

	~FMHSteamServerDiagnostics()
	{
		ServersConnectedCallback.Unregister();
		ServersDisconnectedCallback.Unregister();
		ServerConnectFailureCallback.Unregister();
		PolicyResponseCallback.Unregister();
		ValidateAuthTicketResponseCallback.Unregister();
	}

private:
	STEAM_GAMESERVER_CALLBACK(FMHSteamServerDiagnostics, OnServersConnected, SteamServersConnected_t, ServersConnectedCallback);
	STEAM_GAMESERVER_CALLBACK(FMHSteamServerDiagnostics, OnServersDisconnected, SteamServersDisconnected_t, ServersDisconnectedCallback);
	STEAM_GAMESERVER_CALLBACK(FMHSteamServerDiagnostics, OnServerConnectFailure, SteamServerConnectFailure_t, ServerConnectFailureCallback);
	STEAM_GAMESERVER_CALLBACK(FMHSteamServerDiagnostics, OnPolicyResponse, GSPolicyResponse_t, PolicyResponseCallback);
	STEAM_GAMESERVER_CALLBACK(FMHSteamServerDiagnostics, OnValidateAuthTicketResponse, ValidateAuthTicketResponse_t, ValidateAuthTicketResponseCallback);

};

void FMHSteamServerDiagnostics::OnServersConnected(SteamServersConnected_t*)
{
	UE_LOG(LogMHOnlineSteam, Log, TEXT("Steam diagnostics: game-server backend connected."));
}

void FMHSteamServerDiagnostics::OnServersDisconnected(SteamServersDisconnected_t* Data)
{
	UE_LOG(LogMHOnlineSteam, Warning, TEXT("Steam diagnostics: game-server backend disconnected (result=%d)."), static_cast<int32>(Data->m_eResult));
}

void FMHSteamServerDiagnostics::OnServerConnectFailure(SteamServerConnectFailure_t* Data)
{
	UE_LOG(LogMHOnlineSteam, Warning, TEXT("Steam diagnostics: game-server backend connection failure (result=%d, retrying=%d)."), static_cast<int32>(Data->m_eResult), Data->m_bStillRetrying ? 1 : 0);
}

void FMHSteamServerDiagnostics::OnPolicyResponse(GSPolicyResponse_t* Data)
{
	UE_LOG(LogMHOnlineSteam, Log, TEXT("Steam diagnostics: game-server policy response (secure=%d)."), Data->m_bSecure ? 1 : 0);
}

void FMHSteamServerDiagnostics::OnValidateAuthTicketResponse(ValidateAuthTicketResponse_t* Data)
{
	UE_LOG(LogMHOnlineSteam, Log, TEXT("Steam diagnostics: ticket validation callback received (response=%d)."), static_cast<int32>(Data->m_eAuthSessionResponse));
}
#endif

class FMHOnlineSteamModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if MH_STEAM_NATIVE
		if (IsRunningDedicatedServer())
		{
			StartupTicker = FTSTicker::GetCoreTicker().AddTicker(
				FTickerDelegate::CreateRaw(this, &FMHOnlineSteamModule::TryStartServerDiagnostics), 0.5f);
		}
#endif
	}

	virtual void ShutdownModule() override
	{
#if MH_STEAM_NATIVE
		if (StartupTicker.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(StartupTicker);
			StartupTicker.Reset();
		}
		ServerDiagnostics.Reset();
#endif
	}

private:
#if MH_STEAM_NATIVE
	bool TryStartServerDiagnostics(float)
	{
		if (SteamGameServer())
		{
			ServerDiagnostics = MakeUnique<FMHSteamServerDiagnostics>();
			UE_LOG(LogMHOnlineSteam, Log, TEXT("Steam diagnostics: observing Game Server callbacks."));
			return false;
		}
		return true;
	}

	FTSTicker::FDelegateHandle StartupTicker;
	TUniquePtr<FMHSteamServerDiagnostics> ServerDiagnostics;
#endif
};

IMPLEMENT_MODULE(FMHOnlineSteamModule, MHOnlineSteam)
