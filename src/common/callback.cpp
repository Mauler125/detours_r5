//=============================================================================//
//
// Purpose: Callback functions for ConVar's.
//
//=============================================================================//

#include "core/stdafx.h"
#include "core/init.h"
#include "windows/id3dx.h"
#include "tier0/fasttimer.h"
#include "tier1/cvar.h"
#include "tier1/fmtstr.h"
#include "engine/shared/shared_rcon.h"
#ifndef CLIENT_DLL
#include "engine/server/sv_rcon.h"
#include "engine/server/server.h"
#endif // !CLIENT_DLL
#ifndef DEDICATED
#include "engine/gl_screen.h"
#include "engine/client/cl_rcon.h"
#include "engine/client/cdll_engine_int.h"
#include "engine/client/clientstate.h"
#endif // !DEDICATED
#include "engine/client/client.h"
#include "engine/net.h"
#include "engine/cmd.h"
#include "engine/host_cmd.h"
#include "engine/host_state.h"
#include "engine/enginetrace.h"

#include "rtech/pak/pakencode.h"
#include "rtech/pak/pakdecode.h"
#include "rtech/pak/pakparse.h"
#include "rtech/pak/pakstate.h"
#include "rtech/pak/paktools.h"

#include "rtech/playlists/playlists.h"

#include "filesystem/basefilesystem.h"
#include "filesystem/filesystem.h"
#include "vpklib/packedstore.h"
#include "vscript/vscript.h"
#include "localize/localize.h"
#include "ebisusdk/EbisuSDK.h"
#ifndef DEDICATED
#include "geforce/reflex.h"
#include "gameui/IBrowser.h"
#include "gameui/IConsole.h"
#endif // !DEDICATED
#ifndef CLIENT_DLL
#include "networksystem/bansystem.h"
#endif // !CLIENT_DLL
#include "public/edict.h"
#include "public/worldsize.h"
#include "mathlib/crc32.h"
#include "mathlib/mathlib.h"
#include "common/completion.h"
#include "common/callback.h"
#ifndef DEDICATED
#include "materialsystem/cmaterialglue.h"
#endif // !DEDICATED
#include "public/bspflags.h"
#include "public/cmodel.h"
#include "public/idebugoverlay.h"
#include "public/localize/ilocalize.h"
#ifndef CLIENT_DLL
#include "game/server/detour_impl.h"
#include "game/server/gameinterface.h"
#endif // !CLIENT_DLL
#ifndef DEDICATED
#include "game/client/cliententitylist.h"
#include "game/client/viewrender.h"
#endif // !DEDICATED
#include "vscript/languages/squirrel_re/vsquirrel.h"


/*
=====================
MP_GameMode_Changed_f
=====================
*/
void MP_GameMode_Changed_f(IConVar* pConVar, const char* pOldString, float flOldValue, ChangeUserData_t pUserData)
{
	v_SetupGamemode(mp_gamemode->GetString());
}

#ifndef CLIENT_DLL
/*
=====================
Host_Changelevel_f

  Goes to a new map, 
  taking all clients along
=====================
*/
void Host_Changelevel_f(const CCommand& args)
{
	const int argCount = args.ArgC();

	if (argCount >= 2
		&& IsOriginInitialized()
		&& g_pServer->IsActive())
	{
		const char* levelName = args[1];
		const char* landMarkName = argCount > 2 ? args[2] : "";

		v_SetLaunchOptions(args);
		v_HostState_ChangeLevelMP(levelName, landMarkName);
	}
}
#endif // !CLIENT_DLL

// TODO: move this to 'packedstore.cpp' and move everything in that file to 'packetstorebuilder.cpp'
static ConVar fs_packedstore_workspace("fs_packedstore_workspace", "ship", FCVAR_DEVELOPMENTONLY, "Determines the current VPK workspace.");
static ConVar fs_packedstore_compression_level("fs_packedstore_compression_level", "default", FCVAR_DEVELOPMENTONLY, "Determines the VPK compression level.", "fastest faster default better uber");
static ConVar fs_packedstore_max_helper_threads("fs_packedstore_max_helper_threads", "-1", FCVAR_DEVELOPMENTONLY, "Max # of additional \"helper\" threads to create during compression.", true, -1, true, LZHAM_MAX_HELPER_THREADS, "Must range between [-1,LZHAM_MAX_HELPER_THREADS], where -1=max practical");

