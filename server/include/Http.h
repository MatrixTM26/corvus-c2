#ifndef HttpH
#define HttpH

#include "Cipher.h"
#include "Config.h"
#include "Session.h"
#include "Tcp.h"

void HttpHandleBeacon(NetSock Listener, SessionPool *P, const Config *C);

#endif
