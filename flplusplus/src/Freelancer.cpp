#include "Freelancer.h"
#include "Common.h"

CShip* GetShip()
{
    typedef IObjInspectImpl* GetPlayerIObjInspectImpl();
    IObjInspectImpl* playerIObjInspect = ((GetPlayerIObjInspectImpl*) OF_GET_PLAYER_INSPECT_IMPL)();

    return !playerIObjInspect ? nullptr : playerIObjInspect->ship;
}

std::wstring GetShipName()
{
    CShip* ship = GetShip();

    if (!ship)
        return {};

    WCHAR buffer[64] = { 0 };
    GetFlString(ship->shiparch->idsName, buffer, sizeof(buffer));

    return std::wstring(buffer);
}