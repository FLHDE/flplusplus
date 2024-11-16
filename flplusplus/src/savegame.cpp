#include "savegame.h"
#include "config.h"
#include "patch.h"
#include "log.h"
#include "offsets.h"

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <cstring>
#include <shlwapi.h>
#include <shlobj.h>
#include <io.h>
#include <direct.h>

void HandleUserDataPathFail(char * const outputBuffer, char * failedSavesDirectory)
{
    logger::writeformat("flplusplus: failed to access the saves directory for reading and writing (%s). Freelancer may not be able to properly load and store save files.", failedSavesDirectory);
    *outputBuffer = '\0';
}

void GetSavesInDirectoryPath(char * path)
{
    GetModuleFileNameA(NULL, path, MAX_PATH);
    PathRemoveFileSpecA(path);
    PathAppendA(path, "..\\SAVE");
}

bool TryGetMyGamesPath(char * path)
{
    if (SHGetFolderPathA(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, 0, path) != S_OK) {
        return false;
    }

    PathAppendA(path, "My Games");

    if (_access(path, 6) != 0) {
        if (_mkdir(path) != 0) {
            return false;
        }
    }

    PathAppendA(path, config::get_config().savefoldername.c_str());

    if (_access(path, 6) != 0) {
        if (_mkdir(path) != 0) {
            return false;
        }
    }

    return true;
}

bool UserDataPath(char * const outputBuffer)
{
    char path[MAX_PATH];

    if(config::get_config().saveindirectory) {
        GetSavesInDirectoryPath(path);
    } else {
        if (!TryGetMyGamesPath(path)) {
            HandleUserDataPathFail(outputBuffer, path);

            logger::writeline("flplusplus: saveindirectory option not set but trying to access the root SAVE directory regardless (fallback).");

            // Fallback
            GetSavesInDirectoryPath(path);
        } else {
            strncpy(outputBuffer, path, MAX_PATH);
            return true;
        }
    }

    if (_access(path, 6) != 0) {
        if (_mkdir(path) != 0) {
            HandleUserDataPathFail(outputBuffer, path);
            return false;
        }
    }

    strncpy(outputBuffer, path, MAX_PATH);
    return true;
}

DWORD loadSaveGameFuncAddr = 0x0;

bool __fastcall LoadSaveGame_Hook(PVOID thisptr, PVOID _edx, LPCSTR path, LPCSTR fileName)
{
    if (_stricmp(fileName, "Restart.fl") == 0)
        return false;

    typedef bool __fastcall LoadSaveGame(PVOID, PVOID, LPCSTR, LPCSTR);
    return ((LoadSaveGame*) loadSaveGameFuncAddr)(thisptr, _edx, path, fileName);
}

// Prevents crashes when FL loads a malformed (e.g. from another mod) Restart.fl file.
// This code makes it so that Restart.fl is recreated on every restart.
void savegame::regen_restart_on_every_launch()
{
    auto server = (DWORD) GetModuleHandleA("server.dll");

    // e.g. console.dll enforces the server library to load without causing any issues, so should be fine
    if (!server)
        server = (DWORD) LoadLibraryA("server.dll");

    UINT loadSaveGameHookPtr = (UINT) &LoadSaveGame_Hook;
    UINT loadSaveGameCallAddr = server + F_OF_LOAD_SAVE_GAME_CALL;
    loadSaveGameFuncAddr = server + F_OF_LOAD_SAVE_GAME;

    patch::patch_uint32(loadSaveGameCallAddr, loadSaveGameHookPtr - loadSaveGameCallAddr - 4);
}

void savegame::init()
{
    HMODULE common = GetModuleHandleA("common.dll");
    auto *origFunc = (unsigned char*)GetProcAddress(common, "?GetUserDataPath@@YA_NQAD@Z");
    unsigned char buffer[5];
    patch::detour(origFunc, (void*)UserDataPath, buffer);

    savegame::regen_restart_on_every_launch();
}

void savegame::get_save_folder(char *buffer)
{
    UserDataPath(buffer);
}
