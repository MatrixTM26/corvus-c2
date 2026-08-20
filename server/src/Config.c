#include "../include/Config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ConfigDefaults(Config *C)
{
    memset(C, 0, sizeof(*C));
    C->Mode      = ModeTcp;
    C->Port      = 4444;
    C->BeaconMs  = 3000;
    C->JitterPct = 15;
    strncpy(C->BindAddr,  "0.0.0.0",                    AddrSize   - 1);
    strncpy(C->UserAgent, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
                          "AppleWebKit/537.36 (KHTML, like Gecko) "
                          "Chrome/124.0.0.0 Safari/537.36", MaxUaLen - 1);
    strncpy(C->HttpPath,  "/update",                    MaxPathLen - 1);
    strncpy(C->CertFile,  "certs/server/server.crt",   MaxPathLen - 1);
    strncpy(C->KeyFile,   "certs/server/server.key",   MaxPathLen - 1);
    strncpy(C->CaFile,    "certs/server/ca.crt",       MaxPathLen - 1);
}

const char *ConfigModeName(TransportMode M)
{
    switch (M) {
        case ModeTcp:   return "tcp";
        case ModeHttp:  return "http";
        case ModeHttps: return "https";
        case ModeTls:   return "tls";
        case ModeMtls:  return "mtls";
        default:        return "unknown";
    }
}

int ConfigParse(int Argc, char *Argv[], Config *C)
{
    for (int I = 1; I < Argc; I++) {
        if      (!strcmp(Argv[I], "-s")       && I+1 < Argc) strncpy(C->BindAddr,  Argv[++I], AddrSize   - 1);
        else if (!strcmp(Argv[I], "-p")       && I+1 < Argc) C->Port = atoi(Argv[++I]);
        else if (!strcmp(Argv[I], "-m")       && I+1 < Argc) {
            const char *M = Argv[++I];
            if      (!strcmp(M, "tcp")  || !strcmp(M, "raw")) C->Mode = ModeTcp;
            else if (!strcmp(M, "http"))                       C->Mode = ModeHttp;
            else if (!strcmp(M, "https"))                      C->Mode = ModeHttps;
            else if (!strcmp(M, "tls"))                        C->Mode = ModeTls;
            else if (!strcmp(M, "mtls"))                       C->Mode = ModeMtls;
            else { fprintf(stderr, "[!] Unknown mode: %s\n", M); return 0; }
        }
        else if (!strcmp(Argv[I], "--cert")   && I+1 < Argc) strncpy(C->CertFile,  Argv[++I], MaxPathLen - 1);
        else if (!strcmp(Argv[I], "--key")    && I+1 < Argc) strncpy(C->KeyFile,   Argv[++I], MaxPathLen - 1);
        else if (!strcmp(Argv[I], "--ca")     && I+1 < Argc) strncpy(C->CaFile,    Argv[++I], MaxPathLen - 1);
        else if (!strcmp(Argv[I], "--path")   && I+1 < Argc) strncpy(C->HttpPath,  Argv[++I], MaxPathLen - 1);
        else if (!strcmp(Argv[I], "--ua")     && I+1 < Argc) strncpy(C->UserAgent, Argv[++I], MaxUaLen   - 1);
        else if (!strcmp(Argv[I], "--beacon") && I+1 < Argc) C->BeaconMs  = atoi(Argv[++I]);
        else if (!strcmp(Argv[I], "--jitter") && I+1 < Argc) C->JitterPct = atoi(Argv[++I]);
    }
    return 1;
}

void ConfigPrint(const Config *C)
{
    printf("  mode     : %s\n",      ConfigModeName(C->Mode));
    printf("  bind     : %s:%d\n",   C->BindAddr, C->Port);
    printf("  beacon   : %dms  jitter %d%%\n", C->BeaconMs, C->JitterPct);
    printf("  path     : %s\n",      C->HttpPath);
    printf("  ua       : %s\n",      C->UserAgent);
    if (C->Mode >= ModeTls)
        printf("  cert/key : %s  %s\n", C->CertFile, C->KeyFile);
    if (C->Mode == ModeMtls)
        printf("  ca       : %s\n",  C->CaFile);
}
