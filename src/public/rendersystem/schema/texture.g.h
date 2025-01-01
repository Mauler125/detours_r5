#ifndef TEXTURE_G_H
#define TEXTURE_G_H

//-----------------------------------------------------------------------------
// Structure definitions
//-----------------------------------------------------------------------------
/*schema*/ struct TextureDesc_t
{
	uint64_t         m_AssetGuid;
	const char*      m_pDebugName;
	uint16           m_nWidth;
	uint16           m_nHeight;
	uint16           m_nDepth;
	uint16_t         m_nImageFormat;
};

/*schema*/ struct TextureHeader_t : public TextureDesc_t
{
	uint32_t m_nDataLength;
	uint8_t unknown_2;
	uint8_t m_nOptStreamedMipCount;
	uint8_t m_nArraySize;
	uint8_t m_nLayerCount;
	uint8_t m_nCPUAccessFlag; // [ PIXIE ]: In RTech::CreateDXBuffer textureDescription Usage is determined by the CPU Access Flag so I assume it's the same case here.
	uint8_t m_nPermanentMipCount;
	uint8_t m_nStreamedMipCount;
	uint8_t unknown_4[13];
	__int64 m_nPixelCount;
	uint8_t unknown_5[3];
	uint8_t m_nTotalStreamedMipCount; // Does not get set until after RTech::CreateDXTexture.
	uint8_t unk4[228];
	uint8_t unk5[57];
	ID3D11Texture2D* m_ppTexture;
	ID3D11ShaderResourceView* m_ppShaderResourceView;
	uint8_t m_nTextureMipLevels;
	uint8_t m_nTextureMipLevelsStreamedOpt;
};

struct TextureBytesPerPixel_s
{
	uint8_t x;
	uint8_t y;
};

//-----------------------------------------------------------------------------
// Table definitions
//-----------------------------------------------------------------------------
static inline const TextureBytesPerPixel_s s_pBytesPerPixel[] =
{
  { uint8_t(8u),  uint8_t(4u) },
  { uint8_t(8u),  uint8_t(4u) },
  { uint8_t(16u), uint8_t(4u) },
  { uint8_t(16u), uint8_t(4u) },
  { uint8_t(16u), uint8_t(4u) },
  { uint8_t(16u), uint8_t(4u) },
  { uint8_t(8u),  uint8_t(4u) },
  { uint8_t(8u),  uint8_t(4u) },
  { uint8_t(16u), uint8_t(4u) },
  { uint8_t(16u), uint8_t(4u) },
  { uint8_t(16u), uint8_t(4u) },
  { uint8_t(16u), uint8_t(4u) },
  { uint8_t(16u), uint8_t(4u) },
  { uint8_t(16u), uint8_t(4u) },
  { uint8_t(16u), uint8_t(1u) },
  { uint8_t(16u), uint8_t(1u) },
  { uint8_t(16u), uint8_t(1u) },
  { uint8_t(12u), uint8_t(1u) },
  { uint8_t(12u), uint8_t(1u) },
  { uint8_t(12u), uint8_t(1u) },
  { uint8_t(8u),  uint8_t(1u) },
  { uint8_t(8u),  uint8_t(1u) },
  { uint8_t(8u),  uint8_t(1u) },
  { uint8_t(8u),  uint8_t(1u) },
  { uint8_t(8u),  uint8_t(1u) },
  { uint8_t(8u),  uint8_t(1u) },
  { uint8_t(8u),  uint8_t(1u) },
  { uint8_t(8u),  uint8_t(1u) },
  { uint8_t(4u),  uint8_t(1u) },
  { uint8_t(4u),  uint8_t(1u) },
  { uint8_t(4u),  uint8_t(1u) },
  { uint8_t(4u),  uint8_t(1u) },
  { uint8_t(4u),  uint8_t(1u) },
  { uint8_t(4u),  uint8_t(1u) },
  { uint8_t(4u),  uint8_t(1u) },
  { uint8_t(4u),  uint8_t(1u) },
  { uint8_t(4u),  uint8_t(1u) },
  { uint8_t(4u),  uint8_t(1u) },
  { uint8_t(4u),  uint8_t(1u) },
  { uint8_t(4u),  uint8_t(1u) },
  { uint8_t(4u),  uint8_t(1u) },
  { uint8_t(4u),  uint8_t(1u) },
  { uint8_t(4u),  uint8_t(1u) },
  { uint8_t(4u),  uint8_t(1u) },
  { uint8_t(2u),  uint8_t(1u) },
  { uint8_t(2u),  uint8_t(1u) },
  { uint8_t(2u),  uint8_t(1u) },
  { uint8_t(2u),  uint8_t(1u) },
  { uint8_t(2u),  uint8_t(1u) },
  { uint8_t(2u),  uint8_t(1u) },
  { uint8_t(2u),  uint8_t(1u) },
  { uint8_t(2u),  uint8_t(1u) },
  { uint8_t(2u),  uint8_t(1u) },
  { uint8_t(1u),  uint8_t(1u) },
  { uint8_t(1u),  uint8_t(1u) },
  { uint8_t(1u),  uint8_t(1u) },
  { uint8_t(1u),  uint8_t(1u) },
  { uint8_t(1u),  uint8_t(1u) },
  { uint8_t(4u),  uint8_t(1u) },
  { uint8_t(4u),  uint8_t(1u) },
  { uint8_t(4u),  uint8_t(1u) },
  { uint8_t(2u),  uint8_t(1u) },
};

