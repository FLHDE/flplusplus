#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include "offsets.h"

std::wstring GetShipName();

inline UINT GetFlString(UINT ids, PWCHAR buffer, UINT bufferSize)
{
    PDWORD resourceHandle = *((PDWORD*) OF_RESOURCES_HANDLE);

    typedef UINT GetFlStringFunc(PDWORD, UINT, PWCHAR, UINT);
    return ((GetFlStringFunc*) OF_GET_FL_STRING)(resourceHandle, ids, buffer, bufferSize);
}