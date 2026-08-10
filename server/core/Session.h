#ifndef SESSION_H
#define SESSION_H

#define BufSize  4096
#define AddrSize 64

typedef struct {
    int  Active;
    int  Interactive;
    int  HasPending;
    char Address[AddrSize];
    char Pending[BufSize];
    char LastOutput[BufSize];
} Session;

void SessionInit(Session *S);
void SessionRegister(Session *S, const char *Addr);
void SessionSendCommand(Session *S, const char *Cmd);
void SessionEnter(Session *S);
void SessionLeave(Session *S);
void SessionKill(Session *S);

#endif