// Map dxgi format to txtr asset format
inline int DxgiFormatToTxtrAsset(DXGI_FORMAT dxgi)
{
	switch (dxgi)
	{
		case DXGI_FORMAT_BC1_UNORM                 : return 0;
		case DXGI_FORMAT_BC1_UNORM_SRGB            : return 1;
		case DXGI_FORMAT_BC2_UNORM                 : return 2;
		case DXGI_FORMAT_BC2_UNORM_SRGB            : return 3;
		case DXGI_FORMAT_BC3_UNORM                 : return 4;
		case DXGI_FORMAT_BC3_UNORM_SRGB            : return 5;
		case DXGI_FORMAT_BC4_UNORM                 : return 6;
		case DXGI_FORMAT_BC4_SNORM                 : return 7;
		case DXGI_FORMAT_BC5_UNORM                 : return 8;
		case DXGI_FORMAT_BC5_SNORM                 : return 9;
		case DXGI_FORMAT_BC6H_UF16                 : return 10;
		case DXGI_FORMAT_BC6H_SF16                 : return 11;
		case DXGI_FORMAT_BC7_UNORM                 : return 12;
		case DXGI_FORMAT_BC7_UNORM_SRGB            : return 13;
		case DXGI_FORMAT_R32G32B32A32_FLOAT        : return 14;
		case DXGI_FORMAT_R32G32B32A32_UINT         : return 15;
		case DXGI_FORMAT_R32G32B32A32_SINT         : return 16;
		case DXGI_FORMAT_R32G32B32_FLOAT           : return 17;
		case DXGI_FORMAT_R32G32B32_UINT            : return 18;
		case DXGI_FORMAT_R32G32B32_SINT            : return 19;
		case DXGI_FORMAT_R16G16B16A16_FLOAT        : return 20;
		case DXGI_FORMAT_R16G16B16A16_UNORM        : return 21;
		case DXGI_FORMAT_R16G16B16A16_UINT         : return 22;
		case DXGI_FORMAT_R16G16B16A16_SNORM        : return 23;
		case DXGI_FORMAT_R16G16B16A16_SINT         : return 24;
		case DXGI_FORMAT_R32G32_FLOAT              : return 25;
		case DXGI_FORMAT_R32G32_UINT               : return 26;
		case DXGI_FORMAT_R32G32_SINT               : return 27;
		case DXGI_FORMAT_R10G10B10A2_UNORM         : return 28;
		case DXGI_FORMAT_R10G10B10A2_UINT          : return 29;
		case DXGI_FORMAT_R11G11B10_FLOAT           : return 30;
		case DXGI_FORMAT_R8G8B8A8_UNORM            : return 31;
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB       : return 32;
		case DXGI_FORMAT_R8G8B8A8_UINT             : return 33;
		case DXGI_FORMAT_R8G8B8A8_SNORM            : return 34;
		case DXGI_FORMAT_R8G8B8A8_SINT             : return 35;
		case DXGI_FORMAT_R16G16_FLOAT              : return 36;
		case DXGI_FORMAT_R16G16_UNORM              : return 37;
		case DXGI_FORMAT_R16G16_UINT               : return 38;
		case DXGI_FORMAT_R16G16_SNORM              : return 39;
		case DXGI_FORMAT_R16G16_SINT               : return 40;
		case DXGI_FORMAT_R32_FLOAT                 : return 41;
		case DXGI_FORMAT_R32_UINT                  : return 42;
		case DXGI_FORMAT_R32_SINT                  : return 43;
		case DXGI_FORMAT_R8G8_UNORM                : return 44;
		case DXGI_FORMAT_R8G8_UINT                 : return 45;
		case DXGI_FORMAT_R8G8_SNORM                : return 46;
		case DXGI_FORMAT_R8G8_SINT                 : return 47;
		case DXGI_FORMAT_R16_FLOAT                 : return 48;
		case DXGI_FORMAT_R16_UNORM                 : return 49;
		case DXGI_FORMAT_R16_UINT                  : return 50;
		case DXGI_FORMAT_R16_SNORM                 : return 51;
		case DXGI_FORMAT_R16_SINT                  : return 52;
		case DXGI_FORMAT_R8_UNORM                  : return 53;
		case DXGI_FORMAT_R8_UINT                   : return 54;
		case DXGI_FORMAT_R8_SNORM                  : return 55;
		case DXGI_FORMAT_R8_SINT                   : return 56;
		case DXGI_FORMAT_A8_UNORM                  : return 57;
		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP        : return 58;
		case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM: return 59;
		case DXGI_FORMAT_D32_FLOAT                 : return 60;
		case DXGI_FORMAT_D16_UNORM                 : return 61;

		default                                    : return 0;
	}
}

