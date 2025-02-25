#include "stdafx.h"
#include "MultiShoot.h"

void ErrorHandling(const char* message);

int main(int argc, char* argv[])
{
    char message[BUFSIZ];
    MultiShoot server;
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    int port = 3000;
    if (argc == 2)
        port = atoi(argv[1]);

    server.init(port, sysInfo.dwNumberOfProcessors);

    fputs("Input message to quit: ", stdout);
    server.start();
    
    fgets(message, BUFSIZ, stdin);

    return 0;
}

void ErrorHandling(const char* message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
