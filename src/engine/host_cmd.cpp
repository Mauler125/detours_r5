#include "core/stdafx.h"
#include "tier0/commandline.h"
#include "rtech/pak/pakstate.h"
#include "host_cmd.h"
#include "common.h"
#include "client/client.h"
#ifndef DEDICATED
#include "windows/id3dx.h"
#endif // !DEDICATED

#ifndef DEDICATED
static void DoNothing(){};

/*
==================
Host_SetupUIMaterials

 setup and initialize
 UI materials
==================
*/
static void Host_SetupUIMaterials()
{
	// Don't sync during video init as this is where this function is called
	// from. We restore the function pointer after we loaded the pak file.
	void* const oldSyncFn = g_pakGlobals->threadSyncFunc;
	g_pakGlobals->threadSyncFunc = DoNothing;

	static const char* const pakFileName = "ui_mainmenu.rpak";
	const PakHandle_t pak = g_pakLoadApi->LoadAsync(pakFileName, AlignedMemAlloc(), 3, false);

	// NOTE: make sure to wait for the async load, as the pak must be loaded
	// before we continue processing UI materials.
	if (pak == PAK_INVALID_HANDLE || !g_pakLoadApi->WaitForAsyncLoad(pak, DoNothing))
		Error(eDLL_T::ENGINE, EXIT_FAILURE, "Failed to load pak file '%s'\n", pakFileName);

	g_pakGlobals->threadSyncFunc = oldSyncFn;

	v_Host_SetupUIMaterials();
}
#endif // !DEDICATED

/*
==================
Host_Shutdown

 shutdown host
 systems
==================
*/
static void Host_Shutdown()
{
#ifndef DEDICATED
	DirectX_Shutdown();
#endif // DEDICATED
	v_Host_Shutdown();
}

/*
==================
Host_Status_PrintClient

 Print client info 
 to console
==================
*/
static void Host_Status_PrintClient(CClient* client, bool bShowAddress, void (*print) (const char* fmt, ...))
{
	CNetChan* nci = client->GetNetChan();
	const char* state = "challenging";

	if (client->IsActive())
		state = "active";
	else if (client->IsSpawned())
		state = "spawning";
	else if (client->IsConnected())
		state = "connecting";

	if (nci != NULL)
	{
		print("# %hu \"%s\" %llu %s %i %i %s %d\n",
			client->GetHandle(), client->GetServerName(), client->GetNucleusID(), COM_FormatSeconds(static_cast<int>(nci->GetTimeConnected())),
			static_cast<int>(1000.0f * nci->GetAvgLatency(FLOW_OUTGOING)), static_cast<int>(100.0f * nci->GetAvgLoss(FLOW_INCOMING)), state, nci->GetDataRate());

		if (bShowAddress)
		{
			print(" %s\n", nci->GetAddress());
		}
	}
	else
	{
		print("#%2hu \"%s\" %llu %s\n", client->GetHandle(), client->GetServerName(), client->GetNucleusID(), state);
	}

	//print("\n");
}

/*
==================
DFS_InitializeFeatureFlagDefinitions

 Initialize feature
 flag definitions
==================
*/
static bool DFS_InitializeFeatureFlagDefinitions(const char* pszFeatureFlags)
{
	if (CommandLine()->CheckParm("-nodfs"))
		return false;

	return v_DFS_InitializeFeatureFlagDefinitions(pszFeatureFlags);
}

///////////////////////////////////////////////////////////////////////////////
void VHostCmd::Detour(const bool bAttach) const
{
#ifndef DEDICATED
	DetourSetup(&v_Host_SetupUIMaterials, &Host_SetupUIMaterials, bAttach);
#endif // !DEDICATED
	DetourSetup(&v_Host_Shutdown, &Host_Shutdown, bAttach);
	DetourSetup(&v_Host_Status_PrintClient, &Host_Status_PrintClient, bAttach);
	DetourSetup(&v_DFS_InitializeFeatureFlagDefinitions, &DFS_InitializeFeatureFlagDefinitions, bAttach);
}

///////////////////////////////////////////////////////////////////////////////
EngineParms_t* g_pEngineParms = nullptr;