// Map txtr asset format to dxgi format
static inline const DXGI_FORMAT g_TxtrAssetToDxgiFormat[] =
{
	DXGI_FORMAT_BC1_UNORM,
	DXGI_FORMAT_BC1_UNORM_SRGB,
	DXGI_FORMAT_BC2_UNORM,
	DXGI_FORMAT_BC2_UNORM_SRGB,
	DXGI_FORMAT_BC3_UNORM,
	DXGI_FORMAT_BC3_UNORM_SRGB,
	DXGI_FORMAT_BC4_UNORM,
	DXGI_FORMAT_BC4_SNORM,
	DXGI_FORMAT_BC5_UNORM,
	DXGI_FORMAT_BC5_SNORM,
	DXGI_FORMAT_BC6H_UF16,
	DXGI_FORMAT_BC6H_SF16,
	DXGI_FORMAT_BC7_UNORM,
	DXGI_FORMAT_BC7_UNORM_SRGB,
	DXGI_FORMAT_R32G32B32A32_FLOAT,
	DXGI_FORMAT_R32G32B32A32_UINT,
	DXGI_FORMAT_R32G32B32A32_SINT,
	DXGI_FORMAT_R32G32B32_FLOAT,
	DXGI_FORMAT_R32G32B32_UINT,
	DXGI_FORMAT_R32G32B32_SINT,
	DXGI_FORMAT_R16G16B16A16_FLOAT,
	DXGI_FORMAT_R16G16B16A16_UNORM,
	DXGI_FORMAT_R16G16B16A16_UINT,
	DXGI_FORMAT_R16G16B16A16_SNORM,
	DXGI_FORMAT_R16G16B16A16_SINT,
	DXGI_FORMAT_R32G32_FLOAT,
	DXGI_FORMAT_R32G32_UINT,
	DXGI_FORMAT_R32G32_SINT,
	DXGI_FORMAT_R10G10B10A2_UNORM,
	DXGI_FORMAT_R10G10B10A2_UINT,
	DXGI_FORMAT_R11G11B10_FLOAT,
	DXGI_FORMAT_R8G8B8A8_UNORM,
	DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
	DXGI_FORMAT_R8G8B8A8_UINT,
	DXGI_FORMAT_R8G8B8A8_SNORM,
	DXGI_FORMAT_R8G8B8A8_SINT,
	DXGI_FORMAT_R16G16_FLOAT,
	DXGI_FORMAT_R16G16_UNORM,
	DXGI_FORMAT_R16G16_UINT,
	DXGI_FORMAT_R16G16_SNORM,
	DXGI_FORMAT_R16G16_SINT,
	DXGI_FORMAT_R32_FLOAT,
	DXGI_FORMAT_R32_UINT,
	DXGI_FORMAT_R32_SINT,
	DXGI_FORMAT_R8G8_UNORM,
	DXGI_FORMAT_R8G8_UINT,
	DXGI_FORMAT_R8G8_SNORM,
	DXGI_FORMAT_R8G8_SINT,
	DXGI_FORMAT_R16_FLOAT,
	DXGI_FORMAT_R16_UNORM,
	DXGI_FORMAT_R16_UINT,
	DXGI_FORMAT_R16_SNORM,
	DXGI_FORMAT_R16_SINT,
	DXGI_FORMAT_R8_UNORM,
	DXGI_FORMAT_R8_UINT,
	DXGI_FORMAT_R8_SNORM,
	DXGI_FORMAT_R8_SINT,
	DXGI_FORMAT_A8_UNORM,
	DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
	DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM,
	DXGI_FORMAT_D32_FLOAT,
	DXGI_FORMAT_D16_UNORM,
};

#endif // TEXTURE_G_H
