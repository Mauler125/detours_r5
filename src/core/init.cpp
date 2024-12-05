//=============================================================================//
//
// Purpose: Main systems initialization file
//
//=============================================================================//

#include "core/stdafx.h"
#include "core/logdef.h"
#include "core/init.h"
#include "tier0/jobthread.h"
#include "tier0/threadtools.h"
#include "tier0/tslist.h"
#include "tier0/memstd.h"
#include "tier0/fasttimer.h"
#include "tier0/cpu.h"
#include "tier0/commandline.h"
#include "tier0/platform_internal.h"
#include "tier0/sigcache.h"
#include "tier1/cmd.h"
#include "tier1/cvar.h"
#include "tier1/keyvalues_iface.h"
#include "vpc/IAppSystem.h"
#include "vpc/interfaces.h"
#include "common/callback.h"
#include "common/completion.h"
#include "vstdlib/keyvaluessystem.h"
#include "common/opcodes.h"
#include "common/netmessages.h"
#include "launcher/prx.h"
#include "launcher/launcher.h"
#include "filesystem/basefilesystem.h"
#include "filesystem/filesystem.h"
#include "datacache/mdlcache.h"
#include "ebisusdk/EbisuSDK.h"
#ifndef DEDICATED
#include "codecs/bink/bink_impl.h"
#include "codecs/miles/miles_impl.h"
#include "codecs/miles/miles_shim.h"
#include "codecs/miles/radshal_wasapi.h"
#endif // !DEDICATED
#include "vphysics/physics_collide.h"
#include "vphysics/QHull.h"
#include "engine/staticpropmgr.h"
#include "materialsystem/cmaterialsystem.h"
#ifndef DEDICATED
#include "materialsystem/cmaterialglue.h"
#include "vgui/vgui_baseui_interface.h"
#include "vgui/vgui_debugpanel.h"
#include "vgui/vgui_fpspanel.h"
#include "vgui/vgui_controls/RichText.h"
#include "vguimatsurface/MatSystemSurface.h"
#include "engine/client/vengineclient_impl.h"
#include "engine/client/cdll_engine_int.h"
#include "engine/client/datablock_receiver.h"
#endif // !DEDICATED
#ifndef CLIENT_DLL
#include "engine/server/server.h"
#include "engine/server/persistence.h"
#include "engine/server/vengineserver_impl.h"
#include "engine/server/datablock_sender.h"
#endif // !CLIENT_DLL
#include "studiorender/studiorendercontext.h"
#ifndef CLIENT_DLL
#include "rtech/liveapi/liveapi.h"
#endif // !CLIENT_DLL
#include "rtech/rstdlib.h"
#include "rtech/rson.h"
#include "rtech/async/asyncio.h"
#include "rtech/pak/pakalloc.h"
#include "rtech/pak/pakparse.h"
#include "rtech/pak/pakstate.h"
#include "rtech/pak/pakstream.h"
#include "rtech/stryder/stryder.h"
#include "rtech/playlists/playlists.h"
#ifndef DEDICATED
#include "rtech/rui/rui.h"
#include "engine/client/cl_ents_parse.h"
#include "engine/client/cl_main.h"
#include "engine/client/cl_rcon.h"
#include "engine/client/cl_splitscreen.h"
#endif // !DEDICATED
#include "engine/client/client.h"
#ifndef DEDICATED
#include "engine/client/clientstate.h"
#endif // !DEDICATED
#include "localize/localize.h"
#include "engine/enginetrace.h"
#include "engine/traceinit.h"
#include "engine/common.h"
#include "engine/cmodel_bsp.h"
#include "engine/modelinfo.h"
#include "engine/host.h"
#include "engine/host_cmd.h"
#include "engine/host_state.h"
#include "engine/modelloader.h"
#include "engine/cmd.h"
#include "engine/net.h"
#include "engine/net_chan.h"
#include "engine/networkstringtable.h"
#ifndef CLIENT_DLL
#include "engine/server/sv_main.h"
#include "engine/server/sv_rcon.h"
#endif // !CLIENT_DLL
#include "engine/sdk_dll.h"
#include "engine/sys_dll.h"
#include "engine/sys_dll2.h"
#include "engine/sys_engine.h"
#include "engine/sys_utils.h"
#ifndef DEDICATED
#include "engine/sys_getmodes.h"
#include "engine/sys_mainwind.h"
#include "engine/matsys_interface.h"
#include "engine/gl_rmain.h"
#include "engine/gl_matsysiface.h"
#include "engine/gl_drawlights.h"
#include "engine/gl_screen.h"
#include "engine/gl_rsurf.h"
#include "engine/debugoverlay.h"
#include "engine/keys.h"
#endif // !DEDICATED
#include "vscript/languages/squirrel_re/include/squirrel.h"
#include "vscript/languages/squirrel_re/include/sqvm.h"
#include "vscript/languages/squirrel_re/include/sqstdaux.h"
#include "vscript/languages/squirrel_re/vsquirrel.h"
#include "vscript/vscript.h"
#include "game/shared/r1/weapon_bolt.h"
#include "game/shared/util_shared.h"
#include "game/shared/usercmd.h"
#include "game/shared/animation.h"
#include "game/shared/vscript_shared.h"
#ifndef CLIENT_DLL
#include "game/server/ai_node.h"
#include "game/server/ai_network.h"
#include "game/server/ai_networkmanager.h"
#include "game/server/ai_utility.h"
#include "game/server/detour_impl.h"
#include "game/server/gameinterface.h"
#include "game/server/movehelper_server.h"
#include "game/server/player.h"
#include "game/server/player_command.h"
#include "game/server/ai_basenpc.h"
#include "game/server/physics_main.h"
#include "game/server/vscript_server.h"
#include "game/server/entitylist.h"
#endif // !CLIENT_DLL
#ifndef DEDICATED
#include "game/client/viewrender.h"
#include "game/client/input.h"
#include "game/client/gamepad.h"
#include "game/client/movehelper_client.h"
#include "game/client/vscript_client.h"
#include "game/client/cliententitylist.h"
#include "game/client/c_player.h"
#endif // !DEDICATED
#include "public/edict.h"
#ifndef DEDICATED
#include "public/idebugoverlay.h"
#include "inputsystem/inputsystem.h"
#include "inputsystem/inputstacksystem.h"
#include "windows/id3dx.h"
#endif // !DEDICATED

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protossl.h"
#include "DirtySDK/proto/protowebsocket.h"


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// ██╗███╗   ██╗██╗████████╗██╗ █████╗ ██╗     ██╗███████╗ █████╗ ████████╗██╗ ██████╗ ███╗   ██╗
// ██║████╗  ██║██║╚══██╔══╝██║██╔══██╗██║     ██║╚══███╔╝██╔══██╗╚══██╔══╝██║██╔═══██╗████╗  ██║
// ██║██╔██╗ ██║██║   ██║   ██║███████║██║     ██║  ███╔╝ ███████║   ██║   ██║██║   ██║██╔██╗ ██║
// ██║██║╚██╗██║██║   ██║   ██║██╔══██║██║     ██║ ███╔╝  ██╔══██║   ██║   ██║██║   ██║██║╚██╗██║
// ██║██║ ╚████║██║   ██║   ██║██║  ██║███████╗██║███████╗██║  ██║   ██║   ██║╚██████╔╝██║ ╚████║
// ╚═╝╚═╝  ╚═══╝╚═╝   ╚═╝   ╚═╝╚═╝  ╚═╝╚══════╝╚═╝╚══════╝╚═╝  ╚═╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝
//
/////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef DEDICATED
// These command line parameters disable a bunch of things in the engine that
// the dedicated server does not need, therefore, reducing a lot of overhead.
void InitCommandLineParameters()
{
	CommandLine()->AppendParm("-collate", "");
	CommandLine()->AppendParm("-multiple", "");
	CommandLine()->AppendParm("-noorigin", "");
	CommandLine()->AppendParm("-nodiscord", "");
	CommandLine()->AppendParm("-noshaderapi", "");
	CommandLine()->AppendParm("-nobakedparticles", "");
	CommandLine()->AppendParm("-novid", "");
	CommandLine()->AppendParm("-nomenuvid", "");
	CommandLine()->AppendParm("-nosound", "");
	CommandLine()->AppendParm("-nomouse", "");
	CommandLine()->AppendParm("-nojoy", "");
	CommandLine()->AppendParm("-nosendtable", "");
}
#endif // DEDICATED

