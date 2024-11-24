#pragma once
#include "engine/gl_model_private.h"

inline void(*v_Host_RunFrame)(void* unused, float time);
inline void(*v_Host_RunFrame_Render)(void);
inline void(*v_Host_CountRealTimePackets)(void);
inline bool(*v_Host_ShouldRun)();
inline void(*v_Host_Error)(const char* error, ...);
//inline void(*v_VCR_EnterPausedState)(void);

inline bool* g_bAbortServerSet = nullptr;

inline jmp_buf* host_abortserver = nullptr;
inline bool* host_initialized = nullptr;
inline float* host_frametime_unbounded = nullptr;
inline float* host_frametime_stddeviation = nullptr;

void Host_Error(const char* const error, ...);

class CCommonHostState
{
public:
	CCommonHostState()
		: worldmodel(NULL)
		, worldbrush(NULL)
		, interval_per_tick(0.0f)
		, max_splitscreen_players(1)
		, max_splitscreen_players_clientdll(1)
	{}

	// cl_entitites[0].model
	model_t* worldmodel;
	struct worldbrushdata_t* worldbrush;
	// Tick interval for game
	float					interval_per_tick;
	// 1, unless a game supports split screen, then probably 2 or 4 (4 is the max allowable)
	int						max_splitscreen_players;
	// This is the # the client .dll thinks is the max, it might be > max_splitscreen_players in -tools mode, etc.
	int						max_splitscreen_players_clientdll;
	void					SetWorldModel(model_t* pModel);
};

extern CCommonHostState* g_pCommonHostState;

///////////////////////////////////////////////////////////////////////////////
class VHost : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("_Host_RunFrame", v_Host_RunFrame);
		LogFunAdr("_Host_RunFrame_Render", v_Host_RunFrame_Render);
		LogFunAdr("Host_CountRealTimePackets", v_Host_CountRealTimePackets);
		LogFunAdr("Host_ShouldRun", v_Host_ShouldRun);
		LogFunAdr("Host_Error", v_Host_Error);
		//LogFunAdr("VCR_EnterPausedState", v_VCR_EnterPausedState);
		LogVarAdr("g_CommonHostState", g_pCommonHostState);
		LogVarAdr("g_bAbortServerSet", g_bAbortServerSet);
		LogVarAdr("host_abortserver", host_abortserver);
		LogVarAdr("host_initialized", host_initialized);
		LogVarAdr("host_frametime_unbounded", host_frametime_unbounded);
		LogVarAdr("host_frametime_stddeviation", host_frametime_stddeviation);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 8B C4 48 89 58 18 48 89 70 20 F3 0F 11 48 ??").GetPtr(v_Host_RunFrame);
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 8B 0D ?? ?? ?? ?? 48 85 C9 75 34").GetPtr(v_Host_RunFrame_Render);
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 30 65 48 8B 04 25 ?? ?? ?? ?? 33 DB").GetPtr(v_Host_CountRealTimePackets);
		g_GameDll.FindPatternSIMD("48 83 EC 28 48 8B 05 ?? ?? ?? ?? 83 78 6C 00 75 07 B0 01").GetPtr(v_Host_ShouldRun);
		g_GameDll.FindPatternSIMD("48 89 4C 24 ?? 48 89 54 24 ?? 4C 89 44 24 ?? 4C 89 4C 24 ?? 53 57 48 81 EC ?? ?? ?? ??").GetPtr(v_Host_Error);
		//g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 65 48 8B 04 25 ?? ?? ?? ?? BB ?? ?? ?? ?? C6 05 ?? ?? ?? ?? ??").GetPtr(v_VCR_EnterPausedState);
	}
	virtual void GetVar(void) const
	{
		g_pCommonHostState = g_GameDll.FindPatternSIMD("48 83 EC 28 84 C9 75 0B")
			.FindPatternSelf("48 8B 15").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CCommonHostState*>();

		const CMemory hostErrorBase(v_Host_Error);

		g_bAbortServerSet = hostErrorBase.FindPattern("40 38 3D", CMemory::Direction::DOWN, 512, 4).ResolveRelativeAddress(3, 7).RCast<bool*>();
		host_abortserver = hostErrorBase.FindPattern("48 8D 0D", CMemory::Direction::DOWN, 512, 5).ResolveRelativeAddress(3, 7).RCast<jmp_buf*>();

		static const int n_host_initialized_search_offset = 0x500;
		static const int n_host_frametime_unbounded_search_offset = 0x330;
		static const int n_host_frametime_stddeviation_search_offset = 0xFAA;

		const CMemory hostRunFrameBase(v_Host_RunFrame);

		host_initialized = hostRunFrameBase.Offset(n_host_initialized_search_offset).FindPatternSelf("44 38").ResolveRelativeAddressSelf(0x3, 0x7).RCast<bool*>();
		host_frametime_unbounded = hostRunFrameBase.Offset(n_host_frametime_unbounded_search_offset).FindPatternSelf("F3 0F 11").ResolveRelativeAddressSelf(0x4, 0x8).RCast<float*>();
		host_frametime_stddeviation = hostRunFrameBase.Offset(n_host_frametime_stddeviation_search_offset).FindPatternSelf("F3 0F 11").ResolveRelativeAddressSelf(0x4, 0x8).RCast<float*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
