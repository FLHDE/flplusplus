#include "touchpad.h"
#include "offsets.h"
#include "patch.h"
#include "config.h"

// TODO: There's a bug in FL where if you play the game on a system that has a touchpad (laptop for instance),
// then scrolling will not work at all. If you use that touchpad to scroll, then it appears that you scroll infinitely.
// When this happens, toggling engine kill doesn't seem to work anymore either.
// This function applies a patch that makes scrolling with a touchpad behave as you'd expect.
// However, it completely breaks normal mouse wheel scrolling.
// It'd be nice if a solution could be implemented that fixes touchpad scrolling without breaking mouse wheel scrolling.
void touchpad::init()
{
    if (config::get_config().touchpadsupport) {
        // Fix touchpad scrolling but break normal mouse wheel scrolling
        patch::patch_uint8(OF_TOUCHPAD_FIX, 0x00);
    }
}
