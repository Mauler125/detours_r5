#include "tier1/keyvalues.h"
#include "engine/cmodel_bsp.h"
#include "materialsystem/texturestreaming.h"

//---------------------------------------------------------------------------------
// Purpose: loads and processes STBSP files
// (overrides level name if stbsp field has value in prerequisites file)
// Input  : *pszLevelName - 
//---------------------------------------------------------------------------------
static void StreamDB_Init(const char* const pszLevelName)
{
	KeyValues* const pSettingsKV = Mod_GetLevelSettings(pszLevelName);
	const char* targetStreamDB = pszLevelName;

	if (pSettingsKV)
	{
		KeyValues* const pStreamKV = pSettingsKV->FindKey("StreamDB");

		if (pStreamKV)
			targetStreamDB = pStreamKV->GetString();
	}

	v_StreamDB_Init(targetStreamDB);

	// If the requested STBSP file doesn't exist, then enable the GPU driven
	// texture streaming system.
	const bool gpuDriven = s_textureStreamMgr->fileHandle == FS_ASYNC_FILE_INVALID;
	gpu_driven_tex_stream->SetValue(gpuDriven);

	if (!gpuDriven)
		Msg(eDLL_T::MS, "StreamDB_Init: Loaded STBSP file '%s.stbsp'\n", targetStreamDB);
}

void VTextureStreaming::Detour(const bool bAttach) const
{
	DetourSetup(&v_StreamDB_Init, &StreamDB_Init, bAttach);
}