/*
=====================
VPK_Pack_f

  Packs VPK files into
  'PLATFORM' VPK directory.
=====================
*/
void VPK_Pack_f(const CCommand& args)
{
	if (args.ArgC() < 4)
	{
		return;
	}

	const char* workspacePath = fs_packedstore_workspace.GetString();

	if (!FileSystem()->IsDirectory(workspacePath, "PLATFORM"))
	{
		Error(eDLL_T::FS, NO_ERROR, "Workspace path \"%s\" doesn't exist!\n", workspacePath);
		return;
	}

	VPKPair_t pair(args.Arg(1), args.Arg(2), args.Arg(3), NULL);
	Msg(eDLL_T::FS, "*** Starting VPK build command for: '%s'\n", pair.m_DirName.String());

	CFastTimer timer;
	timer.Start();

	CPackedStoreBuilder builder;

	builder.InitLzEncoder(fs_packedstore_max_helper_threads.GetInt(), fs_packedstore_compression_level.GetString());
	builder.PackStore(pair, workspacePath, "vpk/");

	timer.End();
	Msg(eDLL_T::FS, "*** Time elapsed: '%lf' seconds\n", timer.GetDuration().GetSeconds());
	Msg(eDLL_T::FS, "\n");
}

/*
=====================
VPK_Unpack_f

  Unpacks VPK files into
  workspace directory.
=====================
*/
void VPK_Unpack_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	CUtlString fileName = args.Arg(1);
	VPKDir_t vpk(fileName, (args.ArgC() > 2));

	if (vpk.Failed())
	{
		Error(eDLL_T::FS, NO_ERROR, "Failed to parse directory tree file \"%s\"!\n", fileName.String());
		return;
	}

	Msg(eDLL_T::FS, "*** Starting VPK extraction command for: '%s'\n", fileName.String());

	CFastTimer timer;
	timer.Start();

	CPackedStoreBuilder builder;

	builder.InitLzDecoder();
	builder.UnpackStore(vpk, fs_packedstore_workspace.GetString());

	timer.End();
	Msg(eDLL_T::FS, "*** Time elapsed: '%lf' seconds\n", timer.GetDuration().GetSeconds());
	Msg(eDLL_T::FS, "\n");
}

/*
=====================
VPK_Mount_f

  Mounts input VPK file for
  internal FileSystem usage
=====================
*/
void VPK_Mount_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	FileSystem()->MountVPKFile(args.Arg(1));
}

/*
=====================
VPK_Unmount_f

  Unmounts input VPK file
  and clears its cache
=====================
*/
void VPK_Unmount_f(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		return;
	}

	FileSystem()->UnmountVPKFile(args.Arg(1));
}

void LanguageChanged_f(IConVar* pConVar, const char* pOldString, float flOldValue, ChangeUserData_t pUserData)
{
	const char* pNewString = language_cvar->GetString();

	if (strcmp(pOldString, pNewString) == NULL)
		return; // Same language.

	if (!Localize_IsLanguageSupported(pNewString))
	{
		// if new text isn't valid but the old value is, reset the value
		if (Localize_IsLanguageSupported(pOldString))
			pNewString = pOldString;
		else
		{
			// this shouldn't really happen, but if neither the old nor new values are valid, set to english
			Assert(0);
			pNewString = g_LanguageNames[0];
		}

		language_cvar->SetValue(pNewString);
	}
}

#ifndef DEDICATED
void setClassVarClient_f(const CCommand& args)
{
	v__setClassVarClient_f(args);
}

static ConCommand _setClassVarClient("_setClassVarClient", setClassVarClient_f, "Set a class var on the client", FCVAR_DEVELOPMENTONLY|FCVAR_CLIENTDLL|FCVAR_CHEAT|FCVAR_SERVER_CAN_EXECUTE);

