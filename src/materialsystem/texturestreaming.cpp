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

//---------------------------------------------------------------------------------
// Purpose: shift and scale the texture's histogram to accommodate varying screen
//          FOV, screen resolutions and texture resolutions.
// Input  : *taskList - 
//---------------------------------------------------------------------------------
static void StreamDB_CreditWorldTextures(TextureStreamMgr_TaskList_s* const taskList)
{
	// If we use the GPU driven texture streaming system, do not credit the textures
	// based on the STBSP pages.
	if (gpu_driven_tex_stream->GetBool())
		return;

	v_StreamDB_CreditWorldTextures(taskList);
}

//---------------------------------------------------------------------------------
// Purpose: same as above, except for older (legacy) STBSP's (v8.0).
// Input  : *taskList - 
//---------------------------------------------------------------------------------
static void StreamDB_CreditWorldTextures_Legacy(TextureStreamMgr_TaskList_s* const taskList)
{
	// If we use the GPU driven texture streaming system, do not credit the textures
	// based on the STBSP pages.
	if (gpu_driven_tex_stream->GetBool())
		return;

	v_StreamDB_CreditWorldTextures_Legacy(taskList);
}

void VTextureStreaming::Detour(const bool bAttach) const
{
	DetourSetup(&v_StreamDB_Init, &StreamDB_Init, bAttach);

	DetourSetup(&v_StreamDB_CreditWorldTextures, &StreamDB_CreditWorldTextures, bAttach);
	DetourSetup(&v_StreamDB_CreditWorldTextures_Legacy, &StreamDB_CreditWorldTextures_Legacy, bAttach);
}
