#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace shippreviewscroll
{
    struct ShipPreviewParent
    {
        DWORD vftable;
    };

    struct ShipPreviewWindow
    {
        DWORD vftable;
        ShipPreviewParent* parent;
        BYTE x08[0x3E4];
        float zoomLevel; // 0x3E8
    };

    void init();
}