/*
=====================
Set_f

  Set a class var on the server & client.
=====================
*/
void Set_f(const CCommand& args)
{
	v__setClassVarClient_f(args);

	const char* key = args.Arg(1);
	const char* val = args.Arg(2);

	char buf[256];
	V_snprintf(buf, sizeof(buf), "_setClassVarServer %s \"%s\"\n", key, val);

	Cbuf_AddText(ECommandTarget_t::CBUF_FIRST_PLAYER, buf, cmd_source_t::kCommandSrcInvalid);
}

/*
=====================
Mat_CrossHair_f

  Print the material under the crosshair.
=====================
*/
void Mat_CrossHair_f(const CCommand& args)
{
	CMaterialGlue* material = v_GetMaterialAtCrossHair();
	if (material)
	{
		Msg(eDLL_T::MS, "______________________________________________________________\n");
		Msg(eDLL_T::MS, "-+ Material --------------------------------------------------\n");
		Msg(eDLL_T::MS, " |-- ADDR: '%llX'\n", material);
		Msg(eDLL_T::MS, " |-- GUID: '%llX'\n", material->assetGuid);
		Msg(eDLL_T::MS, " |-- Num Streaming Textures: '%d'\n", material->numStreamingTextureHandles);
		Msg(eDLL_T::MS, " |-- Material width: '%d'\n", material->width);
		Msg(eDLL_T::MS, " |-- Material height: '%d'\n", material->height);
		Msg(eDLL_T::MS, " |-- Samplers: '%08X'\n", material->samplers);

		std::function<void(CMaterialGlue*, const char*)> fnPrintChild = [](CMaterialGlue* material, const char* print)
		{
			Msg(eDLL_T::MS, " |-+\n");
			Msg(eDLL_T::MS, " | |-+ Child material ----------------------------------------\n");
			Msg(eDLL_T::MS, print, material);
			Msg(eDLL_T::MS, " |     |-- GUID: '%llX'\n", material->assetGuid);
			Msg(eDLL_T::MS, " |     |-- Material name: '%s'\n", material->name);
		};

		Msg(eDLL_T::MS, " |-- Material name: '%s'\n", material->name);
		Msg(eDLL_T::MS, " |-- Material surface name 1: '%s'\n", material->surfaceProp);
		Msg(eDLL_T::MS, " |-- Material surface name 2: '%s'\n", material->surfaceProp2);
		Msg(eDLL_T::MS, " |-- DX buffer: '%llX'\n", material->dxBuffer);
		Msg(eDLL_T::MS, " |-- DX buffer VFTable: '%llX'\n", material->unkD3DPointer);

		material->depthShadowMaterial
			? fnPrintChild(material->depthShadowMaterial, " |   |-+ DepthShadow: '%llX'\n")
			: Msg(eDLL_T::MS, " |   |-+ DepthShadow: 'NULL'\n");
		material->depthPrepassMaterial
			? fnPrintChild(material->depthPrepassMaterial, " |   |-+ DepthPrepass: '%llX'\n")
			: Msg(eDLL_T::MS, " |   |-+ DepthPrepass: 'NULL'\n");
		material->depthVSMMaterial
			? fnPrintChild(material->depthVSMMaterial, " |   |-+ DepthVSM: '%llX'\n")
			: Msg(eDLL_T::MS, " |   |-+ DepthVSM: 'NULL'\n");
		material->depthShadowTightMaterial
			? fnPrintChild(material->depthShadowTightMaterial, " |   |-+ DepthShadowTight: '%llX'\n")
			: Msg(eDLL_T::MS, " |   |-+ DepthShadowTight: 'NULL'\n");
		material->colpassMaterial
			? fnPrintChild(material->colpassMaterial, " |   |-+ ColPass: '%llX'\n")
			: Msg(eDLL_T::MS, " |   |-+ ColPass: 'NULL'\n");

		Msg(eDLL_T::MS, "-+ Texture GUID map ------------------------------------------\n");
		Msg(eDLL_T::MS, " |-- Texture handles: '%llX'\n", material->textureHandles);
		Msg(eDLL_T::MS, " |-- Streaming texture handles: '%llX'\n", material->streamingTextureHandles);

		Msg(eDLL_T::MS, "--------------------------------------------------------------\n");
	}
	else
	{
		Msg(eDLL_T::MS, "%s: No material found >:(\n", __FUNCTION__);
	}
}

