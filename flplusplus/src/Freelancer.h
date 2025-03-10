#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include "offsets.h"

std::wstring GetSystemName();
std::wstring GetBaseName();
std::wstring GetShipName();

inline UINT GetFlString(UINT ids, PWCHAR buffer, UINT bufferSize)
{
    PDWORD resourceHandle = *((PDWORD*) OF_RESOURCES_HANDLE);

    typedef UINT GetFlStringFunc(PDWORD, UINT, PWCHAR, UINT);
    return ((GetFlStringFunc*) OF_GET_FL_STRING)(resourceHandle, ids, buffer, bufferSize);
}

inline UINT GetShipId()
{
    typedef UINT GetShipIdFunc();
    return ((GetShipIdFunc*) OF_GET_SHIP_ID)();
}

inline void ResetIds()
{
    *((PUINT) OF_CURRENT_SYSTEM_ID) = *((PUINT) OF_CURRENT_BASE_ID) = *((PUINT) OF_CURRENT_SHIP_ID) = NULL;
}
