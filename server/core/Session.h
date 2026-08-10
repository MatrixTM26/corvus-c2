#ifndef SESSION_H
#define SESSION_H

#define BufferSize   4096
#define AddrLen      64

typedef struct {
    int  IsActive;
    int  InSession;
    int  CommandPending;
    int  WaitingResult;
    char Address[AddrLen];
    char PendingCommand[BufferSize];
    char LastResult[BufferSize];
} AgentSession;

void SessionInit(AgentSession *S);
void SessionRegister(AgentSession *S, const char *Addr);
void SessionQueueCommand(AgentSession *S, const char *Cmd);
void SessionEnter(AgentSession *S);
void SessionLeave(AgentSession *S);
void SessionReset(AgentSession *S);

#endif