void ScriptConstantRegistrationCallback(CSquirrelVM* s)
{
	Script_RegisterListenServerConstants(s);
}

void Systems_Init()
{
	Msg(eDLL_T::NONE, "+-------------------------------------------------------------+\n");
	QuerySystemInfo();

	DetourRegister();
	CFastTimer initTimer;

	initTimer.Start();
	DetourInit();
	initTimer.End();

	Msg(eDLL_T::NONE, "+-------------------------------------------------------------+\n");
	Msg(eDLL_T::NONE, "%-16s '%10.6f' seconds ('%12lu' clocks)\n", "Detour->InitDB()",
		initTimer.GetDuration().GetSeconds(), initTimer.GetDuration().GetCycles());

	initTimer.Start();

	// Begin the detour transaction to hook the process
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// Hook functions
	for (const IDetour* pd : g_DetourVec)
	{
		pd->Detour(true);
	}

	// Patch instructions
	RuntimePtc_Init();

	// Commit the transaction
	HRESULT hr = DetourTransactionCommit();
	if (hr != NO_ERROR)
	{
		// Failed to hook into the process, terminate
		Assert(0);
		Error(eDLL_T::COMMON, 0xBAD0C0DE, "Failed to detour process: error code = %08x\n", hr);
	}

	initTimer.End();
	Msg(eDLL_T::NONE, "%-16s '%10.6f' seconds ('%12lu' clocks)\n", "Detour->Attach()",
		initTimer.GetDuration().GetSeconds(), initTimer.GetDuration().GetCycles());
	Msg(eDLL_T::NONE, "+-------------------------------------------------------------+\n");
	Msg(eDLL_T::NONE, "\n");

#ifdef DEDICATED
	InitCommandLineParameters();
#else
	// Must append these here if user specified the -offline parameter so the
	// engine can take care of disabling the platform systems on time. The
	// dedicated server already has these disabled, so we don't need to check
	// for it here.
	if (CommandLine()->CheckParm("-offline"))
	{
		CommandLine()->AppendParm("-noorigin", "");
		CommandLine()->AppendParm("-nodiscord", "");
	}
#endif// DEDICATED

	// Script context registration callbacks.
	ScriptConstantRegister_Callback = ScriptConstantRegistrationCallback;

#ifndef CLIENT_DLL
	ServerScriptRegister_Callback = Script_RegisterServerFunctions;
	ServerScriptRegisterEnum_Callback = Script_RegisterServerEnums;
#endif// !CLIENT_DLL

#ifndef SERVER_DLL
	ClientScriptRegister_Callback = Script_RegisterClientFunctions;
	UiScriptRegister_Callback =  Script_RegisterUIFunctions;

#ifndef CLIENT_DLL
	UiServerScriptRegister_Callback = Script_RegisterUIServerFunctions;
	UiAdminPanelScriptRegister_Callback = Script_RegisterAdminServerFunctions;
#endif // !CLIENT_DLL
#endif // !SERVER_DLL

#ifdef CLIENT_DLL
	g_bClientDLL = true;
#endif
}

