/******************************************************************************
-------------------------------------------------------------------------------
File   : IStreamOverlay.cpp
Date   : 08:01:2025
Author : Kawe Mazidjatari
Purpose: Implements the in-game texture streaming debug overlay
-------------------------------------------------------------------------------
History:
- 08:01:2025 | 19:05 : Created by Kawe Mazidjatari

******************************************************************************/

#include "windows/id3dx.h"
#include "materialsystem/texturestreaming.h"
#include "IStreamOverlay.h"

//-----------------------------------------------------------------------------
// Console variables
//-----------------------------------------------------------------------------
static ConVar stream_overlay_memory("stream_overlay_memory", "524288", FCVAR_DEVELOPMENTONLY, "Total string memory to allocate for the texture streaming debug overlay.", true, 1.f, false, 0.0f);

//-----------------------------------------------------------------------------
// Console commands
//-----------------------------------------------------------------------------
static ConCommand stream_dumpinfo("stream_dumpinfo", CStreamOverlay::DumpStreamInfo_f, "Dump texture streaming debug info to the console", FCVAR_DEVELOPMENTONLY, nullptr, "tex mtl bsp short");

//-----------------------------------------------------------------------------
// Purpose: constructor/destructor
//-----------------------------------------------------------------------------
CStreamOverlay::CStreamOverlay(void)
{
	m_surfaceLabel = "Stream Overlay";
	m_scratchBuffer = nullptr;
	m_scratchBufferSize = 0;
	m_lastAvailability = false;
}
CStreamOverlay::~CStreamOverlay(void)
{
	Shutdown();
}

//-----------------------------------------------------------------------------
// Purpose: stream overlay initialization
//-----------------------------------------------------------------------------
bool CStreamOverlay::Init(void)
{
	SetStyleVar(1200, 524, -1000, 50);

	m_initialized = true;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: stream overlay shutdown
//-----------------------------------------------------------------------------
void CStreamOverlay::Shutdown(void)
{
	FreeScratchBuffer();
	m_initialized = false;
}

//-----------------------------------------------------------------------------
// Purpose: check value of stream_overlay and determine availability of window
//-----------------------------------------------------------------------------
void CStreamOverlay::UpdateWindowAvailability(void)
{
	const bool enabled = stream_overlay->GetBool();

	if (enabled == m_lastAvailability)
		return;

	if (!enabled && m_activated)
	{
		m_activated = false;
		ResetInput();
	}

	else if (enabled && !m_activated)
		m_activated = true;

	m_lastAvailability = enabled;
}

//-----------------------------------------------------------------------------
// Purpose: run stream overlay frame
//-----------------------------------------------------------------------------
void CStreamOverlay::RunFrame(void)
{
	if (!m_initialized)
		Init();

	Animate();

	int baseWindowStyleVars = 0;
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_fadeAlpha); baseWindowStyleVars++;

	const bool drawn = DrawSurface();
	ImGui::PopStyleVar(baseWindowStyleVars);

	if (!drawn)
		FreeScratchBuffer();

	UpdateWindowAvailability();
}

//-----------------------------------------------------------------------------
// Purpose: syncs the cvar and updates the availability of mouse/key inputs
//-----------------------------------------------------------------------------
static void StreamOverlay_HandleClose(void)
{
	stream_overlay->SetValue(false);
	ResetInput();
}

//-----------------------------------------------------------------------------
// Purpose: draw stream overlay
//-----------------------------------------------------------------------------
bool CStreamOverlay::DrawSurface(void)
{
	if (!IsVisible())
		return false;

	if (!ImGui::Begin(m_surfaceLabel, &m_activated, ImGuiWindowFlags_None, &StreamOverlay_HandleClose))
	{
		ImGui::End();
		return false;
	}

	if (ImGui::BeginChild("##StreamReport", ImVec2(-1, -1), ImGuiChildFlags_Border, ImGuiWindowFlags_None))
	{
		ResizeScratchBuffer(stream_overlay_memory.GetInt());

		TextureStreamMgr_GetStreamOverlay(stream_overlay_mode->GetString(), m_scratchBuffer, m_scratchBufferSize);
		ImGui::TextUnformatted(m_scratchBuffer);
	}

	ImGui::EndChild();
	ImGui::End();

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: dynamically scale the scratch buffer if it became smaller or larger
//-----------------------------------------------------------------------------
void CStreamOverlay::ResizeScratchBuffer(const size_t newSize)
{
	Assert(newSize > 0);

	if (newSize == m_scratchBufferSize)
		return; // Same size.

	if (m_scratchBuffer)
		delete[] m_scratchBuffer;

	m_scratchBuffer = new char[newSize];
	m_scratchBufferSize = newSize;
}

//-----------------------------------------------------------------------------
// Purpose: free the scratch buffer if we have one
//-----------------------------------------------------------------------------
void CStreamOverlay::FreeScratchBuffer(void)
{
	if (m_scratchBuffer)
	{
		delete[] m_scratchBuffer;

		m_scratchBuffer = nullptr;
		m_scratchBufferSize = 0;
	}
	else
		Assert(m_scratchBufferSize == 0);
}

//-----------------------------------------------------------------------------
// Purpose: render current streaming data to console with given or default mode
//-----------------------------------------------------------------------------
void CStreamOverlay::RenderToConsole(const char* const mode)
{
	const bool isTemp = m_scratchBuffer == nullptr;

	// If we have a buffer already, use that to render the overlay report into
	// it. Else create a temporary buffer and free it afterwards.
	if (isTemp)
	{
		const size_t targetBufLen = stream_overlay_memory.GetInt();

		m_scratchBuffer = new char[targetBufLen];
		m_scratchBufferSize = targetBufLen;
	}

	TextureStreamMgr_GetStreamOverlay(mode ? mode : stream_overlay_mode->GetString(), m_scratchBuffer, m_scratchBufferSize);
	Msg(eDLL_T::MS, "%s\n", m_scratchBuffer);

	if (isTemp)
	{
		delete[] m_scratchBuffer;

		m_scratchBuffer = nullptr;
		m_scratchBufferSize = 0;
	}
}

CStreamOverlay g_streamOverlay;

/*
=====================
DumpStreamInfo_f

  Dumps the stream info to the console.
=====================
*/
void CStreamOverlay::DumpStreamInfo_f(const CCommand& args)
{
	const char* const mode = args.ArgC() >= 2 ? args.Arg(1) : nullptr;
	g_streamOverlay.RenderToConsole(mode);
}