/*
=====================
Line_f

  Draws a line at 
  start<x1 y1 z1> end<x2 y2 z2>.
=====================
*/
void Line_f(const CCommand& args)
{
	if (args.ArgC() != 7)
	{
		Msg(eDLL_T::CLIENT, "Usage 'line': start(vector) end(vector)\n");
		return;
	}

	Vector3D start, end;
	for (int i = 0; i < 3; ++i)
	{
		start[i] = float(atof(args[i + 1]));
		end[i] = float(atof(args[i + 4]));
	}

	g_pDebugOverlay->AddLineOverlay(start, end, 255, 255, 0, !r_debug_draw_depth_test.GetBool(), 100);
}

/*
=====================
Sphere_f

  Draws a sphere at origin(x1 y1 z1) 
  radius(float) theta(int) phi(int).
=====================
*/
void Sphere_f(const CCommand& args)
{
	if (args.ArgC() != 7)
	{
		Msg(eDLL_T::CLIENT, "Usage 'sphere': origin(vector) radius(float) theta(int) phi(int)\n");
		return;
	}

	Vector3D start;
	for (int i = 0; i < 3; ++i)
	{
		start[i] = float(atof(args[i + 1]));
	}

	float radius = float(atof(args[4]));
	int theta = atoi(args[5]);
	int phi = atoi(args[6]);

	g_pDebugOverlay->AddSphereOverlay(start, radius, theta, phi, 20, 210, 255, 0, 100);
}

/*
=====================
Capsule_f

  Draws a capsule at start<x1 y1 z1> 
  end<x2 y2 z2> radius <x3 y3 z3>.
=====================
*/
void Capsule_f(const CCommand& args)
{
	if (args.ArgC() != 10)
	{
		Msg(eDLL_T::CLIENT, "Usage 'capsule': start(vector) end(vector) radius(vector)\n");
		return;
	}

	Vector3D start, end, radius;
	for (int i = 0; i < 3; ++i)
	{
		start[i] = float(atof(args[i + 1]));
		end[i] = float(atof(args[i + 4]));
		radius[i] = float(atof(args[i + 7]));
	}
	g_pDebugOverlay->AddCapsuleOverlay(start, end, radius, { 0,0,0 }, { 0,0,0 }, 141, 233, 135, 0, 100);
}
#endif // !DEDICATED