//////////////////////////////////////////////////////////////////////////
//
// ███████╗██╗  ██╗██╗   ██╗████████╗██████╗  ██████╗ ██╗    ██╗███╗   ██╗
// ██╔════╝██║  ██║██║   ██║╚══██╔══╝██╔══██╗██╔═══██╗██║    ██║████╗  ██║
// ███████╗███████║██║   ██║   ██║   ██║  ██║██║   ██║██║ █╗ ██║██╔██╗ ██║
// ╚════██║██╔══██║██║   ██║   ██║   ██║  ██║██║   ██║██║███╗██║██║╚██╗██║
// ███████║██║  ██║╚██████╔╝   ██║   ██████╔╝╚██████╔╝╚███╔███╔╝██║ ╚████║
// ╚══════╝╚═╝  ╚═╝ ╚═════╝    ╚═╝   ╚═════╝  ╚═════╝  ╚══╝╚══╝ ╚═╝  ╚═══╝
//
//////////////////////////////////////////////////////////////////////////

void Systems_Shutdown()
{
	// Shutdown RCON (closes all open sockets)
#ifndef CLIENT_DLL
	RCONServer()->Shutdown();
#endif// !CLIENT_DLL
#ifndef SERVER_DLL
	RCONClient()->Shutdown();
#endif // !SERVER_DLL

#ifndef CLIENT_DLL
	LiveAPISystem()->Shutdown();
#endif// !CLIENT_DLL

	CFastTimer shutdownTimer;
	shutdownTimer.Start();

	// Begin the detour transaction to unhook the process
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// Unhook functions
	for (const IDetour* pd : g_DetourVec)
	{
		pd->Detour(false);
	}

	// Commit the transaction
	DetourTransactionCommit();

	shutdownTimer.End();
	Msg(eDLL_T::NONE, "%-16s '%10.6f' seconds ('%12lu' clocks)\n", "Detour->Detach()",
		shutdownTimer.GetDuration().GetSeconds(), shutdownTimer.GetDuration().GetCycles());
	Msg(eDLL_T::NONE, "+-------------------------------------------------------------+\n");
	Msg(eDLL_T::NONE, "\n");
}

