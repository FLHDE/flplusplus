#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace directips {
    void init();
    bool connectToServer(LPCSTR ipAndPort);
}