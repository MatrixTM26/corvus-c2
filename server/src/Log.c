#include "../include/Log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define ColReset  "\033[0m"
#define ColWhite  "\033[1;37m"
#define ColBlue   "\033[1;34m"
#define ColYellow "\033[1;33m"
#define ColRed    "\033[1;31m"
#define ColGray   "\033[0;90m"
#define ColGreen  "\033[0;32m"
#define ColCyan   "\033[0;36m"

static const char *LevelTag(LogLevel L)
{
    switch (L) {
        case LogInfo:   return "info";
        case LogWarn:   return "warn";
        case LogError:  return "error";
        case LogCmd:    return "cmd";
        case LogOutput: return "output";
        default:        return "info";
    }
}

static const char *LevelColor(LogLevel L)
{
    switch (L) {
        case LogInfo:   return ColBlue;
        case LogWarn:   return ColYellow;
        case LogError:  return ColRed;
        case LogCmd:    return ColCyan;
        case LogOutput: return ColGreen;
        default:        return ColBlue;
    }
}

static void PrintEntry(const LogEntry *E)
{
    char Ts[20];
    struct tm *Tm = localtime(&E->Ts);
    strftime(Ts, sizeof(Ts), "%H:%M:%S", Tm);

    printf("%s%s%s [%s%s%s] %s%s%s\n",
           ColGray, Ts, ColReset,
           LevelColor(E->Level), LevelTag(E->Level), ColReset,
           ColWhite, E->Msg, ColReset);
}

void LogInit(LogStore *L)
{
    memset(L, 0, sizeof(*L));
}

void LogAdd(LogStore *L, LogLevel Level, int SessionId, const char *Msg)
{
    if (L->Count >= LogMaxEntries) return;
    LogEntry *E = &L->Entries[L->Count++];
    E->Ts        = time(NULL);
    E->Level     = Level;
    E->SessionId = SessionId;
    strncpy(E->Msg, Msg, LogMsgLen - 1);
    E->Msg[LogMsgLen - 1] = '\0';
}

void LogPrint(const LogStore *L)
{
    if (L->Count == 0) {
        printf("\n  %sNo log entries yet.%s\n\n", ColGray, ColReset);
        return;
    }
    printf("\n");
    for (int I = 0; I < L->Count; I++)
        PrintEntry(&L->Entries[I]);
    printf("\n");
}

static void WriteEntryToFile(FILE *F, const LogEntry *E)
{
    char Ts[32];
    struct tm *Tm = localtime(&E->Ts);
    strftime(Ts, sizeof(Ts), "%Y-%m-%d %H:%M:%S", Tm);
    if (E->SessionId > 0)
        fprintf(F, "[%s] [%s] [session-%d] %s\n",
                Ts, LevelTag(E->Level), E->SessionId, E->Msg);
    else
        fprintf(F, "[%s] [%s] %s\n", Ts, LevelTag(E->Level), E->Msg);
}

void LogExportAll(const LogStore *L, const char *Path)
{
    FILE *F = fopen(Path, "w");
    if (!F) {
        Msg((LogStore *)L, LogError, "Cannot open file: %s", Path);
        return;
    }
    for (int I = 0; I < L->Count; I++)
        WriteEntryToFile(F, &L->Entries[I]);
    fclose(F);
    Msg((LogStore *)L, LogInfo, "Exported %d entries to %s", L->Count, Path);
}

void LogExportSession(const LogStore *L, int SessionId, const char *Path)
{
    FILE *F = fopen(Path, "w");
    if (!F) {
        Msg((LogStore *)L, LogError, "Cannot open file: %s", Path);
        return;
    }
    int Written = 0;
    for (int I = 0; I < L->Count; I++) {
        if (L->Entries[I].SessionId == SessionId) {
            WriteEntryToFile(F, &L->Entries[I]);
            Written++;
        }
    }
    fclose(F);
    Msg((LogStore *)L, LogInfo, "Exported %d entries for session-%d to %s",
        Written, SessionId, Path);
}

void Msg(LogStore *L, LogLevel Level, const char *Fmt, ...)
{
    char Buf[LogMsgLen];
    va_list Args;
    va_start(Args, Fmt);
    vsnprintf(Buf, sizeof(Buf), Fmt, Args);
    va_end(Args);

    LogAdd(L, Level, 0, Buf);

    char Ts[20];
    time_t Now = time(NULL);
    struct tm *Tm = localtime(&Now);
    strftime(Ts, sizeof(Ts), "%H:%M:%S", Tm);

    printf("\r\033[K%s%s%s [%s%s%s] %s%s%s\n",
           ColGray, Ts, ColReset,
           LevelColor(Level), LevelTag(Level), ColReset,
           ColWhite, Buf, ColReset);
}