/////////////////////////////////////////////////////
//
// ██╗   ██╗████████╗██╗██╗     ██╗████████╗██╗   ██╗
// ██║   ██║╚══██╔══╝██║██║     ██║╚══██╔══╝╚██╗ ██╔╝
// ██║   ██║   ██║   ██║██║     ██║   ██║    ╚████╔╝ 
// ██║   ██║   ██║   ██║██║     ██║   ██║     ╚██╔╝  
// ╚██████╔╝   ██║   ██║███████╗██║   ██║      ██║   
//  ╚═════╝    ╚═╝   ╚═╝╚══════╝╚═╝   ╚═╝      ╚═╝   
//
/////////////////////////////////////////////////////

void Winsock_Startup()
{
	WSAData wsaData{};
	const int nError = ::WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (nError != 0)
	{
		Error(eDLL_T::COMMON, 0, "%s: Windows Sockets API startup failure: (%s)\n",
			__FUNCTION__, NET_ErrorString(WSAGetLastError()));
	}
}

void Winsock_Shutdown()
{
	const int nError = ::WSACleanup();

	if (nError != 0)
	{
		Error(eDLL_T::COMMON, 0, "%s: Windows Sockets API shutdown failure: (%s)\n",
			__FUNCTION__, NET_ErrorString(WSAGetLastError()));
	}
}

void DirtySDK_Startup()
{
	const int32_t netConStartupRet = NetConnStartup("-servicename=sourcesdk");

	if (netConStartupRet < 0)
	{
		Error(eDLL_T::COMMON, 0, "%s: Network connection module startup failure: (%i)\n",
			__FUNCTION__, netConStartupRet);
	}
}

void DirtySDK_Shutdown()
{
	const int32_t netConShutdownRet = NetConnShutdown(0);

	if (netConShutdownRet < 0)
	{
		Error(eDLL_T::COMMON, 0, "%s: Network connection module shutdown failure: (%i)\n",
			__FUNCTION__, netConShutdownRet);
	}
}

