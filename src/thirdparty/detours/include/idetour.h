#ifndef IDETOUR_H
#define IDETOUR_H

//-----------------------------------------------------------------------------
// Interface class for context hooks
//-----------------------------------------------------------------------------
class IDetour
{
public:
	virtual ~IDetour() { ; }
	virtual void GetAdr(void) const = 0;
	virtual void GetFun(void) const = 0;
	virtual void GetVar(void) const = 0;
	virtual void GetCon(void) const = 0;

	virtual void Detour(const bool bAttach) const = 0;
	template<
		typename T,
		typename std::enable_if<DetoursIsFunctionPointer<T>::value, int>::type = 0>
	LONG DetourSetup(_Inout_ T* ppPointer, _In_ T pDetour, const bool bAttach) const
	{
		if (bAttach)
			return DetourAttach(ppPointer, pDetour);
		else
			return DetourDetach(ppPointer, pDetour);
	}
};

extern std::vector<IDetour*> g_DetourVec;
std::size_t AddDetour(IDetour* const pDetour);

#define ADDDETOUR(x,y) static std::size_t dummy_reg_##y = AddDetour( new x() );
#define XREGISTER(x,y)  ADDDETOUR(x, y)
#define REGISTER(x)     XREGISTER(x, __COUNTER__)

#endif // IDETOUR_H
