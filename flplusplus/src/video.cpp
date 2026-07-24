#include "video.h"
#include "offsets.h"
#include "patch.h"
#include "config.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void video::init()
{
    patch::patch_uint32(OF_VSYNC, config::get_config().vsync);

    auto common = (DWORD) GetModuleHandleA("common.dll");
    patch::patch_float(OF_MAX_FPS1, config::get_config().maxfps);
    patch::patch_float(common + F_OF_MAX_FPS2, config::get_config().maxfps);
    patch::patch_uint32(common + F_OF_MAX_FPS3, (unsigned int) &config::get_config().maxfps);
}
