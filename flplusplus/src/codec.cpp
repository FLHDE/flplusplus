#include "codec.h"
#include "offsets.h"
#include "patch.h"
#include "config.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void codec::init()
{
    // adoxa created a plugin called MP3 Codec Fix which fixes the "missing MP3 codec" spew warning the right way.
    // However, supposedly this fix doesn't work on Wine (probably due to the missing Fraunhofer codec), despite the audio working just fine.
    // Hence we apply patches to wipe out the MP3 codec spew warnings on Wine only.
    if (!config::is_wine())
        return;

    //Patch out MP3 warnings
    auto soundManager = (DWORD) GetModuleHandleA("soundmanager.dll");
    if (soundManager)
        patch::patch_uint8(soundManager + F_OF_SOUNDMAN_MP3, 0xC3);

    auto soundStreamer = (DWORD) GetModuleHandleA("soundstreamer.dll");
    if (soundStreamer)
        patch::patch_uint8(soundStreamer + F_OF_SOUNDSTR_MP3, 0xC3);
}