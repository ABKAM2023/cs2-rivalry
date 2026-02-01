#include <stdio.h>
#include "Rivalry.h"
#include "metamod_oslink.h"
#include "schemasystem/schemasystem.h"
#include <map>
#include <string>
#include <cstdint>
#include <KeyValues.h>

Rivalry g_Rivalry;
PLUGIN_EXPOSE(Rivalry, g_Rivalry);
IVEngineServer2* engine = nullptr;
CGameEntitySystem* g_pGameEntitySystem = nullptr;
CEntitySystem* g_pEntitySystem = nullptr;
CGlobalVars *gpGlobals = nullptr;

IUtilsApi* g_pUtils;
IPlayersApi* g_pPlayers = nullptr;

std::map<std::string, std::string> g_vecPhrases;

void LoadTranslations()
{
	KeyValues::AutoDelete g_kvPhrases("Phrases");
	const char *pszPath = "addons/translations/rivalry.phrases.txt";
	if (!g_kvPhrases->LoadFromFile(g_pFullFileSystem, pszPath))
	{
		ConColorMsg(Color(255, 165, 0, 255), "[Rivalry] Failed to load %s\n", pszPath);
		return;
	}

	std::string szLanguage = std::string(g_pUtils->GetLanguage());
	const char* g_pszLanguage = szLanguage.c_str();
	for (KeyValues *pKey = g_kvPhrases->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey())
		g_vecPhrases[std::string(pKey->GetName())] = std::string(pKey->GetString(g_pszLanguage));
}

static uint64 BuildPlayerId(int iSlot)
{
	if (g_pPlayers)
	{
		uint64 steamId = g_pPlayers->GetSteamID64(iSlot);
		if (steamId != 0)
			return steamId;
	}
	return (1ULL << 63) | static_cast<uint64>(iSlot);
}

std::map<std::pair<uint64, uint64>, int> g_KillStats;

CGameEntitySystem* GameEntitySystem()
{
	return g_pUtils->GetCGameEntitySystem();
}

const char* GetPlayerName(int iSlot)
{
	CCSPlayerController* pController = CCSPlayerController::FromSlot(iSlot);
	if (!pController)
		return "Unknown";
	return pController->m_iszPlayerName();
}

void OnPlayerDeath(const char* szName, IGameEvent* pEvent, bool bDontBroadcast)
{
	if (!pEvent)
		return;

	int iVictim = pEvent->GetPlayerSlot("userid").Get();
	int iAttacker = pEvent->GetPlayerSlot("attacker").Get();

	if (iVictim == iAttacker || iAttacker == -1)
		return;

	CCSPlayerController* pVictimController = CCSPlayerController::FromSlot(iVictim);
	CCSPlayerController* pAttackerController = CCSPlayerController::FromSlot(iAttacker);

	if (!pVictimController || !pAttackerController)
		return;

	uint64 attackerId = BuildPlayerId(iAttacker);
	uint64 victimId = BuildPlayerId(iVictim);

	g_KillStats[{attackerId, victimId}]++;

	int iAttackerKills = g_KillStats[{attackerId, victimId}];
	int iVictimKills = g_KillStats[{victimId, attackerId}];

	const char* szAttackerName = pAttackerController->m_iszPlayerName();
	const char* szVictimName = pVictimController->m_iszPlayerName();

	g_pUtils->PrintToChat(iAttacker, g_vecPhrases["KillMessage"].c_str(), 
		szAttackerName, iAttackerKills, iVictimKills, szVictimName);

	g_pUtils->PrintToChat(iVictim, g_vecPhrases["DeathMessage"].c_str(), 
		szVictimName, iVictimKills, iAttackerKills, szAttackerName);
}

void StartupServer()
{
	g_pGameEntitySystem = GameEntitySystem();
	g_pEntitySystem = g_pUtils->GetCEntitySystem();
	gpGlobals = g_pUtils->GetCGlobalVars();

	g_KillStats.clear();
}

bool Rivalry::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, g_pSchemaSystem, ISchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetFileSystemFactory, g_pFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);

	g_SMAPI->AddListener( this, this );

	return true;
}

bool Rivalry::Unload(char *error, size_t maxlen)
{
	ConVar_Unregister();
	
	return true;
}

void Rivalry::AllPluginsLoaded()
{
	char error[64];
	int ret;
	g_pUtils = (IUtilsApi *)g_SMAPI->MetaFactory(Utils_INTERFACE, &ret, NULL);
	if (ret == META_IFACE_FAILED)
	{
		g_SMAPI->Format(error, sizeof(error), "Missing Utils system plugin");
		ConColorMsg(Color(255, 0, 0, 255), "[%s] %s\n", GetLogTag(), error);
		std::string sBuffer = "meta unload "+std::to_string(g_PLID);
		engine->ServerCommand(sBuffer.c_str());
		return;
	}
	g_pPlayers = (IPlayersApi *)g_SMAPI->MetaFactory(PLAYESample_INTERFACE, &ret, NULL);
	if (ret == META_IFACE_FAILED)
		ConColorMsg(Color(255, 165, 0, 255), "[%s] Missing Players API, fallback to slot-based IDs for bots/unauthed clients\n", GetLogTag());
	LoadTranslations();
	g_pUtils->StartupServer(g_PLID, StartupServer);
	g_pUtils->HookEvent(g_PLID, "player_death", OnPlayerDeath);
}

///////////////////////////////////////
const char* Rivalry::GetLicense()
{
	return "GPL";
}

const char* Rivalry::GetVersion()
{
	return "1.0.1";
}

const char* Rivalry::GetDate()
{
	return __DATE__;
}

const char *Rivalry::GetLogTag()
{
	return "[Rivalry]";
}

const char* Rivalry::GetAuthor()
{
	return "ABKAM";
}

const char* Rivalry::GetDescription()
{
	return "Rivalry";
}

const char* Rivalry::GetName()
{
	return "Rivalry";
}

const char* Rivalry::GetURL()
{
	return "https://discord.gg/ChYfTtrtmS";
}
