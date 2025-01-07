#pragma once
#ifndef DEDICATED
#include "materialsystem/cshaderglue.h"
#include "public/imaterialinternal.h"
#include "public/materialsystem/shader_vcs_version.h"
#include "public/rendersystem/schema/texture.g.h"

#define MATERIAL_RENDER_PARAMS_COUNT 2 // the same for r2 and r5
#define MATERIAL_SAMPLER_COUNT 4

class CMaterialGlue;

enum MaterialDepthPass_e
{
	DEPTH_SHADOW,
	DEPTH_PREPASS,
	DEPTH_VSM,
	DEPTH_SHADOW_TIGHT,

	MATERIAL_DEPTH_PASS_MAT_COUNT,
};

// Virtual function-less material instance.
struct MaterialGlue_s
{
	PakGuid_t guid;
	const char* name;

	const char* surfaceProp;
	const char* surfaceProp2;

	CMaterialGlue* depthMaterials[MATERIAL_DEPTH_PASS_MAT_COUNT];
	CMaterialGlue* colpassMaterial;

	CShaderGlue* shaderset;

	TextureAsset_s** textureHandles;
	TextureAsset_s** streamingTextureHandles;
	uint16 streamingTextureHandleCount;

	uint16 width;
	uint16 height;
	uint16 depth;

	// An array of indices into sampler states array. must be set properly to
	// have accurate texture tiling. Used in CShaderGlue::SetupShader (1403B3C60)
	byte samplers[MATERIAL_SAMPLER_COUNT];// example: 0x1D0300;

	uint32 unk_7C;

	// some features? mostly differs per material with different shader types, but
	// it seems mostly unused by the runtime too.
	uint32 unk_80_0x1F5A92BD;
	uint32 unk_84;

	uint32 materialFlags;
	uint32 materialFlags2;

	MaterialRenderParams_s renderParams[MATERIAL_RENDER_PARAMS_COUNT];
	uint32 numAnimationFrames;
	MaterialShaderType_e materialType;
	uint8 uberBufferFlags;

	int dwordf4;
	void* textureAnim;
	ID3D11Buffer* uberBuffer;
	void** pID3D11BufferVTable;
	void* viewBuffer;

	// Last frame this material was used to shift the texture streaming histogram.
	uint32 lastFrame;

	uint16 m_iUnknown4;
	uint16 m_iUnknown5;
	uint16 m_iUnknown6;
};

class CMaterialGlue : public IMaterialInternal
{
public:
	inline const MaterialGlue_s* Get() const { return &material; }
	inline MaterialGlue_s* Get() { return &material; }

private:
	byte reserved[8];
	MaterialGlue_s material;

}; //Size: 0x0130 confirmed end size.

static_assert(sizeof(CMaterialGlue) == 0x130);
#endif // !DEDICATED

inline void* g_pMaterialGlueVFTable = nullptr;


/* ==== CMATERIALGLUE ================================================================================================================================================== */
#ifndef DEDICATED
inline CMaterialGlue*(*v_GetMaterialAtCrossHair)(void);
#endif // !DEDICATED

///////////////////////////////////////////////////////////////////////////////
class VMaterialGlue : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogConAdr("CMaterialGlue::`vftable'", g_pMaterialGlueVFTable);
#ifndef DEDICATED
		LogFunAdr("CMaterialGlue::GetMaterialAtCrossHair", v_GetMaterialAtCrossHair);
#endif // !DEDICATED
	}
	virtual void GetFun(void) const
	{
#ifndef DEDICATED
		g_GameDll.FindPatternSIMD("48 8B C4 48 83 EC 58 48 83 3D ?? ?? ?? ?? ??").GetPtr(v_GetMaterialAtCrossHair);
#endif // !DEDICATED
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const
	{
		g_pMaterialGlueVFTable = g_GameDll.GetVirtualMethodTable(".?AVCMaterialGlue@@");
	}
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////
