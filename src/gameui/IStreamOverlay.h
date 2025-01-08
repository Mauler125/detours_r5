#pragma once
#include "imgui/misc/imgui_logger.h"
#include "imgui/misc/imgui_utility.h"

#include "imgui_surface.h"

class CStreamOverlay : public CImguiSurface
{
public:
	CStreamOverlay(void);
	~CStreamOverlay(void);

	virtual bool Init(void);
	virtual void Shutdown(void);

	virtual void RunFrame(void);
	virtual bool DrawSurface(void);

	void UpdateWindowAvailability(void);

	void ResizeScratchBuffer(const size_t newSize);
	void FreeScratchBuffer(void);

	void RenderToConsole(const char* const mode);

	// Command callbacks.
	static void DumpStreamInfo_f(const CCommand& args);

private:
	char* m_scratchBuffer;
	size_t m_scratchBufferSize;

	bool m_lastAvailability;
};

extern CStreamOverlay g_streamOverlay;
