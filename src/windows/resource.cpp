#include "core/stdafx.h"
#include "core/resource.h"
#include "windows/resource.h"

/*-----------------------------------------------------------------------------
 * _resource.cpp
 *-----------------------------------------------------------------------------*/

//#############################################################################
// 
//#############################################################################
MODULERESOURCE GetModuleResource(HMODULE hModule, const int iResource)
{
    const HRSRC rc = FindResource(hModule, MAKEINTRESOURCE(iResource), MAKEINTRESOURCE(PNG));
    if (!rc)
    {
        assert(0);
        return MODULERESOURCE();
    }

    const HGLOBAL rcData = LoadResource(hModule, rc);
    if (!rcData)
    {
        assert(0);
        return MODULERESOURCE();
    }

    return (MODULERESOURCE(LockResource(rcData), SizeofResource(hModule, rc)));
}
///////////////////////////////////////////////////////////////////////////////