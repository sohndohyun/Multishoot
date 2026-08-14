#include "game/multi_shoot.hpp"
#include "network/packet.hpp"

#include <Windows.h>
#include <cstdio>
#include <cstdlib>

int main(int argc, char* argv[]) {
    char message[dr::packet_capacity];
    multi_shoot server;
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    int port = 3000;
    if (argc == 2)
        port = atoi(argv[1]);

    int result = server.init(port, system_info.dwNumberOfProcessors);
    if (result != 0) {
        fprintf(stderr, "server initialization failed: %d\n", result);
        return result;
    }

    fputs("input message to quit: ", stdout);
    server.start();

    fgets(message, dr::packet_capacity, stdin);
    server.end();

    return 0;
}
