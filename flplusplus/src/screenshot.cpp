//Based off code by Laz

#include "screenshot.h"
#include "patch.h"
#include "offsets.h"
#include "config.h"
#include "log.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ctime>
#include <string>
#include <shlwapi.h>
#include <shlobj.h>
#include <gdiplus.h>
#include <wchar.h>
#include <io.h>
#include <direct.h>
#include "Freelancer.h"

using namespace Gdiplus;

void HandleScreenShotPathFail(char * const outputBuffer, char * failedScreenshotsDirectory)
{
    logger::writeformat(
            "flplusplus: failed to access the screenshots directory for reading and writing (%s). Freelancer may not be able to properly store screenshots.",
            failedScreenshotsDirectory);
    *outputBuffer = '\0';
}

void GetScInDirectoryPath(char * path)
{
    GetModuleFileNameA(NULL, path, MAX_PATH);
    PathRemoveFileSpecA(path);
    PathAppendA(path, "..\\SCREENSHOTS");
}

bool TryGetScreenshotsPath(char * path)
{
    if (SHGetFolderPathA(NULL, CSIDL_MYPICTURES | CSIDL_FLAG_CREATE, NULL, 0, path) != S_OK) {
        return false;
    }

    PathAppendA(path, config::get_config().screenshotsfoldername.c_str());

    if (_access(path, 6) != 0) {
        if (_mkdir(path) != 0) {
            return false;
        }
    }

    return true;
}

bool ScreenShotPath(char * const outputBuffer)
{
    char path[MAX_PATH];

    if (config::get_config().screenshotsindirectory) {
        GetScInDirectoryPath(path);
    } else {
        if (!TryGetScreenshotsPath(path)) {
            HandleScreenShotPathFail(outputBuffer, path);
            logger::writeline("flplusplus: screenshotsindirectory option not set but trying to access the root SCREENSHOTS directory regardless (fallback).");

            // Fallback
            GetScInDirectoryPath(path);
        } else {
            strncpy(outputBuffer, path, MAX_PATH);
            return true;
        }
    }

    if (_access(path, 6) != 0) {
        if (_mkdir(path) != 0) {
            HandleScreenShotPathFail(outputBuffer, path);
            return false;
        }
    }

    strncpy(outputBuffer, path, MAX_PATH);
    return true;
}

std::wstring stows(std::string str)
{
    return std::wstring(str.begin(), str.end());
}

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
	UINT  num = 0;          // number of image encoders
	UINT  size = 0;         // size of the image encoder array in bytes

	ImageCodecInfo* pImageCodecInfo = nullptr;

	GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;  // Failure

	pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == nullptr)
		return -1;  // Failure

	GetImageEncoders(num, size, pImageCodecInfo);

	for (UINT j = 0; j < num; ++j)
	{
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0)
		{
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;  // Success
		}
	}
	free(pImageCodecInfo);
	return -1;  // Failure
}

void GetWindowSize(HWND flHWND, int& width, int& height)
{
    RECT gameWindow;
    GetClientRect(flHWND, &gameWindow);
    ClientToScreen(flHWND, (LPPOINT) &gameWindow.left);
    ClientToScreen(flHWND, (LPPOINT) &gameWindow.right);

    width = gameWindow.right - gameWindow.left;
    height = gameWindow.bottom - gameWindow.top;
}

// Ensures that chars which are forbidden according to the Windows file system are removed
std::wstring FilterFileName(const std::wstring &str)
{
    std::wstring result = str;
    const wchar_t forbiddenChars[] = L"<>:\"/\\|?*"
        "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
        "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F";
    size_t forbiddenCharsLen = wcslen(forbiddenChars);

    for (UINT i = 0; i < forbiddenCharsLen; ++i)
        result.erase(std::remove(result.begin(), result.end(), forbiddenChars[i]), result.end());

    return result;
}

