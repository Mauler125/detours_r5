//=============================================================================//
// 
// Purpose: Dear ImGui engine implementation
// 
//=============================================================================//

#include "imgui/misc/imgui_snapshot.h"
#include "engine/sys_mainwind.h"
#include "windows/id3dx.h"

#include "IBrowser.h"
#include "IConsole.h"

#include "imgui_system.h"

//-----------------------------------------------------------------------------
// Constructors/Destructors.
//-----------------------------------------------------------------------------
CImguiSystem::CImguiSystem()
{
	m_systemInitState = ImguiSystemInitStage_e::IM_PENDING_INIT;
}

//-----------------------------------------------------------------------------
// Initializes the imgui system. If this fails, false would be returned and the
// implementation won't run.
//-----------------------------------------------------------------------------
bool CImguiSystem::Init()
{
	Assert(ThreadInMainThread(), "CImguiSystem::Init() should only be called from the main thread!");
	Assert(m_systemInitState == ImguiSystemInitStage_e::IM_PENDING_INIT, "CImguiSystem::Init() called recursively?");

	///////////////////////////////////////////////////////////////////////////
	IMGUI_CHECKVERSION();
	ImGuiContext* const context = ImGui::CreateContext();

	if (!context)
		return false;

	AUTO_LOCK(m_snapshotBufferMutex);
	AUTO_LOCK(m_inputEventQueueMutex);

	// This is required to disable the ctrl+tab menu as some users use this
	// shortcut for other things in-game. See: https://github.com/ocornut/imgui/issues/4828
	context->ConfigNavWindowingKeyNext = 0;
	context->ConfigNavWindowingKeyPrev = 0;

	ImGuiViewport* const vp = ImGui::GetMainViewport();
	vp->PlatformHandleRaw = g_pGame->GetWindow();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_IsSRGB;

	if (!ImGui_ImplWin32_Init(g_pGame->GetWindow()) || 
		!ImGui_ImplDX11_Init(D3D11Device(), D3D11DeviceContext()))
	{
		Assert(0);

		m_systemInitState = ImguiSystemInitStage_e::IM_INIT_FAILURE;
		return false;
	}

	m_systemInitState = ImguiSystemInitStage_e::IM_SYSTEM_INIT;
	return true;
}

//-----------------------------------------------------------------------------
// Shuts the imgui system down, frees all allocated buffers.
//-----------------------------------------------------------------------------
void CImguiSystem::Shutdown()
{
	Assert(ThreadInMainThread(), "CImguiSystem::Shutdown() should only be called from the main thread!");
	Assert(m_systemInitState != ImguiSystemInitStage_e::IM_PENDING_INIT, "CImguiSystem::Shutdown() called recursively?");

	// Nothing to shutdown.
	if (m_systemInitState == ImguiSystemInitStage_e::IM_PENDING_INIT)
		return;

	AUTO_LOCK(m_snapshotBufferMutex);
	AUTO_LOCK(m_inputEventQueueMutex);

	m_systemInitState = ImguiSystemInitStage_e::IM_PENDING_INIT;

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();

	ImGui::DestroyContext();

	m_snapshotData.Clear();
}

//-----------------------------------------------------------------------------
// Copies currently drawn data into the snapshot buffer which is queued to be
// rendered in the render thread. This should only be called from the same
// thread SampleFrame() is being called from.
//-----------------------------------------------------------------------------
void CImguiSystem::SwapBuffers()
{
	Assert(ThreadInMainThread(), "CImguiSystem::SwapBuffers() should only be called from the main thread!");

	if (m_systemInitState < ImguiSystemInitStage_e::IM_FRAME_SAMPLED)
		return;

	AUTO_LOCK(m_snapshotBufferMutex);
	ImDrawData* const drawData = ImGui::GetDrawData();

	// Nothing has been drawn, nothing to swap
	if (!drawData)
		return;

	m_snapshotData.SnapUsingSwap(drawData, ImGui::GetTime());

	if (m_systemInitState == ImguiSystemInitStage_e::IM_FRAME_SAMPLED)
		m_systemInitState = ImguiSystemInitStage_e::IM_FRAME_SWAPPED;
}

//-----------------------------------------------------------------------------
// Draws the ImGui panels and applies all queued input events.
//-----------------------------------------------------------------------------
void CImguiSystem::SampleFrame()
{
	Assert(ThreadInMainThread(), "CImguiSystem::SampleFrame() should only be called from the main thread!");

	if (m_systemInitState == ImguiSystemInitStage_e::IM_PENDING_INIT)
		return;

	AUTO_LOCK(m_inputEventQueueMutex);

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();

	g_Browser.RunFrame();
	g_Console.RunFrame();

	ImGui::EndFrame();
	ImGui::Render();

	if (m_systemInitState == ImguiSystemInitStage_e::IM_SYSTEM_INIT)
		m_systemInitState = ImguiSystemInitStage_e::IM_FRAME_SAMPLED;
}

//-----------------------------------------------------------------------------
// Renders the drawn frame out which has been swapped to the snapshot buffer.
//-----------------------------------------------------------------------------
void CImguiSystem::RenderFrame()
{
	if (m_systemInitState < ImguiSystemInitStage_e::IM_FRAME_SWAPPED)
		return;

	{
		AUTO_LOCK(m_snapshotBufferMutex);
		ImGui_ImplDX11_RenderDrawData(&m_snapshotData.DrawData);
	}

	if (m_systemInitState == ImguiSystemInitStage_e::IM_FRAME_SAMPLED)
		m_systemInitState = ImguiSystemInitStage_e::IM_FRAME_RENDERED;
}

//-----------------------------------------------------------------------------
// Window procedure handler.
//-----------------------------------------------------------------------------
LRESULT CImguiSystem::MessageHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImguiSystem()->m_systemInitState == ImguiSystemInitStage_e::IM_PENDING_INIT)
		return NULL;

	AUTO_LOCK(ImguiSystem()->m_inputEventQueueMutex);

	extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
}

static CImguiSystem s_imguiSystem;

CImguiSystem* ImguiSystem()
{
	return &s_imguiSystem;
}
