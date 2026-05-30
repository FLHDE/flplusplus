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

// It is recommended to call this function rather than getting the CURRENT_SHIP_ID directly.
// This is because Console hooks this function to make it so that the player has no ship sometimes.
inline UINT GetShipId()
{
    typedef UINT GetShipIdFunc();
    return ((GetShipIdFunc*) OF_GET_SHIP_ID)();
}
