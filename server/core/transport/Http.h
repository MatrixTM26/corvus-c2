#ifndef HTTP_H
#define HTTP_H

#include "../Cipher.h"
#include "../Config.h"
#include "../Session.h"
#include "Tcp.h"

void HttpHandleBeacon(NetSock Listener, SessionPool *P, const Config *C);

#endif
