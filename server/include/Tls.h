#ifndef TLS_H
#define TLS_H

#include "Config.h"
#include "Session.h"
#include "Tcp.h"

#ifdef HAVE_OPENSSL
    #include <openssl/ssl.h>
    #include <openssl/err.h>

SSL_CTX *TlsCreateCtx(const Config *C);
void     TlsHandleBeacon(NetSock Listener, SessionPool *P, const Config *C, SSL_CTX *Ctx);
void     HttpsHandleBeacon(NetSock Listener, SessionPool *P, const Config *C, SSL_CTX *Ctx);
#endif

#endif
