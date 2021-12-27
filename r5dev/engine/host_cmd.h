#pragma once

namespace
{
	/* ==== HOST ============================================================================================================================================================ */
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS p_Host_Init = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xD9\xFF\x15\x00\x00\x00\x00", "xxxx?xxxx?xxxx?xxxxxxxxxxxx????xxxxx????");
	void* (*Host_Init)(bool* bDedicated) = (void* (*)(bool*))p_Host_Init.GetPtr(); /*48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 48 8B D9 FF 15 ? ? ? ?*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS p_Host_Init = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x00\x00\x00\x00\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x2B\xE0\x48\x8B\xD9", "xxxx?xxxx?xxxx?xxxxxxxxxxxxx????x????x????xxxxxx");
	void* (*Host_Init)(bool* bDedicated) = (void* (*)(bool*))p_Host_Init.GetPtr(); /*48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 48 8B D9*/
#endif
}

namespace
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS g_pMallocPool = p_Host_Init.Offset(0x600).FindPatternSelf("48 8D 15 ?? ?? ?? 01", ADDRESS::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS g_pMallocPool = p_Host_Init.Offset(0x130).FindPatternSelf("48 8D 15 ?? ?? ?? 01", ADDRESS::Direction::DOWN, 100).ResolveRelativeAddressSelf(0x3, 0x7);
#endif
}

///////////////////////////////////////////////////////////////////////////////
class HHostCmd : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: Host_Init                            : 0x" << std::hex << std::uppercase << p_Host_Init.GetPtr()   << std::setw(npad) << " |" << std::endl;
		std::cout << "| VAR: g_pMallocPool                        : 0x" << std::hex << std::uppercase << g_pMallocPool.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HHostCmd);