// TODO: move to other file?
static ConVar bhit_depth_test("bhit_depth_test", "0", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Use depth test for bullet ray trace overlay");
static ConVar bhit_abs_origin("bhit_abs_origin", "1", FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED, "Draw entity's predicted abs origin upon bullet impact for trajectory debugging (requires 'r_visualizetraces' to be set!)");
/*
=====================
BHit_f

  Bullet trajectory tracing
  from shooter to target entity.
=====================
*/
void BHit_f(const CCommand& args)
{
#ifndef CLIENT_DLL // Stubbed to suppress server warnings as this is a GAMEDLL command!
	if (args.ArgC() != 9)
		return;

	if (!bhit_enable->GetBool())
		return;

	if (sv_visualizetraces->GetBool())
	{
		Vector3D vecAbsStart;
		Vector3D vecAbsEnd;

		for (int i = 0; i < 3; ++i)
			vecAbsStart[i] = float(atof(args[i + 4]));

		QAngle vecBulletAngles;
		for (int i = 0; i < 2; ++i)
			vecBulletAngles[i] = float(atof(args[i + 7]));

		vecBulletAngles.z = 180.f; // Flipped axis.
		AngleVectors(vecBulletAngles, &vecAbsEnd);

		vecAbsEnd.MulAdd(vecAbsStart, vecAbsEnd, MAX_COORD_RANGE);

		Ray_t ray(vecAbsStart, vecAbsEnd);
		trace_t trace;

		g_pEngineTraceServer->TraceRay(ray, TRACE_MASK_NPCWORLDSTATIC, &trace);

		g_pDebugOverlay->AddLineOverlay(trace.startpos, trace.endpos, 0, 255, 0, !bhit_depth_test.GetBool(), sv_visualizetraces_duration->GetFloat());
		g_pDebugOverlay->AddLineOverlay(trace.endpos, vecAbsEnd, 255, 0, 0, !bhit_depth_test.GetBool(), sv_visualizetraces_duration->GetFloat());
	}
#endif // !CLIENT_DLL

#ifndef DEDICATED
	if (bhit_abs_origin.GetBool() && r_visualizetraces->GetBool())
	{
		const int iEnt = atoi(args[2]);
		if (const IClientEntity* pEntity = g_pClientEntityList->GetClientEntity(iEnt))
		{
			g_pDebugOverlay->AddSphereOverlay( // Render a debug sphere at the client's predicted entity origin.
				pEntity->GetAbsOrigin(), 10.f, 8, 6, 20, 60, 255, 0, r_visualizetraces_duration->GetFloat());
		}
	}
#endif // !DEDICATED
}

/*
=====================
CVHelp_f

  Show help text for a
  particular convar/concommand
=====================
*/
void CVHelp_f(const CCommand& args)
{
	cv->CvarHelp(args);
}

/*
=====================
CVList_f

  List all ConCommandBases
=====================
*/
void CVList_f(const CCommand& args)
{
	cv->CvarList(args);
}

/*
=====================
CVDiff_f

  List all ConVar's 
  who's values deviate 
  from default value
=====================
*/
void CVDiff_f(const CCommand& args)
{
	cv->CvarDifferences(args);
}

/*
=====================
CVFlag_f

  List all ConVar's
  with specified flags
=====================
*/
void CVFlag_f(const CCommand& args)
{
	cv->CvarFindFlags_f(args);
}

#ifndef DEDICATED
static double s_flScriptExecTimeBase = 0.0f;
static int s_nScriptExecCount = 0;
#endif // !DEDICATED
/*
=====================
Cmd_Exec_f

  executes a cfg file
=====================
*/
#ifndef DEDICATED
static ConVar sv_quota_scriptExecsPerSecond("sv_quota_scriptExecsPerSecond", "3", FCVAR_REPLICATED | FCVAR_RELEASE,
	"How many script executions per second clients are allowed to submit, 0 to disable the limitation thereof.", true, 0.f, false, 0.f);
#endif // !DEDICATED

void Cmd_Exec_f(const CCommand& args)
{
#ifndef DEDICATED
	// Prevent users from running neo strafe commands and other quick hacks.
	// TODO: when reBar becomes a thing, we should verify this function and
	// flag users that patch them out.
	if (g_pClientState->IsActive() && !ThreadInServerFrameThread())
	{
		const int execQuota = sv_quota_scriptExecsPerSecond.GetInt();

		if (execQuota > 0)
		{
			const double flCurrentTime = Plat_FloatTime();

			// Reset every second.
			if ((flCurrentTime - s_flScriptExecTimeBase) > 1.0)
			{
				s_flScriptExecTimeBase = flCurrentTime;
				s_nScriptExecCount = 0;
			}

			if (s_nScriptExecCount >= execQuota)
			{
				DevWarning(eDLL_T::ENGINE, "Client is simulating and exec count = %d of %d; dropped exec command: %s\n",
					s_nScriptExecCount, execQuota, args.ArgS());

				return;
			}

			s_nScriptExecCount++;
		}
	}
#endif // !DEDICATED
	v__Cmd_Exec_f(args);
}

#ifndef DEDICATED
void UIScript_Reset_f()
{
	// NOTE: function 'Script_InitUIVM' hasn't been called yet; nothing to reset,
	// prevent crashing during init code.
	if (!*g_bUIScriptInitialized)
		return;

	// NOTE: don't allow the execution of this command in loading screens, doing
	// so will hard crash the engine!
	if (*scr_drawloading)
		return;

	v__UIScript_Reset_f();
}
#endif // !DEDICATED

void VCallback::Detour(const bool bAttach) const
{
	DetourSetup(&v__Cmd_Exec_f, &Cmd_Exec_f, bAttach);
#ifndef DEDICATED
	DetourSetup(&v__UIScript_Reset_f, &UIScript_Reset_f, bAttach);
#endif // !DEDICATED
}
