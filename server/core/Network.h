#ifndef NETWORK_H
#define NETWORK_H

#include "Session.h"

#ifdef _WIN32
  #include <winsock2.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef SOCKET NetSock;
  #define NetInvalid  INVALID_SOCKET
  #define NetClose(s) closesocket(s)
#else
  #include <arpa/inet.h>
  #include <sys/socket.h>
  #include <unistd.h>
  typedef int NetSock;
  #define NetInvalid  (-1)
  #define NetClose(s) close(s)
#endif

int     NetInit(void);
void    NetShutdown(void);
NetSock NetListen(const char *Addr, int Port);
void    NetNonBlock(NetSock Sock);
void    NetHandleBeacon(NetSock Listener, Session *S);

#endif
