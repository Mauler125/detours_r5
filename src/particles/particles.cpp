//===========================================================================//
//
// Purpose: particle system code
//
//===========================================================================//
#include "tier0/commandline.h"
#include "rtech/pak/pakstate.h"
#include "particles.h"

static void ParticleSystem_Init()
{
	// Call the original function to load the core particle files, and then
	// load our effects rpak afterwards to we can patch what is loaded in the
	// effects.rpak loaded by the engine.
	v_ParticleSystem_Init();

	// This tells the engine to load the raw DMX files instead, which are
	// listed inside the particles_manifest.txt file as PCF files.
	const bool loadUnbaked = CommandLine()->FindParm("-tools") || CommandLine()->FindParm("-nobakedparticles");

	if (!loadUnbaked || CommandLine()->FindParm("-bakedparticles"))
	{
		const char* const pakName = "effects_sdk.rpak";
		const PakHandle_t pakId = g_pakLoadApi->LoadAsyncAndWait(pakName, AlignedMemAlloc(), 3, nullptr);
		
		if (pakId == PAK_INVALID_HANDLE)
			Error(eDLL_T::ENGINE, EXIT_FAILURE, "Failed to load pak file '%s'\n", pakName);
	}
}

void VParticles::Detour(const bool bAttach) const
{
	DetourSetup(&v_ParticleSystem_Init, ParticleSystem_Init, bAttach);
}
