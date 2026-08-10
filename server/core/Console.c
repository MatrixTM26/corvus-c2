#include "Console.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void ClearScreen(void)
{
#ifdef _WIN32
    int R = system("cls"); (void)R;
#else
    fputs("\033[2J\033[H", stdout);
    fflush(stdout);
#endif
}

void ConsolePrintHelp(void)
{
    printf("\n\033[1;37m  Server Commands\033[0m\n");
    printf("  \033[33m%-22s\033[0m %s\n", "help",             "Show this help");
    printf("  \033[33m%-22s\033[0m %s\n", "sessions",         "List all active sessions");
    printf("  \033[33m%-22s\033[0m %s\n", "use <id>",         "Enter interactive shell with agent");
    printf("  \033[33m%-22s\033[0m %s\n", "kill <id>",        "Kill a specific agent session");
    printf("  \033[33m%-22s\033[0m %s\n", "kill all",         "Kill all active sessions");
    printf("  \033[33m%-22s\033[0m %s\n", "info",             "Show server configuration");
    printf("  \033[33m%-22s\033[0m %s\n", "clear",            "Clear terminal");
    printf("  \033[33m%-22s\033[0m %s\n", "exit / quit",      "Shutdown server");
    printf("\n\033[1;37m  Session Commands\033[0m  (inside use <id>)\n");
    printf("  \033[33m%-22s\033[0m %s\n", "<command>",        "Execute shell command on agent");
    printf("  \033[33m%-22s\033[0m %s\n", "back",             "Return to server console");
    printf("  \033[33m%-22s\033[0m %s\n", "kill",             "Kill this session");
    printf("  \033[33m%-22s\033[0m %s\n", "clear",            "Clear terminal");
    printf("\n");
}

int ConsoleRead(char *Out, int Cap)
{
    if (fgets(Out, Cap, stdin) == NULL)
        return 0;
    Out[strcspn(Out, "\n")] = '\0';
    return 1;
}

static void PrintPrompt(SessionPool *P)
{
    if (P->Interactive && P->ActiveId > 0)
        printf("session-%d ~$ ", P->ActiveId);
    else
        printf("server ~$ ");
    fflush(stdout);
}

int ConsoleExec(const char *Line, SessionPool *P, const Config *C)
{
    (void)C;

    if (P->Interactive && P->ActiveId > 0) {
        AgentSession *S = PoolById(P, P->ActiveId);

        if (strcmp(Line, "back") == 0) {
            PoolLeave(P);
            return 1;

        } else if (strcmp(Line, "clear") == 0) {
            ClearScreen();
            PrintPrompt(P);

        } else if (strcmp(Line, "kill") == 0) {
            int Id = P->ActiveId;
            PoolLeave(P);
            PoolKill(P, Id);
            return 1;

        } else if (strcmp(Line, "help") == 0) {
            ConsolePrintHelp();
            PrintPrompt(P);

        } else if (strlen(Line) > 0) {
            if (!S) {
                printf("[!] Session no longer active.\n");
                PoolLeave(P);
                return 1;
            }
            PoolQueueCommand(S, Line);
            printf("\033[90m[*] queued — waiting for beacon...\033[0m\n");
            PrintPrompt(P);

        } else {
            PrintPrompt(P);
        }

        return 1;
    }

    if (strcmp(Line, "help") == 0) {
        ConsolePrintHelp();

    } else if (strcmp(Line, "clear") == 0) {
        ClearScreen();

    } else if (strcmp(Line, "exit") == 0 || strcmp(Line, "quit") == 0) {
        printf("Shutting down.\n");
        return 0;

    } else if (strcmp(Line, "sessions") == 0 || strcmp(Line, "session") == 0) {
        PoolList(P);

    } else if (strcmp(Line, "info") == 0) {
        printf("\n");
        ConfigPrint(C);
        printf("\n");

    } else if (strncmp(Line, "use ", 4) == 0) {
        int Id = atoi(Line + 4);
        if (Id > 0)
            PoolEnter(P, Id);
        else
            printf("[!] Usage: use <id>\n");
        return 1;

    } else if (strncmp(Line, "interact ", 9) == 0) {
        int Id = atoi(Line + 9);
        if (Id > 0)
            PoolEnter(P, Id);
        else
            printf("[!] Usage: interact <id>\n");
        return 1;

    } else if (strcmp(Line, "kill all") == 0) {
        for (int I = 0; I < P->Count; I++) {
            if (P->Slots[I].State == StateActive)
                PoolKill(P, P->Slots[I].Id);
        }
        return 1;

    } else if (strncmp(Line, "kill ", 5) == 0) {
        int Id = atoi(Line + 5);
        if (Id > 0)
            PoolKill(P, Id);
        else
            printf("[!] Usage: kill <id>\n");
        return 1;

    } else if (strlen(Line) > 0) {
        printf("[!] Unknown command — type 'help'.\n");
    }

    PrintPrompt(P);
    return 1;
}
