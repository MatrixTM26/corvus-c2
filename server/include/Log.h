#ifndef LogH
#define LogH

#include <stdio.h>
#include <time.h>

#define LogMaxEntries 4096
#define LogMsgLen     1024

typedef enum {
    LogInfo  = 0,
    LogWarn  = 1,
    LogError = 2,
    LogCmd   = 3,
    LogOutput = 4
} LogLevel;

typedef struct {
    time_t    Ts;
    LogLevel  Level;
    int       SessionId;
    char      Msg[LogMsgLen];
} LogEntry;

typedef struct {
    LogEntry Entries[LogMaxEntries];
    int      Count;
} LogStore;

void LogInit(LogStore *L);
void LogAdd(LogStore *L, LogLevel Level, int SessionId, const char *Msg);
void LogPrint(const LogStore *L);
void LogExportAll(const LogStore *L, const char *Path);
void LogExportSession(const LogStore *L, int SessionId, const char *Path);

void Msg(LogStore *L, LogLevel Level, const char *Fmt, ...);

#endif
