#ifndef NETWORK_H
#define NETWORK_H

#include "Config.h"
#include "Session.h"
#include "transport/Tcp.h"
#include "transport/Http.h"
#include "transport/Tls.h"

typedef struct {
    NetSock Listener;
#ifdef HAVE_OPENSSL
    SSL_CTX *Ctx;
#endif
} NetHandle;

int  NetInit(void);
void NetShutdown(void);
int  NetStart(const Config *C, NetHandle *H);
void NetStop(NetHandle *H);
void NetDispatch(NetHandle *H, SessionPool *P, const Config *C);

#endif
