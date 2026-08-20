#ifndef TlsH
#define TlsH

#include "Config.h"
#include "Session.h"
#include "Tcp.h"

#ifdef HaveOpenssl
    #include <openssl/ssl.h>
    #include <openssl/err.h>

SSL_CTX *TlsCreateCtx(const Config *C);
void     TlsHandleBeacon(NetSock Listener, SessionPool *P, const Config *C, SSL_CTX *Ctx);
void     HttpsHandleBeacon(NetSock Listener, SessionPool *P, const Config *C, SSL_CTX *Ctx);
#endif

#endif