static DWORD OnScreenshot()
{
    char directoryA[MAX_PATH];

    if(!ScreenShotPath(directoryA))
    {
        return DWORD(-1);
    }

    WCHAR directory[MAX_PATH];
    mbstowcs(directory, directoryA, MAX_PATH);

    // TODO: Is this check volatile?
    // What happens if the user turned fullscreen off via a third party app like Borderless Gaming.
    // Will this be reflected in Freelancer's fullscreen flag?
    // TODO: Also if fullscreen is true, it's possible that the user moved their window to a secondary monitor.
    // When this happens, the code below will take a screenshot of the main monitor's display which won't show FL.
    // It should actually capture the application on the other monitor.
    bool isFullscreen = (*((PBYTE) OF_FREELANCER_FULLSCREEN_FLAG) & 1) == 1;
    HWND flHWND = isFullscreen ? nullptr : *(HWND*) OF_FREELANCER_HWND;

    // get the device context of FL's window
	HDC hScreenDC = GetDC(flHWND);

	// and a device context to put it in
	HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    int width, height;

    if (isFullscreen)
    {
        width = GetDeviceCaps(hScreenDC, HORZRES);
        height = GetDeviceCaps(hScreenDC, VERTRES);
    }
    else
    {
        GetWindowSize(flHWND, width, height);
    }

	// maybe worth checking these are positive values
	HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);

	// get a new bitmap
	HBITMAP hOldBitmap = HBITMAP(SelectObject(hMemoryDC, hBitmap));

	BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, 0, 0, SRCCOPY);
	hBitmap = (HBITMAP)SelectObject(hMemoryDC, hOldBitmap);

    std::time_t rawtime;
	std::tm* timeinfo;
	WCHAR buffer[100];

	std::time(&rawtime);
	timeinfo = std::localtime(&rawtime);

	std::wcsftime(buffer, 80, L"%Y-%m-%d_%H-%M-%S", timeinfo);

    std::wstring outfile = std::wstring(directory) + L'/' + std::wstring(buffer);
    std::wstring systemName = FilterFileName(GetSystemName());
    std::wstring baseName = FilterFileName(GetBaseName());
    std::wstring shipName = FilterFileName(GetShipName());

    if (!systemName.empty())
        outfile += L'_' + systemName;
    if (!baseName.empty())
        outfile += L'_' + baseName;
    if (!shipName.empty())
        outfile += L'_' + shipName;

    std::wstring suffix = std::wstring(L"");
    int i = 0;
    while(PathFileExistsW((outfile + suffix + std::wstring(L".png")).c_str())) {
        i++;
        suffix = std::wstring(L"_") + std::to_wstring(i);
    }
    outfile = outfile + suffix + std::wstring(L".png");

    GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
	Bitmap* image = new Bitmap(hBitmap, nullptr);

	CLSID myClsId;
	GetEncoderClsid(L"image/png", &myClsId);

	Status status = image->Save(outfile.c_str(), &myClsId, nullptr);
	delete image;

	GdiplusShutdown(gdiplusToken);

	// clean up
	DeleteDC(hMemoryDC);
	ReleaseDC(flHWND, hScreenDC);

	return DWORD(-1);
}

PDWORD InitMainMenuHook()
{
    // Reset ship id, base id, and universe id when the main menu is displayed
    // to prevent incorrect values from being added to the screenshot names.
    ResetIds();
    return (PDWORD) 0x67A598; // Original return value
}

void screenshot::init()
{
    HMODULE common = GetModuleHandleA("common.dll");
    auto* getScreenShotPath = (unsigned char*)GetProcAddress(common, "?GetScreenShotPath@@YA_NQAD@Z");
    unsigned char buffer[5];
    patch::detour((unsigned char*)OF_PRINTSCREEN, (void*)OnScreenshot, buffer);
    patch::detour(getScreenShotPath, (void*)ScreenShotPath, buffer);
    patch::detour((unsigned char*)OF_INIT_MAIN_MENU, (void*)InitMainMenuHook, buffer, false);
}
