#include "log.h"
#include "config.h"
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <ctime>
#include <stdarg.h>

static bool linked = false;

typedef DWORD (*pFDUMP)(DWORD, const char *, ...);
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
    do_linking();
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

void logger::writeformat(const char *fmt, ...)
{
    char buffer[4096];
    va_list args;
    va_start (args, fmt);
    vsnprintf(buffer, 4096, fmt, args);
    va_end (args);
    logger::writeline(buffer);
}
