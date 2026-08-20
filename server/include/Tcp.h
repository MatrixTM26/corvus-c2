#ifndef TcpH
#define TcpH

#include "Session.h"
#include "Config.h"
#include "Cipher.h"

#ifdef _WIN32
    #include <winsock2.h>
    typedef SOCKET NetSock;
    #define NetInvalid  INVALID_SOCKET
    #define NetClose(s) closesocket(s)
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    typedef int NetSock;
    #define NetInvalid  (-1)
    #define NetClose(s) close(s)
#endif

NetSock TcpListen(const Config *C);
void    TcpNonBlock(NetSock Sock);
void    TcpHandleBeacon(NetSock Listener, SessionPool *P);

#endif
