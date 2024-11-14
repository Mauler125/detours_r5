//=============================================================================//
// 
// Purpose: Dear ImGui engine implementation
// 
//=============================================================================//
#ifndef IMGUI_SYSTEM_H
#define IMGUI_SYSTEM_H
#include "imgui/misc/imgui_snapshot.h"

class CImguiSystem
{
public:
	CImguiSystem();

	bool Init();
	void Shutdown();

	void SwapBuffers();

	void SampleFrame();
	void RenderFrame();

	// statics:
	static LRESULT MessageHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// inlines:
	inline bool IsInitialized() const
	{
		return m_systemInitState >= ImguiSystemInitStage_e::IM_SYSTEM_INIT;
	};

private:
	enum class ImguiSystemInitStage_e
	{
		// When the system failed to initialize, the stage would be set to
		// this.
		IM_INIT_FAILURE = -1,

		IM_PENDING_INIT,
		IM_SYSTEM_INIT,

		// State gets set to this when the first frame has been sampled.
		IM_FRAME_SAMPLED,

		// State gets set to this then buffers have been swapped for the first
		// time.
		IM_FRAME_SWAPPED,

		// Rendered for the first time.
		IM_FRAME_RENDERED
	};

	ImguiSystemInitStage_e m_systemInitState;
	ImDrawDataSnapshot m_snapshotData;

	// Mutex used during swapping and rendering, we draw the windows in the
	// main thread, and render it in the render thread. The only place this
	// mutex is used is during snapshot swapping and during rendering
	mutable CThreadMutex m_snapshotBufferMutex;

	// Mutex used between ImGui window procedure handling and drawing, see
	// https://github.com/ocornut/imgui/issues/6895. In this engine the window
	// is ran in thread separate from the main thread, therefore it needs a
	// lock to control access as main calls SampleFrame().
	mutable CThreadMutex m_inputEventQueueMutex;
};

CImguiSystem* ImguiSystem();

#endif // IMGUI_SYSTEM_H
