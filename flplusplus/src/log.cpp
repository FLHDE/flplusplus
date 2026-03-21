#include "log.h"
#include "config.h"
#include "patch.h"
#include "offsets.h"
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <ctime>
#include <stdarg.h>

static bool linked = false;

typedef int (*pFDUMP)(DWORD, const char *, ...);
static pFDUMP *FDUMP;
static pFDUMP fdump_original;

static void do_linking()
{
    if(linked) return;
    linked = true;
    
    HMODULE dacom = GetModuleHandleA("dacom.dll");
    FDUMP = (pFDUMP*)GetProcAddress(dacom, "FDUMP");
}

void logger::writeline(const char *line)
{
#define ERRORCODE_NOTICE 0x100003
    (*FDUMP)(ERRORCODE_NOTICE, "%s", line);
}

static DWORD fdump_timestamped(DWORD errorCode, const char *fmt, ...)
{
    char buffer[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, 4096, fmt, args);
    va_end(args);
    std::time_t rawtime;
	std::tm* timeinfo;
    char timestamp[100];
	std::time(&rawtime);
	timeinfo = std::localtime(&rawtime);
	std::strftime(timestamp, 80, "%Y-%m-%d %H:%M:%S", timeinfo);
	if(config::get_config().logtoconsole) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

        auto severity = (BYTE) (errorCode);

        if (severity <= 1)
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
        else if (severity <= 2)
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN);
        else
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

        // TODO: Figure out why it sometimes prints entries with double newlines (this doesn't happen in FLSpew.txt)
	    printf("[%s] %s\n", timestamp, buffer);
	}
    return fdump_original(errorCode, "[%s] %s", timestamp, buffer);
}

void logger::patch_fdump()
{
    do_linking();
    fdump_original = *FDUMP;
    *FDUMP = (pFDUMP)fdump_timestamped;
}

// FLServer has hard coded calls to a function called ServerLogF,
// which prints non-timestamped messages,
// so patch every call to show timestamped messages instead.
void logger::patch_serverlogf()
{
#define FLSERVER_BASE (0x400000)

    // File offsets of ServerLogF calls
    DWORD serverLogCalls[] = {
            0xB152, 0xB18F, 0xB1CC, 0xB235, 0xB26D, 0xBCE4,
            0xBFD6, 0xCD03, 0x1398D, 0x13B1F, 0x13BA0
    };

    // Hook all instances where ServerLogF is called
    unsigned char originalData[5];
    for (const DWORD serverLogCall : serverLogCalls) {
        auto *originalFunc = (unsigned char *)(serverLogCall + FLSERVER_BASE);
        patch::detour(originalFunc, (void*) fdump_timestamped, originalData, false);
    }

    // Sets the server log function in remoteclient.dll
    // Never seen it being used but overwrite the function just in case
    patch::patch_uint32(OF_SERVER_LOG_FUNCTION_REF, (UINT) &fdump_timestamped);
}

void logger::writeformat(const char *fmt, ...)
{
    char buffer[4096];
    va_list args;
    va_start (args, fmt);
    vsnprintf(buffer, 4096, fmt, args);
    va_end (args);
    logger::writeline(buffer);
}
