#pragma once
#ifndef DEDICATED
#include "materialsystem/cshaderglue.h"
#include "public/imaterialinternal.h"
#include "public/materialsystem/shader_vcs_version.h"
#include "public/rendersystem/schema/texture.g.h"

struct MaterialDXState_t
{
	uint32_t blendState[8];
	unsigned int unkFlags;
	unsigned __int16 depthStencilFlags;
	unsigned __int16 rasterizerFlags;
	char pad[8];
};

#pragma pack(push, 1)
class CMaterialGlue : public IMaterialInternal
{
public:
	uint8_t pad_0008[8]; //0x0008
	uint64_t assetGuid; //0x0010
	const char* name; //0x0018

	const char* surfaceProp; //0x0020
	const char* surfaceProp2; //0x0028

	CMaterialGlue* depthShadowMaterial; //0x0030
	CMaterialGlue* depthPrepassMaterial; //0x0038
	CMaterialGlue* depthVSMMaterial; //0x0040
	CMaterialGlue* depthShadowTightMaterial; //0x0048
	CMaterialGlue* colpassMaterial; //0x0050

	CShaderGlue* shaderset; //0x0058

	TextureAsset_s** textureHandles; //0x0060
	TextureAsset_s** streamingTextureHandles; //0x0068

	int16_t numStreamingTextureHandles; //0x0070

	int16_t width; //0x0072 
	int16_t height; //0x0074
	int16_t depth; //0x0076

	uint32_t samplers; //0x0078

	char padding_7C[4]; //0x007C

	uint32_t unk_80;
	uint32_t unk_84;

	uint64_t flags; // 0x0088

	MaterialDXState_t dxStates[2];

	uint16_t numAnimationFrames; // used in CMaterialGlue::GetNumAnimationFrames (0x1403B4250), which is called from GetSpriteInfo @ 0x1402561FC
	uint8_t materialType;
	uint8_t bytef3;

	char padding_F4[4];

	void* textureAnim;
	void** dxBuffer;
	void** unkD3DPointer; // was m_pID3D11BufferVTable
	void* viewsBuffer;

	uint32_t unknown3; //0x0118
	uint16_t unknown4; //0x011C
	uint16_t unknown5; //0x011E
	uint16_t unknown6; //0x0120
	uint64_t unknown7; //0x0122
	uint32_t unknown8; //0x012A
	uint16_t unknown9; //0x012E
}; //Size: 0x0130 confirmed end size.
static_assert(sizeof(CMaterialGlue) == 0x130);
#pragma pack(pop)
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
