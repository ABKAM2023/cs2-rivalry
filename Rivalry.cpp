#include <stdio.h>
#include "Rivalry.h"
#include "metamod_oslink.h"
#include "schemasystem/schemasystem.h"
#include <map>
#include <string>
#include <KeyValues.h>

Rivalry g_Rivalry;
PLUGIN_EXPOSE(Rivalry, g_Rivalry);
IVEngineServer2* engine = nullptr;
CGameEntitySystem* g_pGameEntitySystem = nullptr;
CEntitySystem* g_pEntitySystem = nullptr;
CGlobalVars *gpGlobals = nullptr;

IUtilsApi* g_pUtils;

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

std::map<std::pair<int, int>, int> g_KillStats;

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

	g_KillStats[{iAttacker, iVictim}]++;

	int iAttackerKills = g_KillStats[{iAttacker, iVictim}];
	int iVictimKills = g_KillStats[{iVictim, iAttacker}];

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
	return "1.0";
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
