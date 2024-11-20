#include "core/stdafx.h"
#include "tier0/fasttimer.h"
#include "tier0/commandline.h"
#include "tier1/cvar.h"
#include "filesystem/filesystem.h"
#include "ebisusdk/EbisuSDK.h"
#include "miles_impl.h"

static ConVar miles_debug("miles_debug", "0", FCVAR_DEVELOPMENTONLY, "Enables debug prints for the Miles Sound System", "1 = print; 0 (zero) = no print");
static ConVar miles_warnings("miles_warnings", "0", FCVAR_RELEASE, "Enables warning prints for the Miles Sound System", "1 = print; 0 (zero) = no print");

//-----------------------------------------------------------------------------
// Purpose: logs debug output emitted from the Miles Sound System
// Input  : nLogLevel - 
//          pszMessage - 
//-----------------------------------------------------------------------------
void AIL_LogFunc(int64_t nLogLevel, const char* pszMessage)
{
	Msg(eDLL_T::AUDIO, "%s\n", pszMessage);
	v_AIL_LogFunc(nLogLevel, pszMessage);
}

//-----------------------------------------------------------------------------
// Purpose: initializes the miles sound system
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool Miles_Initialize()
{
	const char* pszLanguage = HEbisuSDK_GetLanguage();
	const bool isDefaultLanguage = _stricmp(pszLanguage, MILES_DEFAULT_LANGUAGE) == 0;

	if (!isDefaultLanguage)
	{
		const bool useShipSound = !CommandLine()->FindParm("-devsound") || CommandLine()->FindParm("-shipsound");
		char baseStreamFilePath[MAX_PATH];

		V_snprintf(baseStreamFilePath, sizeof(baseStreamFilePath), "%s/general_%s.mstr", useShipSound ? "audio/ship" : "audio/dev", pszLanguage);

		// if the requested language for miles does not have a MSTR file present, throw a non-fatal error and force MILES_DEFAULT_LANGUAGE as a fallback
		// if we are loading MILES_DEFAULT_LANGUAGE and the file is still not found, we can let it hit the regular engine error, since that is not recoverable
		if (!FileSystem()->FileExists(baseStreamFilePath))
		{
			Error(eDLL_T::AUDIO, NO_ERROR, "%s: attempted to load language '%s' but the required streaming source file (%s) was not found. falling back to '%s'...\n",
				__FUNCTION__, pszLanguage, baseStreamFilePath, MILES_DEFAULT_LANGUAGE);

			pszLanguage = MILES_DEFAULT_LANGUAGE;
			miles_language->SetValue(pszLanguage);
		}
	}

	Msg(eDLL_T::AUDIO, "%s: initializing MSS with language: '%s'\n", __FUNCTION__, pszLanguage);
	CFastTimer initTimer;

	initTimer.Start();
	const bool bResult = v_Miles_Initialize();
	initTimer.End();

	Msg(eDLL_T::AUDIO, "%s: %s ('%f' seconds)\n", __FUNCTION__, bResult ? "success" : "failure", initTimer.GetDuration().GetSeconds());
	return bResult;
}

void MilesQueueEventRun(Miles::Queue* queue, const char* eventName)
{
	if(miles_debug.GetBool())
		Msg(eDLL_T::AUDIO, "%s: running event: '%s'\n", __FUNCTION__, eventName);

	v_MilesQueueEventRun(queue, eventName);
}

void MilesBankPatch(Miles::Bank* bank, char* streamPatch, char* localizedStreamPatch)
{
	if (miles_debug.GetBool())
	{
		Msg(eDLL_T::AUDIO,
			"%s: patching bank \"%s\". stream patches: \"%s\", \"%s\"\n",
			__FUNCTION__,
			bank->GetBankName(),
			V_UnqualifiedFileName(streamPatch), V_UnqualifiedFileName(localizedStreamPatch)
		);
	}

	const Miles::BankHeader_t* header = bank->GetHeader();

	if (header->bankIndex >= header->project->bankCount)
		Error(eDLL_T::AUDIO, EXIT_FAILURE,
			"%s: attempted to patch bank \"%s\" that identified itself as bank #%i, project expects a highest index of #%i\n",
			__FUNCTION__,
			bank->GetBankName(),
			header->bankIndex,
			header->project->bankCount - 1
		);

	v_MilesBankPatch(bank, streamPatch, localizedStreamPatch);
}

void CSOM_AddEventToQueue(const char* eventName)
{
	if (miles_debug.GetBool())
		Msg(eDLL_T::AUDIO, "%s: queuing audio event '%s'\n", __FUNCTION__, eventName);

	v_CSOM_AddEventToQueue(eventName);

	if (miles_warnings.GetBool())
	{
		if (g_milesGlobals->queuedEventHash == 1)
			Warning(eDLL_T::AUDIO, "%s: failed to add event to queue; invalid event name '%s'\n", __FUNCTION__, eventName);

		if (g_milesGlobals->queuedEventHash == 2)
			Warning(eDLL_T::AUDIO, "%s: failed to add event to queue; event '%s' not found.\n", __FUNCTION__, eventName);
	}
};


///////////////////////////////////////////////////////////////////////////////
void MilesCore::Detour(const bool bAttach) const
{
	DetourSetup(&v_AIL_LogFunc, &AIL_LogFunc, bAttach);
	DetourSetup(&v_Miles_Initialize, &Miles_Initialize, bAttach);
	DetourSetup(&v_MilesQueueEventRun, &MilesQueueEventRun, bAttach);
	DetourSetup(&v_MilesBankPatch, &MilesBankPatch, bAttach);
	DetourSetup(&v_CSOM_AddEventToQueue, &CSOM_AddEventToQueue, bAttach);
}
