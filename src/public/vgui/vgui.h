#pragma once

namespace vgui
{
	// handle to an internal vgui panel
	// this is the only handle to a panel that is valid across dll boundaries
	typedef void* VPANEL;

	// handles to vgui objects
	// NULL values signify an invalid value
	typedef unsigned long HScheme;
	// Both -1 and 0 are used for invalid textures. Be careful.
	typedef unsigned long HTexture;
	typedef unsigned long HCursor;
	typedef unsigned long HPanel;
	const HPanel INVALID_PANEL = 0xffffffff;
	typedef unsigned long HFont;
	const HFont INVALID_FONT = 0; // the value of an invalid font handle
}
