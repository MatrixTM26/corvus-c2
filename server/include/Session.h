#ifndef SESSION_H
#define SESSION_H

#include "Config.h"
#include <time.h>

typedef enum {
    StateIdle   = 0,
    StateActive = 1,
    StateDead   = 2
} SessionState;

typedef struct {
    int          Id;
    SessionState State;
    char         Address[AddrSize];
    time_t       FirstSeen;
    time_t       LastSeen;
    int          HasPending;
    char         Pending[BufSize];
    char         LastOutput[BufSize];
} AgentSession;

typedef struct {
    AgentSession Slots[MaxSessions];
    int          Count;
    int          Interactive;
    int          ActiveId;
} SessionPool;

void          PoolInit(SessionPool *P);
AgentSession *PoolRegister(SessionPool *P, const char *Addr);
AgentSession *PoolById(SessionPool *P, int Id);
AgentSession *PoolActive(SessionPool *P);
void          PoolList(SessionPool *P);
void          PoolKill(SessionPool *P, int Id);
void          PoolEnter(SessionPool *P, int Id);
void          PoolLeave(SessionPool *P);
void          PoolQueueCommand(AgentSession *S, const char *Cmd);
void          PoolNotifyConnect(SessionPool *P, AgentSession *S);
void          PoolNotifyOutput(SessionPool *P, AgentSession *S);

#endif
