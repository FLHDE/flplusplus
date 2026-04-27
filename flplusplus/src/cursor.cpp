#include "cursor.h"
#include "config.h"
#include "offsets.h"
#include "patch.h"

int __fastcall UpdateMouseX(int mouseX, int increase)
{
    MOUSE_X = mouseX + increase;
    if (MOUSE_X < 0)
        MOUSE_X = 0;
    else if (MOUSE_X > WINDOW_WIDTH - 1)
        MOUSE_X = WINDOW_WIDTH - 1;

    return increase;
}

int __fastcall UpdateMouseY(int mouseY, int increase)
{
    MOUSE_Y = mouseY + increase;
     if (MOUSE_Y < 0)
         MOUSE_Y = 0;
     else if (MOUSE_Y > WINDOW_HEIGHT - 1)
         MOUSE_Y = WINDOW_HEIGHT - 1;

    return increase;
}

void cursor::hook_mouse_func(unsigned int address, void* func)
{
    unsigned char dummy[5];
    patch::patch_uint16(address, 0xC289); // mov edx, eax
    patch::detour((unsigned char*) (address + 2), func, dummy, false);
    patch::patch_uint8(address + 7, 0x90);
}

void cursor::init()
{
    if (!config::get_config().confinecursor)
        return;

    hook_mouse_func(OF_MOUSE_X_UPDATE, (void*) UpdateMouseX);
    hook_mouse_func(OF_MOUSE_Y_UPDATE, (void*) UpdateMouseY);
}
