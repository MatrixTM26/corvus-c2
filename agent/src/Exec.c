#define _POSIX_C_SOURCE 200809L
#include "../include/Exec.h"
#include <stdio.h>
#include <string.h>

void ExecCommand(const char *Cmd, char *Out, size_t Max)
{
    memset(Out, 0, Max);
#ifdef _WIN32
    FILE *P = _popen(Cmd, "r");
#else
    FILE *P = popen(Cmd, "r");
#endif
    if (!P) { snprintf(Out, Max, "[error] popen failed"); return; }
    char Line[512];
    size_t Total = 0;
    while (fgets(Line, sizeof(Line), P)) {
        size_t N = strlen(Line);
        if (Total + N >= Max - 1) break;
        memcpy(Out + Total, Line, N);
        Total += N;
    }
    if (!Total) snprintf(Out, Max, "[ok] no output");
#ifdef _WIN32
    _pclose(P);
#else
    pclose(P);
#endif
}
