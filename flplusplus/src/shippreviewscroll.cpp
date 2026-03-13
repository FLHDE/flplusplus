#include "shippreviewscroll.h"
#include "offsets.h"
#include "patch.h"
#include "config.h"
#include <algorithm>

using namespace shippreviewscroll;

#define MIN_SCROLLING_SPEED 0.0f
#define MAX_SCROLLING_SPEED 50.0f

#define NN_SHIPTRADER_VFTABLE_ADDR (0x5D593C)

float scrollingSpeed;
float scrollMinDistance;
float scrollMaxDistance;

bool __fastcall ShipPreviewWindowScroll(ShipPreviewWindow* window, PVOID _edx, int scrollValue)
{
    // The exact same ship preview element is used in other places as well,
    // e.g. the FL beta inventory showed a top-down model of the player ship, but plugins can re-enable it.
    // We don't want the zooming to work anywhere else other than in the ship dealer.
    if (window->parent->vftable != NN_SHIPTRADER_VFTABLE_ADDR)
        return false;

    window->zoomLevel += scrollingSpeed * static_cast<float>(scrollValue);

    // Zoom levels are always negative if you "zoom away" from the ship.
    // If you want to zoom "through" the ship, it becomes positive.
    window->zoomLevel = std::max<float>(-scrollMaxDistance, std::min<float>(-scrollMinDistance, window->zoomLevel));

    // The scroll function should just return false
    return false;
}

// TODO: Hook the "on-frame" update function and implement smooth scrolling (see Turret Zoom plugin)
// TODO: Allow the scrolling speed to be scaled based on the ship archetype (ini configurable)
// [ShipPreviewWindow* +0x32C] contains the ship archetype ID
void shippreviewscroll::init()
{
    if (config::get_config().shippreviewscrollingspeed < MIN_SCROLLING_SPEED)
        scrollingSpeed = MIN_SCROLLING_SPEED;
    else if (config::get_config().shippreviewscrollingspeed > MAX_SCROLLING_SPEED)
        scrollingSpeed = MAX_SCROLLING_SPEED;
    else
        scrollingSpeed = config::get_config().shippreviewscrollingspeed;

    if (config::get_config().shippreviewscrollinginverse)
        scrollingSpeed = -scrollingSpeed;

    scrollMinDistance = config::get_config().shippreviewscrollingmindistance;
    scrollMaxDistance = config::get_config().shippreviewscrollingmaxdistance;

    if (scrollMaxDistance < scrollMinDistance)
        scrollMaxDistance = scrollMinDistance;

    // Every window in Freelancer has a virtual "scroll" function
    // In the case of the ship preview window, this function does basically nothing
    // We replace the pointer to this dummy function in the ship preview window's vftable with our own
    patch::patch_uint32(OF_SHIP_PREVIEW_WINDOW_SCROLL, (UINT) ShipPreviewWindowScroll);
}
