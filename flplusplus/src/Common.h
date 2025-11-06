#pragma once
#pragma ms_struct on

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "offsets.h"

class INI_Reader
{
public:
    INI_Reader();
    ~INI_Reader();

    bool open(LPCSTR path, bool throwExceptionOnFail);
    bool read_header();
    bool is_header(LPCSTR header);
    bool read_value();
    bool is_value(LPCSTR value);
    bool get_value_bool(UINT index);
    int get_value_int(UINT index);
    float get_value_float(UINT index);
    LPCSTR get_value_string(UINT index);
    void close();
    LPCSTR get_name_ptr();

private:
    BYTE data[0x1568];
};

namespace Archetype
{
    struct Ship
    {
        BYTE x00[0x14];
        UINT idsName;
    };

    Ship* GetShip(UINT id);
}

struct CShip
{
    BYTE x00[0x88];
    Archetype::Ship* shiparch;
};

struct IObjInspectImpl
{
    BYTE data[0x10];
    CShip* ship;
};

namespace Universe
{
    struct IBase
    {
        BYTE data[0xC];
        UINT idsName;
    };

    struct ISystem
    {
        BYTE data[0x68];
        UINT idsName;
    };

    IBase* get_base(UINT id);
    ISystem* get_system(UINT id);
}