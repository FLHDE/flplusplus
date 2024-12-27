#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "startup.h"
#include "patch.h"
#include "offsets.h"

bool startup::check_save_games_hook()
{
    UINT originalIds = *((PUINT) OF_SAVE_GAME_FAILED_ERROR_IDS); // Save the original value
    *((PUINT) OF_SAVE_GAME_FAILED_ERROR_IDS) = 1849; // Overwrite the original value

    // Call the original function
    typedef bool (check_save_games)();
    bool result = ((check_save_games*) OF_CHECK_SAVE_GAMES)();

    *((PUINT) OF_SAVE_GAME_FAILED_ERROR_IDS) = originalIds; // Restore the original value

    return result;
}

void startup::init()
{
    BYTE originalBytes[5];

    patch::set_execute_read_write(OF_SAVE_GAME_FAILED_ERROR_IDS, sizeof(UINT));
    patch::detour((unsigned char*) OF_CHECK_SAVE_GAMES_CALL, (void*) startup::check_save_games_hook, originalBytes, false);
}