#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace shippreviewscroll
{
    struct ShipPreviewWindow
    {
        BYTE x00[0x3EC];
        float zoomLevel;
    };

    void init();
}
