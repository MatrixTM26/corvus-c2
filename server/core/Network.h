#ifndef NETWORK_H
#define NETWORK_H

#include "Session.h"

#ifdef _WIN32
  #include <winsock2.h>
  #pragma comment(lib, "ws2_32.lib")
  #define NetClose(S)      closesocket(S)
  #define NetInvalid       INVALID_SOCKET
  typedef SOCKET           NetSocket;
#else
  #include <arpa/inet.h>
  #include <sys/socket.h>
  #include <unistd.h>
  #define NetClose(S)      close(S)
  #define NetInvalid       (-1)
  typedef int              NetSocket;
#endif

int       NetInit(void);
void      NetCleanup(void);
NetSocket NetCreateListener(const char *BindAddr, int Port);
void      NetSetNonBlocking(NetSocket Sock);
int       NetHandleBeacon(NetSocket Listener, AgentSession *Session);

#endif
