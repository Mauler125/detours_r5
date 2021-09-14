#include "pch.h"
#include "patterns.h"

char* sq_getstring(void* sqvm, int i)
{
	std::uintptr_t thisptr = reinterpret_cast<std::uintptr_t>(sqvm);

	return *(char**)(*(__int64*)(thisptr + 0x58) + 0x10 * i + 0x8) + 0x40;
}

int sq_getinteger(void* sqvm, int i)
{
	std::uintptr_t thisptr = reinterpret_cast<std::uintptr_t>(sqvm);

	return *(int*)(*(__int64*)(thisptr + 0x58) + 0x10 * i + 0x8);
}

void sq_pushbool(void* sqvm, int val)
{
	addr_sq_pushbool(sqvm, val);
}

void sq_pushstring(void* sqvm, char* string, int len)
{
	addr_sq_pushstring(sqvm, string, len);
}

void sq_pushinteger(void* sqvm, int val)
{
	addr_sq_pushinteger(sqvm, val);
}

void sq_newarray(void* sqvm, int size)
{
	addr_sq_newarray(sqvm, size);
}

void sq_arrayappend(void* sqvm, int idx)
{
	addr_sq_arrayappend(sqvm, idx);
}

void sq_newtable(void* sqvm)
{
	addr_sq_newtable(sqvm);
}

void sq_newslot(void* sqvm, int idx)
{
	addr_sq_newslot(sqvm, idx);
}