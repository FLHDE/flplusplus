//Based off code by Laz

#include "screenshot.h"
#include "patch.h"
#include "offsets.h"
#include "config.h"
#include "log.h"
#include "Freelancer.h"

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
#include <algorithm>
#include <cstdlib>

using namespace Gdiplus;

#define MAX_SCREENSHOT_PATH_CHECK_ATTEMPTS 20
bool altFullscreenScreenshots = false;
bool altWindowedScreenshots = false;

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
            strncpy_s(outputBuffer, MAX_PATH, path, MAX_PATH);
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

std::wstring stows(const std::string& str)
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

std::wstring GetScreenshotOutPath(LPCWSTR directory, const std::wstring &baseFileName, int suffixIndex)
{
    std::wstring fileName = baseFileName;
    if (suffixIndex > 0) {
        fileName += std::wstring(L"_") + std::to_wstring(suffixIndex);
    }
    fileName += std::wstring(L".png");

    WCHAR cleanedFileName[MAX_PATH];
    wcscpy_s(cleanedFileName, _countof(cleanedFileName), fileName.c_str());
    PathCleanupSpec(directory, cleanedFileName);

    return std::wstring(directory) + L'\\' + std::wstring(cleanedFileName);
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
    // TODO: Also if fullscreen is true, it's possible that the user moved their window to a secondary monitor and then enabled fullscreen.
    // Now the FL window is in fullscreen mode on the secondary monitor.
    // When this happens, the code below will take a screenshot of the main monitor's display which won't show anything from the FL window.
    // It should actually capture the fullscreen FL window on the secondary monitor.

    // Test if you can move fullscreen window to other monitor and still take a screenshot (windows shift left arrow).
    // Replace nullptr with GetActiveWindow, or GetForegroundWindow, or GetDesktopWindow, GetWindowDC. Test with broken DxWrapper version from the old FLSR release.
    bool isFullscreen = (*((PBYTE) OF_FREELANCER_FULLSCREEN_FLAG) & 1) == 1;
    bool useFullscreenScreenshotCode = isFullscreen;

    if ((isFullscreen && altFullscreenScreenshots) || (!isFullscreen && altWindowedScreenshots))
        useFullscreenScreenshotCode = !useFullscreenScreenshotCode;

    HWND flHWND = useFullscreenScreenshotCode ? nullptr : *(HWND*) OF_FREELANCER_HWND;

    // get the device context of FL's window
	HDC hScreenDC = GetDC(flHWND);

	// and a device context to put it in
	HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    int width, height;

    if (useFullscreenScreenshotCode)
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

    std::wstring systemName = GetSystemName();
    std::wstring baseName = GetBaseName();
    std::wstring shipName = GetShipName();

    std::wstring fileName = std::wstring(buffer);
    if (!systemName.empty())
        fileName += L'_' + systemName;
    if (!baseName.empty())
        fileName += L'_' + baseName;
    if (!shipName.empty())
        fileName += L'_' + shipName;

    int i = 0;
    std::wstring outfile = GetScreenshotOutPath(directory, fileName, i);

    while (PathFileExistsW(outfile.c_str()) && i < MAX_SCREENSHOT_PATH_CHECK_ATTEMPTS) {
        i++;
        outfile = GetScreenshotOutPath(directory, fileName, i);
    }

    if (i < MAX_SCREENSHOT_PATH_CHECK_ATTEMPTS)
    {
        GdiplusStartupInput gdiplusStartupInput;
        ULONG_PTR gdiplusToken;
        GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
        Bitmap* image = new Bitmap(hBitmap, nullptr);

        CLSID myClsId;
        GetEncoderClsid(L"image/png", &myClsId);

        Status status = image->Save(outfile.c_str(), &myClsId, nullptr);
        delete image;

        GdiplusShutdown(gdiplusToken);
    }

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
    altFullscreenScreenshots = config::get_config().altfullscreenscreenshots;
    altWindowedScreenshots = config::get_config().altwindowedscreenshots;

    HMODULE common = GetModuleHandleA("common.dll");
    auto* getScreenShotPath = (unsigned char*)GetProcAddress(common, "?GetScreenShotPath@@YA_NQAD@Z");
    unsigned char buffer[5];
    patch::detour((unsigned char*)OF_PRINTSCREEN, (void*)OnScreenshot, buffer);
    patch::detour(getScreenShotPath, (void*)ScreenShotPath, buffer);
    patch::detour((unsigned char*)OF_INIT_MAIN_MENU, (void*)InitMainMenuHook, buffer, false);
}
