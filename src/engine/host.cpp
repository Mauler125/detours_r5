//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/frametask.h"
#include "engine/host.h"
#ifndef DEDICATED
#include "windows/id3dx.h"
#include "geforce/reflex.h"
#include "vgui/vgui_debugpanel.h"
#include <materialsystem/cmaterialsystem.h>
#endif // !DEDICATED

CCommonHostState* g_pCommonHostState = nullptr;

void CCommonHostState::SetWorldModel(model_t* pModel)
{
	if (worldmodel == pModel)
		return;

	worldmodel = pModel;
	if (pModel)
	{
		worldbrush = pModel->brush.pShared;
	}
	else
	{
		worldbrush = NULL;
	}
}

/*
==================
Host_CountRealTimePackets

Counts the number of
packets in non-prescaled
clock frames (does not
count for bots or Terminal
Services environments)
==================
*/
void Host_CountRealTimePackets()
{
	v_Host_CountRealTimePackets();
#ifndef DEDICATED
	GeForce_SetLatencyMarker(D3D11Device(), SIMULATION_START, MaterialSystem()->GetCurrentFrameCount());
#endif // !DEDICATED
}

/*
==================
_Host_RunFrame

Runs all active servers
==================
*/
void _Host_RunFrame(void* unused, float time)
{
	for (IFrameTask* const& task : g_TaskQueueList)
	{
		task->RunFrame();
	}

	g_TaskQueueList.erase(std::remove_if(g_TaskQueueList.begin(), g_TaskQueueList.end(), [](const IFrameTask* task)
		{
			return task->IsFinished();
		}), g_TaskQueueList.end());

#ifndef DEDICATED
	g_TextOverlay.ShouldDraw(time);
#endif // !DEDICATED

	return v_Host_RunFrame(unused, time);
}

void Host_Error(const char* const error, ...)
{
	char buf[1024];
	{/////////////////////////////
		va_list args{};
		va_start(args, error);

		vsnprintf(buf, sizeof(buf), error, args);

		buf[sizeof(buf) - 1] = '\0';
		va_end(args);
	}/////////////////////////////

	Error(eDLL_T::ENGINE, NO_ERROR, "Host_Error: %s", buf);
	v_Host_Error(buf);
}

///////////////////////////////////////////////////////////////////////////////
void VHost::Detour(const bool bAttach) const
{
	DetourSetup(&v_Host_RunFrame, &_Host_RunFrame, bAttach);
	DetourSetup(&v_Host_CountRealTimePackets, &Host_CountRealTimePackets, bAttach);

#ifndef DEDICATED // Dedicated already logs this!
	DetourSetup(&v_Host_Error, &Host_Error, bAttach);
#endif // !DEDICATED
}
