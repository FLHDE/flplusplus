#include "Freelancer.h"
#include "Common.h"

CShip* GetShip()
{
    typedef IObjInspectImpl* GetPlayerIObjInspectImpl();
    IObjInspectImpl* playerIObjInspect = ((GetPlayerIObjInspectImpl*) OF_GET_PLAYER_INSPECT_IMPL)();

    return !playerIObjInspect ? nullptr : playerIObjInspect->ship;
}

std::wstring GetSystemName()
{
    UINT currentSystemId = *((PUINT) OF_CURRENT_SYSTEM_ID);

    if (!currentSystemId)
        return {};

    UINT systemIds = Universe::get_system(currentSystemId)->idsName;

    if (!systemIds)
        return {};

    WCHAR buffer[64] = { 0 };
    GetFlString(systemIds, buffer, sizeof(buffer));

    return std::wstring(buffer);
}

std::wstring GetBaseName()
{
    UINT currentBaseId = *((PUINT) OF_CURRENT_BASE_ID);

    if (!currentBaseId)
        return {};

    UINT baseIds = Universe::get_base(currentBaseId)->idsName;

    if (!baseIds)
        return {};

    WCHAR buffer[64] = { 0 };
    GetFlString(baseIds, buffer, sizeof(buffer));

    return std::wstring(buffer);
}

std::wstring GetShipName()
{
    //CShip* ship = GetShip();
    //if (!ship)
        //return {};

    UINT currentShipId = *((PUINT) OF_CURRENT_SHIP_ID);

    if (!currentShipId)
        return {};

    Archetype::Ship* shiparch = Archetype::GetShip(currentShipId);

    if (!shiparch)
        return {};

    WCHAR buffer[64] = { 0 };
    GetFlString(shiparch->idsName, buffer, sizeof(buffer));

    return std::wstring(buffer);
}
