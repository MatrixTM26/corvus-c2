#include "Session.h"
#include <string.h>
#include <stdio.h>

void SessionInit(Session *S)
{
    memset(S, 0, sizeof(*S));
}

static void ReprintPrompt(Session *S)
{
    if (S->Interactive)
        printf("\r\033[KC2Session[%s]> ", S->Address);
    else
        printf("\r\033[KC2Main> ");
    fflush(stdout);
}

void SessionRegister(Session *S, const char *Addr)
{
    int WasActive = S->Active;
    S->Active = 1;
    strncpy(S->Address, Addr, AddrSize - 1);
    S->Address[AddrSize - 1] = '\0';

    if (!WasActive) {
        printf("\r\033[K[+] Agent connected: %s\n", S->Address);
        ReprintPrompt(S);
    }
}

void SessionNotifyOutput(Session *S)
{
    printf("\r\033[K[Output]\n%s\n", S->LastOutput);
    ReprintPrompt(S);
}

void SessionSendCommand(Session *S, const char *Cmd)
{
    strncpy(S->Pending, Cmd, BufSize - 1);
    S->Pending[BufSize - 1] = '\0';
    S->HasPending = 1;
}

void SessionEnter(Session *S)
{
    S->Interactive = 1;
    printf("[*] Entering session with %s — type 'back' to return.\n", S->Address);
    printf("C2Session[%s]> ", S->Address);
    fflush(stdout);
}

void SessionLeave(Session *S)
{
    S->Interactive = 0;
    printf("C2Main> ");
    fflush(stdout);
}

void SessionKill(Session *S)
{
    SessionSendCommand(S, "kill");
    printf("[*] Kill queued.\n");
    S->Interactive = 0;
    fflush(stdout);
}
