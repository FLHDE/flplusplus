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

static DWORD OnScreenshot()
{
    char directoryA[MAX_PATH];

    if(!ScreenShotPath(directoryA))
    {
        return DWORD(-1);
    }

    WCHAR directory[MAX_PATH];
    mbstowcs(directory, directoryA, MAX_PATH);

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
    std::wstring shipName = GetShipName();

    // TODO: filter strings
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


void screenshot::init()
{
    HMODULE common = GetModuleHandleA("common.dll");
    auto* getScreenShotPath = (unsigned char*)GetProcAddress(common, "?GetScreenShotPath@@YA_NQAD@Z");
    unsigned char buffer[5];
    patch::detour((unsigned char*)OF_PRINTSCREEN, (void*)OnScreenshot, buffer);
    patch::detour(getScreenShotPath, (void*)ScreenShotPath, buffer);
}
