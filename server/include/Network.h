#ifndef NetworkH
#define NetworkH

#include "Config.h"
#include "Session.h"
#include "Tcp.h"
#include "Http.h"
#include "Tls.h"

typedef struct {
    NetSock Listener;
#ifdef HaveOpenssl
    SSL_CTX *Ctx;
#endif
} NetHandle;

int  NetInit(void);
void NetShutdown(void);
int  NetStart(const Config *C, NetHandle *H);
void NetStop(NetHandle *H);
void NetDispatch(NetHandle *H, SessionPool *P, const Config *C);

#endif
