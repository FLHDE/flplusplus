#include "graphics.h"
#include "patch.h"
#include "offsets.h"
#include "config.h"
#include "Common.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

config::ConfigData& cfg = config::get_config();

#define MIN_DETAIL_SCALE 1.0f
#define MAX_DETAIL_SCALE 1000000.0f
#define MAX_ASTEROID_DIST_SCALE 10.0f

float __fastcall multiply_lodranges_float(INI_Reader* reader, PVOID _edx, UINT index)
{
    return reader->get_value_float(index) * cfg.lodscale;
}

float __fastcall multiply_pbubble_float(INI_Reader* reader, PVOID _edx, UINT index)
{
    return reader->get_value_float(index) * cfg.pbubblescale;
}

float __fastcall multiply_characterdetail_float(INI_Reader* reader, PVOID _edx, UINT index)
{
    return reader->get_value_float(index) * cfg.characterdetailscale;
}

float __fastcall multiply_asteroiddist_float(INI_Reader* reader, PVOID _edx, UINT index)
{
    return reader->get_value_float(index) * cfg.asteroiddistscale;
}

bool patch_lodranges()
{
    if(cfg.lodscale <= MIN_DETAIL_SCALE)
        return false;
    else if (cfg.lodscale > MAX_DETAIL_SCALE)
        cfg.lodscale = MAX_DETAIL_SCALE;

    //distances
    patch::set_execute_read_write(OF_REN_DIST0, sizeof(float));
    *((float*)OF_REN_DIST0) *= cfg.lodscale;

    static UINT multiplyLodsPtr = (UINT) &multiply_lodranges_float;
    patch::patch_uint32(OF_LODS_GET_VALUE, (UINT) &multiplyLodsPtr);

    return true;
}

bool patch_pbubble()
{
    if (cfg.pbubblescale <= MIN_DETAIL_SCALE)
        return false;
    else if (cfg.pbubblescale > MAX_DETAIL_SCALE)
        cfg.pbubblescale = MAX_DETAIL_SCALE;

    static UINT multiplyPbubblePtr = (UINT) &multiply_pbubble_float;

    patch::patch_uint32(OF_PBUBBLE_GET_VALUE0, (UINT) &multiplyPbubblePtr);
    patch::patch_uint32(OF_PBUBBLE_GET_VALUE1, (UINT) &multiplyPbubblePtr);

    patch::set_execute_read_write(OF_REN_DIST1, sizeof(float));
    float ren_dist1 = *((float*)OF_REN_DIST1) * cfg.pbubblescale;

    // 40000.0f is considered to be the maximum "safe" value
    if (ren_dist1 > 40000.0f)
        ren_dist1 = 40000.0f;

    patch::patch_float(OF_REN_DIST1, ren_dist1);

    return true;
}

bool patch_characterdetail()
{
    if (cfg.characterdetailscale <= MIN_DETAIL_SCALE)
        return false;
    else if (cfg.characterdetailscale > MAX_DETAIL_SCALE)
        cfg.characterdetailscale = MAX_DETAIL_SCALE;

    auto common = (DWORD) GetModuleHandleA("common.dll");

    UINT multiplyCharacterDetailPtr = (UINT) &multiply_characterdetail_float;
    UINT detailSwitchAddr = common + F_OF_BODYPART_DETAILSWITCH_GET_VALUE;

    patch::patch_uint32(detailSwitchAddr, multiplyCharacterDetailPtr - detailSwitchAddr - 4);

    return true;
}

bool patch_asteroiddist()
{
    if (cfg.asteroiddistscale <= MIN_DETAIL_SCALE)
        return false;
    else if (cfg.asteroiddistscale > MAX_ASTEROID_DIST_SCALE)
        cfg.asteroiddistscale = MAX_ASTEROID_DIST_SCALE;

    static UINT multiplyAsteroidDistPtr = (UINT) &multiply_asteroiddist_float;
    patch::patch_uint32(OF_ASTEROID_DIST_GET_VALUE, (UINT) &multiplyAsteroidDistPtr);
    patch::patch_uint32(OF_AST_BILLBOARD_DIST_GET_VALUE, (UINT)&multiplyAsteroidDistPtr);

    return true;
}

void graphics::init()
{
    patch::patch_uint8(OF_VIDEODIALOG, 0x33); //disable unsupported video dialog
    patch::patch_uint16(OF_MAXTEXSIZE, 0x2000); //texture size bug fix
    //replace "Vibrocentric" string
    //FL tries to load this font over Agency FB, screws up UI if it finds it
    //if you have a font named '\b' you have big problems
    const char *garbageFont = "\b\0";
    auto common = (DWORD) GetModuleHandleA("common.dll");
    unsigned int address = common + F_OF_VIBROCENTRICFONT_V11;
    patch::patch_bytes(address, (void*)garbageFont, 2);

    patch_lodranges();
    patch_pbubble();
    patch_characterdetail();
    patch_asteroiddist();
}
