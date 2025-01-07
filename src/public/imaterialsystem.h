#ifndef IMATERIALSYSTEM_H
#define IMATERIALSYSTEM_H

#define NVIDIA_VENDOR_ID 0x10DE
#define AMD_VENDOR_ID 0x1002

//-----------------------------------------------------------------------------
// Material adapter info..
//-----------------------------------------------------------------------------
struct MaterialAdapterInfo_t
{
	enum
	{
		MATERIAL_ADAPTER_NAME_LENGTH = 512
	};

	char m_pDriverName[MATERIAL_ADAPTER_NAME_LENGTH];
	unsigned int m_VendorID;
	unsigned int m_DeviceID;
	unsigned int m_SubSysID;
	unsigned int m_Revision;
	int m_nDXSupportLevel;			// This is the *preferred* dx support level
	int m_nMinDXSupportLevel;
	int m_nMaxDXSupportLevel;
	unsigned int m_nDriverVersionHigh;
	unsigned int m_nDriverVersionLow;
};

//-----------------------------------------------------------------------------
// Video mode info..
//-----------------------------------------------------------------------------
struct MaterialVideoMode_t
{
	int m_Width;
	int m_Height;
	int m_Format;
	int m_RefreshRate;
};

//-----------------------------------------------------------------------------
// Material system config..
//-----------------------------------------------------------------------------
struct MaterialSystem_Config_t
{
	MaterialVideoMode_t m_VideoMode;
	int m_nPad;
	int m_Flags;

	// TODO: The rest..
};

#endif // IMATERIALSYSTEM_H
