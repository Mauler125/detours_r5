#ifndef IMATERIAL_H
#define IMATERIAL_H

// See https://www.gdcvault.com/play/1024418/Efficient-Texture-Streaming-in-Titanfall
#define MATERIAL_HISTOGRAM_BIN_COUNT 16
#define MATERIAL_BLEND_STATE_COUNT 8 // R2 is 4

struct MaterialBlendState_s
{
	MaterialBlendState_s() = default;

	MaterialBlendState_s(const bool bUnknown, const bool bBlendEnable,
		const D3D11_BLEND _srcBlend, const D3D11_BLEND _destBlend,
		const D3D11_BLEND_OP _blendOp, const D3D11_BLEND _srcBlendAlpha,
		const D3D11_BLEND _destBlendAlpha, const D3D11_BLEND_OP _blendOpAlpha,
		const int8 _renderTargetWriteMask)
	{
		unknown = bUnknown ? 1 : 0;
		blendEnable = bBlendEnable ? 1 : 0;

		srcBlend = _srcBlend;
		destBlend = _destBlend;
		blendOp = _blendOp;
		srcBlendAlpha = _srcBlendAlpha;
		destBlendAlpha = _destBlendAlpha;
		blendOpAlpha = _blendOpAlpha;

		renderTargetWriteMask = _renderTargetWriteMask & 0xF;
	}

	MaterialBlendState_s(const uint32 _flags)
	{
		unknown = (_flags & 1);
		blendEnable = ((_flags >> 1) & 1);

		srcBlend = ((_flags >> 2) & 0x1F);
		destBlend = ((_flags >> 7) & 0x1F);
		blendOp = ((_flags >> 12) & 7);
		srcBlendAlpha = ((_flags >> 15) & 0x1F);
		destBlendAlpha = ((_flags >> 20) & 0x1F);
		blendOpAlpha = ((_flags >> 25) & 7);

		renderTargetWriteMask = (_flags >> 28) & 0xF;
	}

	uint32 unknown : 1;
	uint32 blendEnable : 1;
	uint32 srcBlend : 5;
	uint32 destBlend : 5;
	uint32 blendOp : 3;
	uint32 srcBlendAlpha : 5;
	uint32 destBlendAlpha : 5;
	uint32 blendOpAlpha : 3;
	uint32 renderTargetWriteMask : 4;
};

// Aligned to 16 bytes so this struct can be loaded with 3 SIMD instructions.
struct ALIGN16 MaterialRenderParams_s
{
	// Bitfield defining a D3D11_RENDER_TARGET_BLEND_DESC for each of the 8 possible DX render targets
	MaterialBlendState_s blendState[MATERIAL_BLEND_STATE_COUNT];
	uint32 blendStateMask;

	// Flags to determine how the D3D11_DEPTH_STENCIL_DESC is defined for this material.
	uint16 depthStencilFlags;

	// Flags to determine how the D3D11_RASTERIZER_DESC is defined.
	uint16 rasterizerFlags;
};

enum MaterialShaderType_e : uint8 // From RSX and RePak
{
	RGDU, // Static model with regular vertices.
	RGDP, // Static model with packed vertices.
	RGDC, // Static model with packed vertices.
	SKNU, // Skinned model with regular vertices.
	SKNP, // Skinned model with packed vertices.
	SKNC, // Skinned model with packed vertices.
	WLDU, // World geometry with regular vertices.
	WLDC, // World geometry with packed vertices.
	PTCU, // Particles with regular vertices.
	PTCS, // Particles sprites?.
};

abstract_class IMaterial
{
public:
	virtual const char*		GetName() const = 0;
	virtual uint8_t			GetMaterialType() const = 0;

	virtual const char*		GetNullString() const = 0;
	virtual int64_t			ReturnZero() const = 0;

	virtual void*			sub_1403B41A0(void* unk) = 0; // IDK

	virtual int				GetMappingWidth() const = 0;
	virtual int				GetMappingHeight() const = 0;

private:
	//TODO! <-- most of these are bitwise and operators testing flags of the member CMaterialGlue::unkFlags.
	// Don't call these without reversing/renaming first, as the const qualifier might have to be removed.
	virtual void stub_0() const = 0;
	virtual void stub_1() const = 0;
	virtual void stub_2() const = 0;
	virtual void stub_3() const = 0;
	virtual void stub_4() const = 0;
	virtual void stub_5() const = 0;
	virtual void stub_6() const = 0;
	virtual void stub_7() const = 0;
	virtual void stub_8() const = 0;
	virtual void stub_9() const = 0;
	virtual void stub_10() const = 0;
	virtual void stub_11() const = 0;
	virtual void stub_12() const = 0;
	virtual void stub_13() const = 0;
	virtual void stub_14() const = 0;
	virtual void stub_15() const = 0;
	virtual void stub_16() const = 0;
	virtual void stub_17() const = 0;
	virtual void stub_18() const = 0;
	virtual void stub_19() const = 0;
	virtual void stub_20() const = 0;
	virtual void stub_21() const = 0;
	virtual void stub_22() const = 0;
	virtual void stub_23() const = 0;
	virtual void stub_24() const = 0;
	virtual void stub_25() const = 0;
	virtual void stub_26() const = 0;
	virtual void stub_27() const = 0;
	virtual void stub_28() const = 0;
	virtual void stub_29() const = 0;
	virtual void stub_30() const = 0;
	virtual void stub_31() const = 0;
	virtual void stub_32() const = 0;
	virtual void stub_33() const = 0;
	virtual void stub_34() const = 0;
	virtual void stub_35() const = 0;
	virtual void stub_36() const = 0;

public:
	virtual bool CanCreditModelTextures() = 0;

private:
	virtual void stub_38() const = 0;
	virtual void stub_39() const = 0;
	virtual void stub_40() const = 0;
	virtual void stub_41() const = 0;
	virtual void stub_42() const = 0;
	virtual void stub_43() const = 0;
	virtual void stub_44() const = 0;
	virtual void stub_45() const = 0;
	virtual void stub_46() const = 0;
	virtual void stub_47() const = 0;
	virtual void stub_48() const = 0;
	virtual void stub_49() const = 0;
	virtual void stub_50() const = 0;
	virtual void stub_51() const = 0;
	virtual void stub_52() const = 0;
	virtual void stub_53() const = 0;
	virtual void stub_54() const = 0;
	virtual void stub_55() const = 0;
	virtual void stub_56() const = 0;
	virtual void stub_57() const = 0;
	virtual void stub_58() const = 0;
	virtual void stub_59() const = 0;
	virtual void stub_60() const = 0;
	virtual void stub_61() const = 0;
	virtual void stub_62() const = 0;
	virtual void stub_63() const = 0;
	virtual void stub_64() const = 0;
	virtual void stub_65() const = 0;
	virtual void stub_66() const = 0;
	virtual void stub_67() const = 0;
	virtual void stub_68() const = 0;
	virtual void stub_69() const = 0;
	virtual void stub_70() const = 0;
	virtual void stub_71() const = 0;
	virtual void stub_72() const = 0;
	virtual void stub_73() const = 0;
	virtual void stub_74() const = 0;
	virtual void stub_75() const = 0;
	virtual void stub_76() const = 0;
	virtual void stub_77() const = 0;
	virtual void stub_78() const = 0;
	// STUB_138 should be GetShaderGlue.
};

#endif // IMATERIAL_H
