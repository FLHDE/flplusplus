#include "directips.h"
#include "config.h"
#include "offsets.h"

bool directips::connectToServer(LPCSTR ipAndPort) {
    typedef bool (__fastcall *ConnectToServerFunc)(DWORD thisptr, PVOID edx, LPCSTR addr);
    return ((ConnectToServerFunc) CONNECT_TO_SERVER_FUNC)(CONNECT_TO_SERVER_THIS, nullptr, ipAndPort);
}

void directips::init()
{
    // Add direct ip connections
    for (const auto& directIp : config::get_config().directips) {
        connectToServer(directIp.c_str());
    }
}