void QuerySystemInfo()
{
#ifndef DEDICATED
	for (int i = 0; ; i++)
	{
		DISPLAY_DEVICE dd = { sizeof(dd), {0} };
		BOOL f = EnumDisplayDevices(NULL, i, &dd, EDD_GET_DEVICE_INTERFACE_NAME);
		if (!f)
		{
			break;
		}

		if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) // Only log the primary device.
		{
			Msg(eDLL_T::NONE, "%-25s: '%s'\n", "GPU model identifier", dd.DeviceString);
		}
	}
#endif // !DEDICATED

	const CPUInformation& pi = GetCPUInformation();

	Msg(eDLL_T::NONE, "%-25s: '%s'\n","CPU model identifier", pi.m_szProcessorBrand);
	Msg(eDLL_T::NONE, "%-25s: '%s'\n","CPU vendor tag", pi.m_szProcessorID);
	Msg(eDLL_T::NONE, "%-25s: '%12hhu' ('%2hhu' %s)\n", "CPU core count", pi.m_nPhysicalProcessors, pi.m_nLogicalProcessors, "logical");
	Msg(eDLL_T::NONE, "%-25s: '%12lld' ('%.1lf' %s)\n", "CPU core speed", pi.m_Speed, f64(pi.m_Speed / 1000000.0), "MHz");
	Msg(eDLL_T::NONE, "%-20s%s: '%12lu' ('0x%-8X')\n", "L1 cache", "(KiB)", pi.m_nL1CacheSizeKb, pi.m_nL1CacheDesc);
	Msg(eDLL_T::NONE, "%-20s%s: '%12lu' ('0x%-8X')\n", "L2 cache", "(KiB)", pi.m_nL2CacheSizeKb, pi.m_nL2CacheDesc);
	Msg(eDLL_T::NONE, "%-20s%s: '%12lu' ('0x%-8X')\n", "L3 cache", "(KiB)", pi.m_nL3CacheSizeKb, pi.m_nL3CacheDesc);

	MEMORYSTATUSEX statex{};
	statex.dwLength = sizeof(statex);

	if (GlobalMemoryStatusEx(&statex))
	{
		DWORDLONG totalPhysical = (statex.ullTotalPhys / 1024) / 1024;
		DWORDLONG totalVirtual = (statex.ullTotalVirtual / 1024) / 1024;

		DWORDLONG availPhysical = (statex.ullAvailPhys / 1024) / 1024;
		DWORDLONG availVirtual = (statex.ullAvailVirtual / 1024) / 1024;

		Msg(eDLL_T::NONE, "%-20s%s: '%12llu' ('%9llu' %s)\n", "Total system memory", "(MiB)", totalPhysical, totalVirtual, "virtual");
		Msg(eDLL_T::NONE, "%-20s%s: '%12llu' ('%9llu' %s)\n", "Avail system memory", "(MiB)", availPhysical, availVirtual, "virtual");
	}
	else
	{
		Error(eDLL_T::COMMON, NO_ERROR, "Unable to retrieve system memory information: %s\n",
			std::system_category().message(static_cast<int>(::GetLastError())).c_str());
	}
}

#if defined (DEDICATED)
#define SIGDB_FILE "cfg/server/startup.bin"
#elif defined (CLIENT_DLL)
#define SIGDB_FILE "cfg/client/startup.bin"
#else
#define SIGDB_FILE "cfg/startup.bin"
#endif

