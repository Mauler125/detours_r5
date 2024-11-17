//=============================================================================//
//
// Purpose: Manage loading mods
//
//-----------------------------------------------------------------------------
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/commandline.h"
#include "tier1/cvar.h"
#include "tier2/fileutils.h"
#include "rtech/rson.h"
#include "localize/localize.h"
#include "modsystem.h"

//-----------------------------------------------------------------------------
// Console variables
//-----------------------------------------------------------------------------
static ConVar modsystem_enable("modsystem_enable", "1", FCVAR_RELEASE, "Enable the modsystem");
static ConVar modsystem_debug("modsystem_debug", "0", FCVAR_RELEASE, "Debug the modsystem");

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CModSystem::~CModSystem()
{
	// clear all allocated mod instances.
	m_ModList.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
// Purpose: initialize the mod system
// Input  :
//-----------------------------------------------------------------------------
void CModSystem::Init()
{
	if (!modsystem_enable.GetBool())
		return;

	// no mods installed, no point in initializing.
	if (!FileSystem()->IsDirectory(MOD_BASE_DIRECTORY, "PLATFORM"))
		return;

	// mod system initializes before the first Cbuf_Execute call, which
	// executes commands/convars over the command line. we check for an
	// explicit modsystem debug flag, and set the convar from here.
	if (CommandLine()->CheckParm("-modsystem_debug"))
		modsystem_debug.SetValue(true);

	CUtlVector<CUtlString> modFileList;
	RecursiveFindFilesMatchingName(modFileList,
		MOD_BASE_DIRECTORY, MOD_SETTINGS_FILE, "PLATFORM", '/');

	FOR_EACH_VEC(modFileList, i)
	{
		// allocate dynamically, so less memory/resources are required when
		// the vector has to grow and reallocate everything. we also got
		// a vector member in the modinstance struct, which would ultimately
		// lead into each item getting copy constructed into the mod list
		// by the 'move' constructor (CUtlVector also doesn't support being
		// nested unless its a pointer).
		CModSystem::ModInstance_t* mod = 
			new CModSystem::ModInstance_t(modFileList.Element(i).DirName());

		if (!mod->IsLoaded())
		{
			delete mod;
			continue;
		}

		m_ModList.AddToTail(mod);
	}

	UpdateModStatusList();
}

//-----------------------------------------------------------------------------
// Purpose: loads, updates, enforces and writes the mod status list
//-----------------------------------------------------------------------------
void CModSystem::UpdateModStatusList()
{
	CUtlMap<CUtlString, bool> enabledList(UtlStringLessFunc);
	LoadModStatusList(enabledList);
	
	// from here, we determine whether or not to enable the loaded mod.
	FOR_EACH_VEC(m_ModList, i)
	{
		ModInstance_t* mod = m_ModList[i];
		Assert(mod->IsLoaded());

		if (!enabledList.HasElement(mod->m_ModID))
		{
			if (modsystem_debug.GetBool())
				Msg(eDLL_T::ENGINE, "Mod '%s' does not exist in '%s'; enabling...\n",
					mod->m_ModID.Get(), MOD_STATUS_LIST_FILE);

			mod->SetState(eModState::ENABLED);
		}
		else
		{
			const bool bEnable = enabledList.FindElement(mod->m_ModID, false);
			mod->SetState(bEnable ? eModState::ENABLED : eModState::DISABLED);

			if (modsystem_debug.GetBool())
				Msg(eDLL_T::ENGINE, "Mod '%s' exists in '%s' and is %s.\n",
					mod->m_ModID.Get(), MOD_STATUS_LIST_FILE, bEnable ? "enabled" : "disabled");
		}
	}

	WriteModStatusList();
}

//-----------------------------------------------------------------------------
// Purpose: loads the mod status file from the disk
// Input  : &enabledList - 
//-----------------------------------------------------------------------------
void CModSystem::LoadModStatusList(CUtlMap<CUtlString, bool>& enabledList)
{
	if (!FileSystem()->FileExists(MOD_STATUS_LIST_FILE, "PLATFORM"))
		return;

	const KeyValues* pModList = FileSystem()->LoadKeyValues(
		IFileSystem::TYPE_COMMON, MOD_STATUS_LIST_FILE, "PLATFORM");

	for (KeyValues* pSubKey = pModList->GetFirstSubKey();
		pSubKey != nullptr; pSubKey = pSubKey->GetNextKey())
	{
		enabledList.Insert(pSubKey->GetName(), pSubKey->GetBool());
	}
}

//-----------------------------------------------------------------------------
// Purpose: writes the mod status file to the disk
//-----------------------------------------------------------------------------
void CModSystem::WriteModStatusList()
{
	KeyValues kv("ModList");

	FOR_EACH_VEC(m_ModList, i)
	{
		const ModInstance_t* mod = m_ModList[i];
		bool enabled = false;

		if (mod->m_iState == eModState::ENABLED)
			enabled = true;

		kv.SetBool(mod->m_ModID.Get(), enabled);
	}

	CUtlBuffer buf = CUtlBuffer(ssize_t(0), 0, CUtlBuffer::TEXT_BUFFER);
	kv.RecursiveSaveToFile(buf, 0);

	if (!FileSystem()->WriteFile(MOD_STATUS_LIST_FILE, "PLATFORM", buf))
		Error(eDLL_T::ENGINE, NO_ERROR, "Failed to write mod status list '%s'.\n", MOD_STATUS_LIST_FILE);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &basePath - 
//-----------------------------------------------------------------------------
CModSystem::ModInstance_t::ModInstance_t(const CUtlString& basePath)
{
	m_SettingsKV = nullptr;
	m_bHasScriptCompileList = false;

	m_BasePath = basePath;
	m_BasePath.AppendSlash('/');

	SetState(eModState::LOADING);

	if (!ParseSettings())
	{
		SetState(eModState::UNLOADED);
		return;
	}

	// parse any additional info from mod.vdf
	ParseConVars();
	ParseLocalizationFiles();

	// add mod folder to search paths so files can be easily loaded from here
	// [rexx]: maybe this isn't ideal as the only way of finding the mod's files,
	//         as there may be name clashes in files where the engine
	//         won't really care about the input file name. it may be better to,
	//         where possible, request files by file path relative to root
	//         (i.e. including platform/mods/{mod}/)
	// [amos]: it might be better to pack core files into the VPK, and disable
	//         the filesystem cache to disk reroute to avoid the file name
	//         clashing problems, research required.
	FileSystem()->AddSearchPath(m_BasePath.Get(), "PLATFORM", SearchPathAdd_t::PATH_ADD_TO_TAIL);

	const CUtlString scriptsRsonPath = m_BasePath + GAME_SCRIPT_COMPILELIST;

	if (FileSystem()->FileExists(scriptsRsonPath.Get(), "PLATFORM"))
		m_bHasScriptCompileList = true;

	SetState(eModState::LOADED);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CModSystem::ModInstance_t::~ModInstance_t()
{
	if (m_SettingsKV)
		delete m_SettingsKV;

	m_ConVars.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
// Purpose: gets a keyvalue from settings KV, and logs an error on failure
// Input  : *settingsPath - 
//          *key          - 
// Output : pointer to KeyValues object
//-----------------------------------------------------------------------------
KeyValues* CModSystem::ModInstance_t::GetRequiredSettingsKey(
	const char* settingsPath, const char* key) const
{
	KeyValues* pKeyValue = m_SettingsKV->FindKey(key);
	if (!pKeyValue)
		Error(eDLL_T::ENGINE, NO_ERROR,
			"Mod settings '%s' has missing or invalid '%s' field; skipping...\n",
			settingsPath, key);

	return pKeyValue;
}

//-----------------------------------------------------------------------------
// Purpose: loads the settings KV and parses the main values
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CModSystem::ModInstance_t::ParseSettings()
{
	const CUtlString settingsPath = m_BasePath + MOD_SETTINGS_FILE;
	const char* pSettingsPath = settingsPath.Get();

	m_SettingsKV = FileSystem()->LoadKeyValues(
		IFileSystem::TYPE_COMMON, pSettingsPath, "PLATFORM");

	if (!m_SettingsKV)
	{
		Error(eDLL_T::ENGINE, NO_ERROR,
			"Failed to parse mod settings '%s'; skipping...\n", m_BasePath.Get());
		return false;
	}

	// "name" "An R5Reloaded Mod"
	// [rexx]: could be optional and have id as fallback
	KeyValues* pName = GetRequiredSettingsKey(pSettingsPath, "name");
	if (!pName)
		return false;

	m_Name = pName->GetString();

	// "version" "1.0.0"
	KeyValues* pVersion = GetRequiredSettingsKey(pSettingsPath, "version");
	if (!pVersion)
		return false;

	m_Version = pVersion->GetString();

	// "id" "r5reloaded.TestMod"
	KeyValues* pId = GetRequiredSettingsKey(pSettingsPath, "id");
	if (!pId)
		return false;

	m_ModID = pId->GetString();

	// optional mod description field
	m_Description = m_SettingsKV->GetString(pSettingsPath, "description");

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: parses and registers convars listed in settings KV
//-----------------------------------------------------------------------------
void CModSystem::ModInstance_t::ParseConVars()
{
	Assert(m_SettingsKV);
	const KeyValues* pConVars = m_SettingsKV->FindKey("ConVars");

	if (pConVars)
	{
		for (KeyValues* pSubKey = pConVars->GetFirstSubKey();
			pSubKey != nullptr; pSubKey = pSubKey->GetNextKey())
		{
			const char* pszName = pSubKey->GetName();
			const char* pszFlagsString = pSubKey->GetString("flags", "NONE");
			const char* pszHelpString = pSubKey->GetString("helpText");
			const char* pszUsageString = pSubKey->GetString("usageText");

			KeyValues* pValues = pSubKey->FindKey("Values");

			const char* pszDefaultValue = "0";
			bool bMin = false;
			bool bMax = false;
			float fMin = 0.f;
			float fMax = 0.f;

			if (pValues)
			{
				pszDefaultValue = pValues->GetString("default", "0");

				// minimum cvar value
				if (pValues->FindKey("min"))
				{
					bMin = true; // has min value
					fMin = pValues->GetFloat("min", 0.f);
				}

				// maximum cvar value
				if (pValues->FindKey("max"))
				{
					bMax = true; // has max value
					fMax = pValues->GetFloat("max", 1.f);
				}
			}

			int flags;
			if (ConVar_ParseFlagString(pszFlagsString, flags, pszName))
			{
				if (g_pCVar->FindCommandBase(pszName) != nullptr)
				{
					Warning(eDLL_T::ENGINE, NO_ERROR, "Failed to register ConVar '%s' for mod '%s' ('%s'); already registered.\n",
						pszName, m_Name.String(), m_ModID.String());

					continue;
				}

				ConVar* cvar = new ConVar(pszName, pszDefaultValue, flags, pszHelpString, bMin, fMin, bMax, fMax, nullptr, pszUsageString);

				if (!cvar)
				{
					// Quit as we ran out of memory.
					Error(eDLL_T::ENGINE, EXIT_FAILURE, "Failed to register ConVar '%s' for mod '%s' ('%s'); allocation failure.\n",
						pszName, m_Name.String(), m_ModID.String());

					return;
				}

				m_ConVars.AddToTail(cvar);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: parses and stores localization file paths in a vector
//-----------------------------------------------------------------------------
void CModSystem::ModInstance_t::ParseLocalizationFiles()
{
	Assert(m_SettingsKV);

	const KeyValues* pLocalizationFiles = m_SettingsKV->FindKey("LocalizationFiles");

	if (pLocalizationFiles)
	{
		for (KeyValues* pSubKey = pLocalizationFiles->GetFirstSubKey();
			pSubKey != nullptr; pSubKey = pSubKey->GetNextKey())
		{
			m_LocalizationFiles.AddToTail(m_BasePath + pSubKey->GetName());
		}
	}
}

CModSystem g_ModSystem;
