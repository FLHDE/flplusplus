#include "flplusplus.h"
#include "patch.h"
#include "offsets.h"
#include "graphics.h"
#include "config.h"
#include "screenshot.h"
#include "savegame.h"
#include "fontresource.h"
#include "startlocation.h"
#include "log.h"
#include "thnplayer.h"
#include <windows.h>
#include <shlwapi.h>
#include <vector>
#include "consolewindow.h"
#include "shippreviewscroll.h"
#include "startup.h"
#include "touchpad.h"
#include "directips.h"

// TODO: FL.exe 004E8F7D - hook wcscat and add a space before and after it

// TODO: Add options for abbreviating names in the contact lis
// Check 00417A51 and https://stackoverflow.com/questions/66795957/c-gettextextentpoint32-doesnt-give-the-correct-size
// fonts.ini: HudSmall <- Contact list font

static unsigned char thornLoadData[5];
typedef void *(__cdecl *ScriptLoadPtr)(const char*);
static ScriptLoadPtr _ThornScriptLoad;

char dataPath[MAX_PATH];

struct LateHookEntry {
    LateHookEntry(flplusplus_cblatehook func, void *data)
        : func(func), data(data)
    {}

    flplusplus_cblatehook func;
    void *data;
};
std::vector<LateHookEntry> lh;

FLPEXPORT void flplusplus_add_latehook(flplusplus_cblatehook hkfunc, void *userData)
{
    lh.emplace_back(hkfunc, userData);
}

void init_config()
{
    config::init_defaults();

    // Get EXE folder path
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    PathRemoveFileSpecA(exePath);

    // Get flplusplus.ini path
    char configPath[MAX_PATH];
    strcpy_s(configPath, sizeof(configPath), exePath);
    PathAppendA(configPath, "flplusplus.ini");

    if(PathFileExistsA(configPath)) {
        logger::writeformat("opening flplusplus.ini at %s", configPath);
        config::init_from_file(configPath);
    }

    // Get DATA folder path
    strcpy_s(dataPath, sizeof(dataPath), exePath);
    PathRemoveFileSpecA(dataPath);
    PathAppendA(dataPath, "DATA\\");

    // Get fonts.ini path
    char fontsIniPath[MAX_PATH];
    strcpy_s(fontsIniPath, sizeof(fontsIniPath), dataPath);
    PathAppendA(fontsIniPath, "FONTS\\fonts.ini");

    if (PathFileExistsA(fontsIniPath)) {
        logger::writeformat("opening fonts.ini at %s", fontsIniPath);
        config::read_font_files(fontsIniPath);
    }
}

void init_patches()
{
    logger::patch_fdump();
    init_config();
    if(config::get_config().logtoconsole)
        RedirectIOToConsole();
    logger::writeline("flplusplus: installing patches");
    graphics::init();
    screenshot::init();
    savegame::init();
    startlocation::init();
    fontresource::init(dataPath);
    thnplayer::init();
    shippreviewscroll::init();
    startup::init();
    touchpad::init();
    directips::init();

    logger::writeline("flplusplus: all patched");
}

void late_init()
{
   for(const auto& entry: lh) {
        entry.func(entry.data);
   }
}


void * __cdecl script_load_hook(const char *script)
{
	late_init();
	patch::undetour((unsigned char*)_ThornScriptLoad, thornLoadData);
	return _ThornScriptLoad(script);
}

void install_latehook(void)
{	
	//HMODULE common = GetModuleHandleA("common.dll");
	//_ThornScriptLoad = (ScriptLoadPtr)GetProcAddress(common, "?ThornScriptLoad@@YAPAUIScriptEngine@@PBD@Z");
	//patch::detour((unsigned char*)_ThornScriptLoad, (void*)script_load_hook, thornLoadData);
}

bool is_module_version11(LPCSTR moduleName, DWORD version10)
{
    typedef DWORD (DACOM_GetDllVersion)(LPCSTR dllName, PDWORD pMajor, PDWORD pMinor, PDWORD pBuild);

    bool result = false;
    static HMODULE dacom = GetModuleHandleA("dacom.dll");
    static auto getDllVersionFunc = (DACOM_GetDllVersion*) GetProcAddress(dacom, "DACOM_GetDllVersion");

    if (getDllVersionFunc)
    {
        // Hack the DACOM_GetDllVersion such that it returns the value we're after as the "major" (lol).
        // Basically instead of returning the high word of dwProductVersionMS, return the high word of dwProductVersionLS.
        // This is the only value that can be used to distinguish 1.0 DLLs from 1.1 DLLs.
        patch::patch_uint8((DWORD) dacom + F_OF_DACOM_VERSION_MS_OFFSET, 0x34);

        DWORD major, minor, build;
        if (getDllVersionFunc(moduleName, &major, &minor, &build) == 0)
        {
            // If the version is anything higher than the 1.0 version, then it's almost certainly 1.1.
            result = major > version10;
        }

        // Restore the original value.
        patch::patch_uint8((DWORD) dacom + F_OF_DACOM_VERSION_MS_OFFSET, 0x30);
    }

    return result;
}

bool check_version11(void)
{
    #define COMMON_DLL_V10_VERSION 1223
    #define SERVER_DLL_V10_VERSION 1223

    if (!is_module_version11("Common.dll", COMMON_DLL_V10_VERSION))
    {
        logger::writeline("flplusplus: Common.dll version is not 1.1, not installing");
        return false;
    }

    // E.g. console.dll enforces the server library to load without causing any issues, so should be fine
    if (!GetModuleHandleA("Server.dll"))
        LoadLibraryA("Server.dll");

    if (!is_module_version11("Server.dll", SERVER_DLL_V10_VERSION))
    {
        logger::writeline("flplusplus: Server.dll version is not 1.1, not installing");
        return false;
    }

    return true;
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        if (check_version11()) {
            init_patches();
            install_latehook();
        } else {
            return FALSE;
        }
    }

    return TRUE;
}


