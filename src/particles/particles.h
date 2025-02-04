//===========================================================================//
//
// Purpose: particle system definitions
//
//===========================================================================//
#ifndef PARTICLES_H
#define PARTICLES_H

inline void (*v_ParticleSystem_Init)(void);

///////////////////////////////////////////////////////////////////////////////
class VParticles : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("ParticleSystem_Init", v_ParticleSystem_Init);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 4C 24 ?? 55 53 56 57 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 33 F6").GetPtr(v_ParticleSystem_Init);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // PARTICLES_H