void DetourInit() // Run the sigscan
{
	const bool bNoSmap = CommandLine()->CheckParm("-nosmap") ? true : false;
	const bool bLogAdr = CommandLine()->CheckParm("-sig_toconsole") ? true : false;
	bool bInitDivider = false;

	g_SigCache.SetDisabled(bNoSmap);
	g_SigCache.ReadCache(SIGDB_FILE);

	// No debug logging in non dev builds.
	const bool bDevMode = !IsCert() && !IsRetail();

	for (const IDetour* pd : g_DetourVec)
	{
		pd->GetCon(); // Constants.
		pd->GetFun(); // Functions.
		pd->GetVar(); // Variables.

		if (bDevMode && bLogAdr)
		{
			if (!bInitDivider)
			{
				bInitDivider = true;
				spdlog::debug("+---------------------------------------------------------------------+\n");
			}
			pd->GetAdr();
			spdlog::debug("+---------------------------------------------------------------------+\n");
		}
	}

#ifdef DEDICATED
	// Must be performed after detour init as we patch instructions which alters the function signatures.
	Dedicated_Init();
#endif // DEDICATED

	g_SigCache.WriteCache(SIGDB_FILE);
	g_SigCache.InvalidateMap();
		}

void DetourAddress() // Test the sigscan results
{
	spdlog::debug("+---------------------------------------------------------------------+\n");
	for (const IDetour* pd : g_DetourVec)
	{
		pd->GetAdr();
		spdlog::debug("+---------------------------------------------------------------------+\n");
	}
}

