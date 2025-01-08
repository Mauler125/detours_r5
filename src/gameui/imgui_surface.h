#ifndef IMGUI_SURFACE_H
#define IMGUI_SURFACE_H

#define IMGUI_SURFACE_FADE_ANIM_SPEED 5.0f

#include "imgui/misc/imgui_utility.h"

class CImguiSurface
{
public:
	CImguiSurface();
	virtual ~CImguiSurface() { };

	virtual bool Init() = 0;
	virtual void Shutdown() = 0;

	virtual void Animate();
	virtual void RunFrame() = 0;

	virtual bool DrawSurface() = 0;
	virtual void SetStyleVar(const float width, const float height, const float x, const float y);

	// inlines:
	inline void ToggleActive() { m_activated ^= true; }

	inline bool IsActivated() const { return m_activated; }
	inline bool IsVisible() const { return m_fadeAlpha > 0.0f; }

protected:
	const char* m_surfaceLabel;

	float m_fadeAlpha;
	ImGuiStyle_t m_surfaceStyle;

	bool m_initialized;
	bool m_activated;
	bool m_reclaimFocus;
};

#endif // IMGUI_SURFACE_H
