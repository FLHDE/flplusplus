#include "restart.h"
#include "offsets.h"
#include "config.h"
#include "patch.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstring>

DWORD loadSaveGameFuncAddr = 0;
char restartFileName[64];

bool __fastcall LoadSaveGame_Hook(PVOID thisptr, PVOID _edx, LPCSTR path, LPCSTR fileName)
{
    if (_stricmp(fileName, restartFileName) == 0)
        return false;

    typedef bool __fastcall LoadSaveGame(PVOID, PVOID, LPCSTR, LPCSTR);
    return ((LoadSaveGame*) loadSaveGameFuncAddr)(thisptr, _edx, path, fileName);
}

// Prevents crashes when FL loads a malformed (e.g. from another mod) Restart.fl file.
// This code makes it so that Restart.fl is recreated on every restart.
void restart::init()
{
    if (!config::get_config().alwaysregeneraterestartfile)
        return;

    // Server.dll should be loaded here
    auto server = (DWORD) GetModuleHandleA("Server.dll");

    UINT loadSaveGameHookPtr = (UINT) &LoadSaveGame_Hook;
    UINT loadSaveGameCallAddr = server + F_OF_LOAD_SAVE_GAME_CALL;
    loadSaveGameFuncAddr = server + F_OF_LOAD_SAVE_GAME;

    patch::set_execute_read_write(server + F_OF_RESTART_NAME_PTR, 4);
    patch::set_execute_read_write(server + F_OF_SAVE_FILE_FMT_PTR, 4);

    // Dynamically obtain the name of the restart file
    LPCSTR restartName = *((LPCSTR*) (server + F_OF_RESTART_NAME_PTR));
    LPCSTR saveFileFmt = *((LPCSTR*) (server + F_OF_SAVE_FILE_FMT_PTR));
    sprintf_s(restartFileName, sizeof(restartFileName), saveFileFmt, restartName);

    patch::patch_uint32(loadSaveGameCallAddr, loadSaveGameHookPtr - loadSaveGameCallAddr - 4);
}