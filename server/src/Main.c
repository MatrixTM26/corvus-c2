#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #include <winsock2.h>
    #define StdinFd _fileno(stdin)
#else
    #include <sys/select.h>
    #define StdinFd STDIN_FILENO
#endif

#include "../include/Banner.h"
#include "../include/Config.h"
#include "../include/Console.h"
#include "../include/Log.h"
#include "../include/Network.h"
#include "../include/Session.h"

int main(int Argc, char *Argv[])
{
    if (Argc < 2) {
        fprintf(stderr, "Usage: %s -s <host> -p <port> -m <mode> [options]\n", Argv[0]);
        fprintf(stderr, "  -m  tcp | http | https | tls | mtls\n");
        fprintf(stderr, "  Run with --help for full options.\n");
        return 1;
    }

    Config C;
    ConfigDefaults(&C);
    if (!ConfigParse(Argc, Argv, &C)) return 1;

    if (!NetInit()) { fprintf(stderr, "[error] Network init failed.\n"); return 1; }

    NetHandle H;
    if (!NetStart(&C, &H)) { NetShutdown(); return 1; }

    LogStore L;
    LogInit(&L);

    BannerPrint(&C);

    SessionPool Pool;
    PoolInit(&Pool, &L);

    Msg(&L, LogInfo, "Listener started on %s:%d [%s]",
        C.BindAddr, C.Port, ConfigModeName(C.Mode));

    printf("server ~$ ");
    fflush(stdout);

    while (1) {
        fd_set Fds;
        FD_ZERO(&Fds);
        FD_SET(H.Listener, &Fds);
        FD_SET(StdinFd, &Fds);

#ifdef _WIN32
        int MaxFd = 0;
#else
        int MaxFd = (int)H.Listener > StdinFd ? (int)H.Listener : StdinFd;
#endif

        struct timeval Tv = {.tv_sec = 0, .tv_usec = 100000};
        if (select(MaxFd + 1, &Fds, NULL, NULL, &Tv) < 0) continue;

        if (FD_ISSET(H.Listener, &Fds))
            NetDispatch(&H, &Pool, &C);

        if (FD_ISSET(StdinFd, &Fds)) {
            char Line[BufSize] = {0};
            if (!ConsoleRead(Line, sizeof(Line))) continue;
            if (!ConsoleExec(Line, &Pool, &C, &L)) break;
        }
    }

    NetStop(&H);
    NetShutdown();
    return 0;
}