void DetourRegister() // Register detour classes to be searched and hooked.
{
	// Tier0
	REGISTER(VPlatform);
	REGISTER(VJobThread);
	REGISTER(VThreadTools);
	REGISTER(VTSListBase);

	// Tier1
	REGISTER(VCommandLine);
	REGISTER(VCVar);

	// VPC
	REGISTER(VAppSystem);
	REGISTER(VKeyValues);
	REGISTER(VRSON);
	REGISTER(VFactory);

	// VstdLib
	REGISTER(VCallback);
	REGISTER(VCompletion);
	REGISTER(HKeyValuesSystem);

	// Common
	REGISTER(VOpcodes);
	REGISTER(V_NetMessages);

	// Launcher
	REGISTER(VPRX);
	REGISTER(VLauncher);

	REGISTER(VAppSystemGroup);

	// FileSystem
	REGISTER(VBaseFileSystem);
	REGISTER(VFileSystem_Stdio);

	// DataCache
	REGISTER(VMDLCache);

	// Ebisu
	REGISTER(VEbisuSDK);

#ifndef DEDICATED

	// Codecs
	REGISTER(BinkCore); // REGISTER CLIENT ONLY!
	REGISTER(MilesCore); // REGISTER CLIENT ONLY!
	REGISTER(MilesShim);
	REGISTER(VRadShal);

#endif // !DEDICATED

	// VPhysics
	REGISTER(VPhysicsCollide);
	REGISTER(VQHull);

	// StaticPropMgr
	REGISTER(VStaticPropMgr);

	// MaterialSystem
	REGISTER(VMaterialSystem);
#ifndef DEDICATED
	REGISTER(VMaterialGlue);
	REGISTER(VShaderGlue);

	// Studio
	REGISTER(VStudioRenderContext);

	// VGui
	REGISTER(VEngineVGui); // REGISTER CLIENT ONLY!
	REGISTER(VFPSPanel); // REGISTER CLIENT ONLY!
	REGISTER(VVGUIRichText); // REGISTER CLIENT ONLY!
	REGISTER(VMatSystemSurface);

	// Client
	REGISTER(HVEngineClient);
	REGISTER(VDll_Engine_Int);
	REGISTER(VClientDataBlockReceiver);
#endif // !DEDICATED

#ifndef CLIENT_DLL

	// Server
	REGISTER(VServer); // REGISTER SERVER ONLY!
	REGISTER(VPersistence); // REGISTER SERVER ONLY!
	REGISTER(HVEngineServer); // REGISTER SERVER ONLY!
	REGISTER(VServerDataBlockSender); // REGISTER SERVER ONLY!

#endif // !CLIENT_DLL

	// Engine/client
	REGISTER(VClient);
#ifndef DEDICATED
	REGISTER(VClientState);
	REGISTER(VCL_Main);
	REGISTER(VSplitScreen);
#endif // !DEDICATED

	// RTech
	REGISTER(V_ReSTD);

	REGISTER(V_AsyncIO);

	REGISTER(V_PakAlloc);
	REGISTER(V_PakParse);
	REGISTER(V_PakState);
	REGISTER(V_PakStream);

	REGISTER(VStryder);
	REGISTER(VPlaylists);

#ifndef DEDICATED
	REGISTER(V_Rui);
	REGISTER(V_CL_Ents_Parse); // REGISTER CLIENT ONLY!
#endif // !DEDICATED

	// Engine
	REGISTER(VCommon);

	REGISTER(VSys_Dll);
	REGISTER(VSys_Dll2);
	REGISTER(VSys_Utils);
	REGISTER(VEngine);
	REGISTER(VEngineTrace);
	REGISTER(VModelInfo);

	REGISTER(VTraceInit);
	REGISTER(VModel_BSP);
	REGISTER(VHost);
	REGISTER(VHostCmd);
	REGISTER(VHostState);
	REGISTER(VModelLoader);
	REGISTER(VCmd);
	REGISTER(VNet);
	REGISTER(VNetChan);
	REGISTER(VNetworkStringTableContainer);

	REGISTER(VLocalize);

#ifndef DEDICATED
	REGISTER(HVideoMode_Common);
	REGISTER(VGL_RMain);
	REGISTER(VMatSys_Interface);
	REGISTER(VGL_MatSysIFace);
	REGISTER(VGL_DrawLights);
	REGISTER(VGL_Screen);
#endif // !DEDICATED

	REGISTER(HSV_Main);

#ifndef DEDICATED
	REGISTER(VGame); // REGISTER CLIENT ONLY!
	REGISTER(VGL_RSurf);

	REGISTER(VDebugOverlay); // !TODO: This also needs to be exposed to server dll!!!
	REGISTER(VKeys);
#endif // !DEDICATED

	// VScript
	REGISTER(VSquirrel);
	REGISTER(VScript);
	REGISTER(VScriptShared);
#ifndef CLIENT_DLL
	REGISTER(VScriptServer);
#endif // !CLIENT_DLL
#ifndef DEDICATED
	REGISTER(VScriptClient);
#endif // !DEDICATED

	// Squirrel
	REGISTER(VSquirrelAPI);
	REGISTER(VSquirrelAUX);
	REGISTER(VSquirrelVM);

	// Game/shared
	REGISTER(VUserCmd);
	REGISTER(VAnimation);
	REGISTER(VUtil_Shared);

#ifndef CLIENT_DLL

	// In shared code, but weapon bolt is SERVER only.
	REGISTER(V_Weapon_Bolt);

	// Game/server
	REGISTER(VAI_Network);
	REGISTER(VAI_NetworkManager);
	REGISTER(VRecast);
	REGISTER(VServerGameDLL);
	REGISTER(VMoveHelperServer);
	REGISTER(VPhysics_Main); // REGISTER SERVER ONLY
	REGISTER(VBaseAnimating);
	REGISTER(VPlayer);
	REGISTER(VAI_BaseNPC);
	REGISTER(VPlayerMove);
	REGISTER(VServerEntityList);

#endif // !CLIENT_DLL

#ifndef DEDICATED
	REGISTER(V_ViewRender);
	REGISTER(VInput);
	REGISTER(V_GamePad);
	REGISTER(VMoveHelperClient);
	REGISTER(VClientEntityList);
	REGISTER(V_Player);
#endif // !DEDICATED

	// Public
	REGISTER(VEdict);

#ifndef DEDICATED
	REGISTER(VInputSystem);
	REGISTER(VInputStackSystem);
	REGISTER(VDXGI);
#endif // !DEDICATED
}

//-----------------------------------------------------------------------------
// Singleton accessors:
//-----------------------------------------------------------------------------
IKeyValuesSystem* KeyValuesSystem()
{
	return g_pKeyValuesSystem;
}
