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
    static bool alreadyPrinted = false;
    if (!alreadyPrinted) {
        logger::writeformat("flplusplus: failed to access the saves directory for reading and writing (%s). Freelancer may not be able to properly load and store save files.", failedSavesDirectory);
        alreadyPrinted = true;
    }

    *outputBuffer = '\0';
}

void WriteSaveDirSuccessMessage(const char* dir)
{
    static bool alreadyPrinted = false;
    if (!alreadyPrinted) {
        logger::writeformat("flplusplus: using the following saves directory: \"%s\"", dir);
        alreadyPrinted = true;
    }
}

void WriteFallbackMessage()
{
    static bool alreadyPrinted = false;
    if (!alreadyPrinted) {
        logger::writeline("flplusplus: saveindirectory option not set but trying to access the root SAVE directory regardless (fallback).");
        alreadyPrinted = true;
    }
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

            // Fallback
            WriteFallbackMessage();
            GetSavesInDirectoryPath(path);
        } else {
            WriteSaveDirSuccessMessage(path);
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

    WriteSaveDirSuccessMessage(path);
    strncpy(outputBuffer, path, MAX_PATH);
    return true;
}

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
void savegame::regen_restart_on_every_launch()
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
    snprintf(restartFileName, sizeof(restartFileName), saveFileFmt, restartName);

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
