//=============================================================================//
// 
// Purpose: Dear ImGui engine implementation
// 
//=============================================================================//

#include "imgui/misc/imgui_snapshot.h"
#include "engine/sys_mainwind.h"
#include "windows/id3dx.h"

#include "imgui_system.h"

//-----------------------------------------------------------------------------
// Constructors/Destructors.
//-----------------------------------------------------------------------------
CImguiSystem::CImguiSystem()
	: m_enabled(true)
	, m_initialized(false)
	, m_hasNewFrame(false)
{
}

//-----------------------------------------------------------------------------
// Initializes the imgui system. If this fails, false would be returned and the
// implementation won't run.
//-----------------------------------------------------------------------------
bool CImguiSystem::Init()
{
	Assert(ThreadInMainThread(), "CImguiSystem::Init() should only be called from the main thread!");
	Assert(!IsInitialized(), "CImguiSystem::Init() called recursively?");

	Assert(IsEnabled(), "CImguiSystem::Init() called while system was disabled!");

	///////////////////////////////////////////////////////////////////////////
	IMGUI_CHECKVERSION();
	ImGuiContext* const context = ImGui::CreateContext();

	if (!context)
	{
		m_enabled = false;
		return false;
	}

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

		m_enabled = false;
		return false;
	}

	m_initialized = true;
	m_hasNewFrame = false;

	return true;
}

//-----------------------------------------------------------------------------
// Shuts the imgui system down, frees all allocated buffers.
//-----------------------------------------------------------------------------
void CImguiSystem::Shutdown()
{
	Assert(ThreadInMainThread(), "CImguiSystem::Shutdown() should only be called from the main thread!");
	Assert(IsInitialized(), "CImguiSystem::Shutdown() called recursively?");

	Assert(IsEnabled(), "CImguiSystem::Shutdown() called while system was disabled!");

	AUTO_LOCK(m_snapshotBufferMutex);
	AUTO_LOCK(m_inputEventQueueMutex);

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();

	ImGui::DestroyContext();
	m_snapshotData.Clear();

	m_initialized = false;
	m_hasNewFrame = false;
}

//-----------------------------------------------------------------------------
// Add an imgui surface.
//-----------------------------------------------------------------------------
void CImguiSystem::AddSurface(CImguiSurface* const surface)
{
	Assert(IsInitialized());
	m_surfaceList.AddToTail(surface);
}

//-----------------------------------------------------------------------------
// Remove an imgui surface.
//-----------------------------------------------------------------------------
void CImguiSystem::RemoveSurface(CImguiSurface* const surface)
{
	Assert(!IsInitialized());
	m_surfaceList.FindAndRemove(surface);
}

//-----------------------------------------------------------------------------
// Draws the ImGui panels and applies all queued input events.
//-----------------------------------------------------------------------------
void CImguiSystem::SampleFrame()
{
	Assert(ThreadInMainThread(), "CImguiSystem::SampleFrame() should only be called from the main thread!");
	Assert(IsInitialized());

	AUTO_LOCK(m_inputEventQueueMutex);

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();

	FOR_EACH_VEC(m_surfaceList, i)
	{
		CImguiSurface* const surface = m_surfaceList[i];
		surface->RunFrame();
	}

	ImGui::EndFrame();
	ImGui::Render();
}

//-----------------------------------------------------------------------------
// Copies currently drawn data into the snapshot buffer which is queued to be
// rendered in the render thread. This should only be called from the same
// thread SampleFrame() is being called from.
//-----------------------------------------------------------------------------
void CImguiSystem::SwapBuffers()
{
	Assert(ThreadInMainThread(), "CImguiSystem::SwapBuffers() should only be called from the main thread!");
	Assert(IsInitialized());

	ImDrawData* const drawData = ImGui::GetDrawData();
	Assert(drawData);

	// Nothing has been drawn, nothing to swap.
	if (!drawData->CmdListsCount)
		return;

	AUTO_LOCK(m_snapshotBufferMutex);

	m_snapshotData.SnapUsingSwap(drawData, ImGui::GetTime());
	m_hasNewFrame = true;
}

//-----------------------------------------------------------------------------
// Renders the drawn frame out which has been swapped to the snapshot buffer.
//-----------------------------------------------------------------------------
void CImguiSystem::RenderFrame()
{
	Assert(IsInitialized());

	if (!m_hasNewFrame.exchange(false))
		return;

	AUTO_LOCK(m_snapshotBufferMutex);
	ImGui_ImplDX11_RenderDrawData(&m_snapshotData.DrawData);
}

//-----------------------------------------------------------------------------
// Checks whether we have an active surface.
//-----------------------------------------------------------------------------
bool CImguiSystem::IsSurfaceActive() const
{
	FOR_EACH_VEC(m_surfaceList, i)
	{
		if (m_surfaceList[i]->IsActivated())
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Window procedure handler.
//-----------------------------------------------------------------------------
LRESULT CImguiSystem::MessageHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (!ImguiSystem()->IsInitialized())
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
